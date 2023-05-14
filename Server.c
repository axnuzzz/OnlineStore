#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "headers.h"

// Set a read lk on the specified file descriptor with the given lk attributes
void setLockCust(int fd_custs, struct flock lock_cust){

    lock_cust.l_len = 0;
    lock_cust.l_type = F_RDLCK;
    lock_cust.l_start = 0;
    lock_cust.l_whence = SEEK_SET;
    fcntl(fd_custs, F_SETLKW, &lock_cust);

    return ;
}

// Unlock the specified file descriptor by setting the lock type to F_UNLCK

void unlock(int fd, struct flock lk){
    lk.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lk);
}

// Set a read lock on the specified file descriptor with the given lk attributes

void productReadLock(int fd, struct flock lk){
    lk.l_len = 0;
    lk.l_type = F_RDLCK;
    lk.l_start = 0;
    lk.l_whence = SEEK_SET;
    fcntl(fd, F_SETLKW, &lk);
}

// Set a write lock on the specified file descriptor with the given lock attributes, adjusting the file pointer to the previous struct product

void productWriteLock(int fd, struct flock lk){
    lseek(fd, (-1)*sizeof(struct product), SEEK_CUR);
    lk.l_type = F_WRLCK;
    lk.l_whence = SEEK_CUR;
    lk.l_start = 0;
    lk.l_len = sizeof(struct product);

    fcntl(fd, F_SETLKW, &lk);
}

// Set a lock on the specified file descriptor at the specified offset with the given lock attributes (read or write lock)

void cartOffsetLock(int fd_cart, struct flock lock_cart, int offset, int ch){
    lock_cart.l_whence = SEEK_SET;
    lock_cart.l_len = sizeof(struct cart);
    lock_cart.l_start = offset;
    if (ch == 1){
        //rdlck
        lock_cart.l_type = F_RDLCK;
    }else{
        //wrlck
        lock_cart.l_type = F_WRLCK;
    }
    fcntl(fd_cart, F_SETLKW, &lock_cart);
    lseek(fd_cart, offset, SEEK_SET);
}

// Get the offset associated with the given customer ID in the file represented by the file descriptor, using file locking

int getOffset(int cust_id, int fd_custs){
    if (cust_id < 0){
        return -1;
    }
    struct flock lock_cust;
    setLockCust(fd_custs, lock_cust);
    struct index id;

    while (read(fd_custs, &id, sizeof(struct index))){
        if (id.custid == cust_id){
            unlock(fd_custs, lock_cust);
            return id.offset;
        }
    }
    unlock(fd_custs, lock_cust);
    return -1;
}

//This function is called when the admin wants to add a product to the store

void addProducts(int fd, int new_filedesc, int filedesc_admin){

    char nm[50];
    char resp[100];
    int id, qty, price;

    struct product p1;
    //reads data from the file associated with the file descriptor new_filedesc and stores it in the memory location pointed to by &p1
    int n = read(new_filedesc, &p1, sizeof(struct product));

    strcpy(nm, p1.name);
    id = p1.id;
    qty = p1.qty;
    price = p1.price;

    struct flock lk;
    productReadLock(fd, lk);

    struct product p;

    int flg = 0;
    //If a product with the same ID exists, then it fails and then unlocks
    while (read(fd, &p, sizeof(struct product))){

        if (p.id == id && p.qty > 0){
            write(new_filedesc, "Duplicate product\n", sizeof("Duplicate product\n"));
            sprintf(resp, "Adding product with product id %d failed as product id is duplicate\n", id);
            write(filedesc_admin, resp, strlen(resp));
            unlock(fd, lk);
            flg = 1;
            return;
        }
    }

    //If the product ID is unique, then it adds it successfully to the store and then unlocks
    if (!flg){

        lseek(fd, 0, SEEK_END);
        p.id = id;
        strcpy(p.name, nm);
        p.price = price;
        p.qty = qty;

        write(fd, &p, sizeof(struct product));
        write(new_filedesc, "Added successfully\n", sizeof("Added succesfully\n"));
        sprintf(resp, "New product with product id %d added successfully\n", id);
        write(filedesc_admin, resp, strlen(resp));
        unlock(fd, lk);   
    }
}

//This function is called when user asks to see the inventory

void listProducts(int fd, int new_filedesc){

    struct flock lk;
    productReadLock(fd, lk);

    struct product p;
    while (read(fd, &p, sizeof(struct product))){
        if (p.id != -1){
            write(new_filedesc, &p, sizeof(struct product));
        }
    }
    
    p.id = -1;
    write(new_filedesc, &p, sizeof(struct product));
    unlock(fd, lk);
}

//This function is called by the admin when they want to delete a product
void deleteProduct(int fd, int new_filedesc, int id, int filedesc_admin){

    struct flock lk;
    productReadLock(fd, lk);
    char resp[100];

    struct product p;
    int flg = 0;
    //If the Product ID exists, then it deletes the product from the store
    while (read(fd, &p, sizeof(struct product))){
        if (p.id == id){
            
            unlock(fd, lk);
            productWriteLock(fd, lk);

            p.id = -1;
            strcpy(p.name, "");
            p.price = -1;
            p.qty = -1;

            write(fd, &p, sizeof(struct product));
            write(new_filedesc, "Delete successful", sizeof("Delete successful"));
            sprintf(resp, "Product with product id %d deleted succesfully\n", id);
            write(filedesc_admin, resp, strlen(resp));

            unlock(fd, lk);
            flg = 1;
            return;
        }
    }

    //If the product ID does not exist, then it's unable to delete the product
    if (!flg){
        sprintf(resp, "Deleting product with product id %d failed as product does not exist\n", id);
        write(filedesc_admin, resp, strlen(resp));
        write(new_filedesc, "Product id invalid", sizeof("Product id invalid"));
        unlock(fd, lk);
    }
}

//This function is called when admin asks to edit the price of a product in the inventory(ch=1) or dmin asks to edit the quantity of a product in the inventory(ch=2)
void updateProduct(int fd, int new_filedesc, int ch, int filedesc_admin){
    int id;
    int val = -1;
    struct product p1;
    read(new_filedesc, &p1, sizeof(struct product));
    id = p1.id;

    char resp[100];
    
    if (ch == 1){
        val = p1.price;
    }else{
        val = p1.qty;
    }

    struct flock lk;
    productReadLock(fd, lk);

    int flg = 0;
    
    struct product p;
    while (read(fd, &p, sizeof(struct product))){
        if (p.id == id){

            unlock(fd, lk);
            productWriteLock(fd, lk);
            int old;
            //If ch is 1 then update the price
            if (ch == 1){
                old = p.price;
                p.price = val;
            }
            //Else update the quantity
            else{
                old = p.qty;
                p.qty = val;
            }

            write(fd, &p, sizeof(struct product));
            if (ch == 1){
                write(new_filedesc, "Price modified successfully", sizeof("Price modified successfully"));
                sprintf(resp, "Price of product with product id %d modified from %d to %d \n", id, old, val);
                write(filedesc_admin, resp, strlen(resp));
            }else{
                sprintf(resp, "Quantity of product with product id %d modified from %d to %d \n", id, old, val);
                write(filedesc_admin, resp, strlen(resp));
                write(new_filedesc, "Quantity modified successfully", sizeof("Quantity modified successfully"));               
            }

            unlock(fd, lk);
            flg = 1;
            break;
        }
    }

    //If the product ID is invalid, the it'll unlock
    if (!flg){
        write(new_filedesc, "Product id invalid", sizeof("Product id invalid"));
        unlock(fd, lk);
    }
}

//This function is called to add the user as a new customer
void addCustomer(int fd_cart, int fd_custs, int new_filedesc){
    char buffer;
    read(new_filedesc, &buffer, sizeof(char));
    if (buffer == 'y'){

        struct flock lk;
        setLockCust(fd_custs, lk);
        
        int max_id = -1; 
        struct index id ;
        while (read(fd_custs, &id, sizeof(struct index))){
            if (id.custid > max_id){
                max_id = id.custid;
            }
        }

        max_id ++;
        
        id.custid = max_id;
        id.offset = lseek(fd_cart, 0, SEEK_END);
        lseek(fd_custs, 0, SEEK_END);
        write(fd_custs, &id, sizeof(struct index));

        struct cart c;
        c.custid = max_id;
        for (int i=0; i<MAX_PROD; i++){
            c.products[i].id = -1;
            strcpy(c.products[i].name , "");
            c.products[i].qty = -1;
            c.products[i].price = -1;
        }

        write(fd_cart, &c, sizeof(struct cart));
        unlock(fd_custs, lk);
        write(new_filedesc, &max_id, sizeof(int));
    }
}

//This function is called ny the user when they want to view their cart
void viewCart(int fd_cart, int new_filedesc, int fd_custs){
    int cust_id = -1;
    read(new_filedesc, &cust_id, sizeof(int));

    int offset = getOffset(cust_id, fd_custs);    
    struct cart c;

    //If the offset is -1, indicating that the customer ID was not found in the customer file
    if (offset == -1){

        struct cart c;
        c.custid = -1;
        write(new_filedesc, &c, sizeof(struct cart));
        
    }
    //If the offset is not -1, indicating that the customer ID was found in the customer file
    else{
        struct cart c;
        struct flock lock_cart;
        
        cartOffsetLock(fd_cart, lock_cart, offset, 1);
        read(fd_cart, &c, sizeof(struct cart));
        write(new_filedesc, &c, sizeof(struct cart));
        unlock(fd_cart, lock_cart);
    }
}

//This function is called when the user wants to add a product to the cart

void addProductToCart(int fd, int fd_cart, int fd_custs, int new_filedesc) {
    // Read customer ID from the new file descriptor
    int cust_id = -1;
    read(new_filedesc, &cust_id, sizeof(int));

    // Get the offset associated with the customer ID from the customer file
    int offset = getOffset(cust_id, fd_custs);

    // Write the offset to the new file descriptor
    write(new_filedesc, &offset, sizeof(int));

    // If the offset is -1, customer ID not found in customer file, return
    if (offset == -1) {
        return;
    }

    // Set a lock on the cart file at the specified offset for reading
    struct flock lock_cart;
    cartOffsetLock(fd_cart, lock_cart, offset, 1);

    // Read the cart information from the cart file
    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));

    // Set a lock on the product file for reading
    struct flock lock_prod;
    productReadLock(fd, lock_prod);

    // Read the product information from the new file descriptor
    struct product p;
    read(new_filedesc, &p, sizeof(struct product));

    // Get the product ID and quantity
    int prod_id = p.id;
    int qty = p.qty;

    // Search for the product in the product file
    struct product p1;
    int found = 0;
    while (read(fd, &p1, sizeof(struct product))) {
        if (p1.id == p.id) {
            if (p1.qty >= p.qty) {
                found = 1;
                break;
            }
        }
    }
    // Unlock the cart file and product file
    unlock(fd_cart, lock_cart);
    unlock(fd, lock_prod);

    // If the product was not found or out of stock, return
    if (!found) {
        write(new_filedesc, "Product id invalid or out of stock\n", sizeof("Product id invalid or out of stock\n"));
        return;
    }

    int flg = 0;
    int flg1 = 0;

    // Check if the product already exists in the cart
    for (int i = 0; i < MAX_PROD; i++) {
        if (c.products[i].id == p.id) {
            flg1 = 1;
            break;
        }
    }

    // If the product already exists in the cart, return
    if (flg1) {
        write(new_filedesc, "Product already exists in cart\n", sizeof("Product already exists in cart\n"));
        return;
    }

    // Add the product to the cart
    for (int i = 0; i < MAX_PROD; i++) {
        if (c.products[i].id == -1) {
            flg = 1;
            c.products[i].id = p.id;
            c.products[i].qty = p.qty;
            strcpy(c.products[i].name, p1.name);
            c.products[i].price = p1.price;
            break;
        } else if (c.products[i].qty <= 0) {
            flg = 1;
            c.products[i].id = p.id;
            c.products[i].qty = p.qty;
            strcpy(c.products[i].name, p1.name);
            c.products[i].price = p1.price;
            break;
        }
    }

       // If the cart limit is reached, return
    if (!flg) {
        write(new_filedesc, "Cart limit reached\n", sizeof("Cart limit reached\n"));
        return;
    }

    // Write a success message to the new file descriptor
    write(new_filedesc, "Item added to cart\n", sizeof("Item added to cart\n"));

    // Set a lock on the cart file at the specified offset for writing
    cartOffsetLock(fd_cart, lock_cart, offset, 2);

    // Write the updated cart information to the cart file
    write(fd_cart, &c, sizeof(struct cart));

    // Unlock the cart file
    unlock(fd_cart, lock_cart);
}

//This function is called whrn the user wants to edit the product in cart

void editProductInCart(int fd, int fd_cart, int fd_custs, int new_filedesc) {
    // Read customer ID from the new file descriptor
    int cust_id = -1;
    read(new_filedesc, &cust_id, sizeof(int));

    // Get the offset associated with the customer ID from the customer file
    int offset = getOffset(cust_id, fd_custs);

    // Write the offset to the new file descriptor
    write(new_filedesc, &offset, sizeof(int));

    // If the offset is -1, customer ID not found in customer file, return
    if (offset == -1) {
        return;
    }

    // Set a lock on the cart file at the specified offset for reading
    struct flock lock_cart;
    cartOffsetLock(fd_cart, lock_cart, offset, 1);

    // Read the cart information from the cart file
    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));

    int pid, qty;
    struct product p;
    // Read the updated product information from the new file descriptor
    read(new_filedesc, &p, sizeof(struct product));

    pid = p.id;
    qty = p.qty;

    int flg = 0;
    int i;
    for (i = 0; i < MAX_PROD; i++) {
        if (c.products[i].id == pid) {
            // Set a lock on the product file for reading
            struct flock lock_prod;
            productReadLock(fd, lock_prod);

            struct product p1;
            while (read(fd, &p1, sizeof(struct product))) {
                if (p1.id == pid && p1.qty > 0) {
                    if (p1.qty >= qty) {
                        flg = 1;
                        break;
                    } else {
                        flg = 0;
                        break;
                    }
                }
            }

            // Unlock the product file
            unlock(fd, lock_prod);
            break;
        }
    }

    // Unlock the cart file
    unlock(fd_cart, lock_cart);

    // If the product was not found or out of stock, return
    if (!flg) {
        write(new_filedesc, "Product id not in the order or out of stock\n", sizeof("Product id not in the order or out of stock\n"));
        return;
    }

    // Update the quantity of the product in the cart
    c.products[i].qty = qty;

    // Write a success message to the new file descriptor
    write(new_filedesc, "Update successful\n", sizeof("Update successful\n"));

    // Set a lock on the cart file at the specified offset for writing
    cartOffsetLock(fd_cart, lock_cart, offset, 2);

    // Write the updated cart information to the cart file
    write(fd_cart, &c, sizeof(struct cart));

    // Unlock the cart file
    unlock(fd_cart, lock_cart);
}

//This function is called when the user wants to checkout and pay for his cart
void payment(int fd, int fd_cart, int fd_custs, int new_filedesc){
    int cust_id = -1;
    read(new_filedesc, &cust_id, sizeof(int));

    int offset;
    offset = getOffset(cust_id, fd_custs);

    write(new_filedesc, &offset, sizeof(int));
    if (offset == -1){
        return;
    }

    struct flock lock_cart;
    cartOffsetLock(fd_cart, lock_cart, offset, 1);

    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));
    unlock(fd_cart, lock_cart);
    write(new_filedesc, &c, sizeof(struct cart));

    int total = 0;

    for (int i=0; i<MAX_PROD; i++){

        if (c.products[i].id != -1){
            write(new_filedesc, &c.products[i].qty, sizeof(int));

            struct flock lock_prod;
            productReadLock(fd, lock_prod);
            lseek(fd, 0, SEEK_SET);

            struct product p;
            int flg = 0;
            while (read(fd, &p, sizeof(struct product))){

                if (p.id == c.products[i].id && p.qty > 0) {
                    int min ;
                    if (c.products[i].qty >= p.qty){
                        min = p.qty;
                    }else{
                        min = c.products[i].qty;
                    }

                    flg =1;
                    write(new_filedesc, &min, sizeof(int));
                    write(new_filedesc, &p.price, sizeof(int));
                    break;
                }
            }

            unlock(fd, lock_prod);

            if (!flg){
                //product got deleted midway
                int val = 0;
                write(new_filedesc, &val, sizeof(int));
                write(new_filedesc, &val, sizeof(int));
            }
        }      
    }

    char ch;
    read(new_filedesc, &ch, sizeof(char));

    for (int i=0; i<MAX_PROD; i++){

        struct flock lock_prod;
        productReadLock(fd, lock_prod);
        lseek(fd, 0, SEEK_SET);

        struct product p;
        while (read(fd, &p, sizeof(struct product))){

            if (p.id == c.products[i].id) {
                int min ;
                if (c.products[i].qty >= p.qty){
                    min = p.qty;
                }else{
                    min = c.products[i].qty;
                }
                unlock(fd, lock_prod);
                productWriteLock(fd, lock_prod);
                p.qty = p.qty - min;

                write(fd, &p, sizeof(struct product));
                unlock(fd, lock_prod);
            }
        }

        unlock(fd, lock_prod);
    }
    
    cartOffsetLock(fd_cart, lock_cart, offset, 2);

    for (int i=0; i<MAX_PROD; i++){
        c.products[i].id = -1;
        strcpy(c.products[i].name, "");
        c.products[i].price = -1;
        c.products[i].qty = -1;
    }

    write(fd_cart, &c, sizeof(struct cart));
    write(new_filedesc, &ch, sizeof(char));
    unlock(fd_cart, lock_cart);

    read(new_filedesc, &total, sizeof(int));
    read(new_filedesc, &c, sizeof(struct cart));

    int fd_rec = open("receipt.txt", O_CREAT | O_RDWR, 0777);
    write(fd_rec, "ProductID\tProductName\tQuantity\tPrice\n", strlen("ProductID\tProductName\tQuantity\tPrice\n"));
    char temp[100];
    for (int i=0; i<MAX_PROD; i++){
        if (c.products[i].id != -1){
            sprintf(temp, "%d\t%s\t%d\t%d\n", c.products[i].id, c.products[i].name, c.products[i].qty, c.products[i].price);
            write(fd_rec, temp, strlen(temp));
        }
    }
    sprintf(temp, "Total - %d\n", total);
    write(fd_rec, temp, strlen(temp));
    close(fd_rec);
}

void generateAdminReceipt(int filedesc_admin, int fd) {
    // Set a lock on the product file for reading
    struct flock lk;
    productReadLock(fd, lk);

    // Write the header for the admin receipt
    write(filedesc_admin, "Current Inventory:\n", strlen("Current Inventory:\n"));
    write(filedesc_admin, "ProductID\tProductName\tQuantity\tPrice\n", strlen("ProductID\tProductName\tQuantity\tPrice\n"));

    // Set the file position to the beginning of the product file
    lseek(fd, 0, SEEK_SET);

    struct product p;
    while (read(fd, &p, sizeof(struct product))) {
        if (p.id != -1) {
            char temp[100];
            // Format the product information into a string
            sprintf(temp, "%d\t%s\t%d\t%d\n", p.id, p.name, p.qty, p.price);

            // Write the formatted product information to the admin receipt
            write(filedesc_admin, temp, strlen(temp));
        }
    }

    // Unlock the product file
    unlock(fd, lk);
}


int main(){
    printf("Setting up server\n");

    // Open files for storing records, orders, customers, and admin receipt
    int fd = open("records.txt", O_RDWR | O_CREAT, 0777);
    int fd_cart = open("orders.txt", O_RDWR | O_CREAT, 0777);
    int fd_custs = open("customers.txt", O_RDWR | O_CREAT, 0777);
    int filedesc_admin = open("adminReceipt.txt", O_RDWR | O_CREAT, 0777);

    // Set the file position to the end of the admin receipt file
    lseek(filedesc_admin, 0, SEEK_END);

    // Create a socket for the server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        perror("Error: ");
        return -1;
    }

    struct sockaddr_in serv, cli;

    // Set up the server address and port
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(5555);

    int opt = 1;
    // Set socket option to reuse address
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Error: ");
        return -1;
    }

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *)&serv, sizeof(serv)) == -1) {
        perror("Error: ");
        return -1;
    }

    // Listen for incoming connections on the socket
    if (listen(sockfd, 5) == -1) {
        perror("Error: ");
        return -1;
    }

    int size = sizeof(cli);
    printf("Server set up successfully\n");


    while (1){

        int new_filedesc = accept(sockfd, (struct sockaddr *)&cli, &size);
        if (!fork()){
            printf("Connection with client successful\n");
            close(sockfd);

            int user;
            read(new_filedesc, &user, sizeof(int));
            
            if (user == 1){

                char ch;
                while (1){
                    read(new_filedesc, &ch, sizeof(char));

                    // Reset file pointers to the beginning of the files
                    lseek(fd, 0, SEEK_SET);
                    lseek(fd_cart, 0, SEEK_SET);
                    lseek(fd_custs, 0, SEEK_SET);

                    if (ch == 'a'){
                        // Terminate connection with the client
                        close(new_filedesc);
                        break;
                    }
                    else if (ch == 'b'){
                        // List all products
                        listProducts(fd, new_filedesc);
                    }
                    else if (ch == 'c'){
                        // View cart
                        viewCart(fd_cart, new_filedesc, fd_custs);
                    }
                    else if (ch == 'd'){
                        // Add product to cart
                        addProductToCart(fd, fd_cart, fd_custs, new_filedesc);
                    }
                    else if (ch == 'e'){
                        // Edit product in cart
                        editProductInCart(fd, fd_cart, fd_custs, new_filedesc);
                    }
                    else if (ch == 'f'){
                        // Process payment
                        payment(fd, fd_cart, fd_custs, new_filedesc);
                    }
                    else if (ch == 'g'){
                        // Add customer
                        addCustomer(fd_cart, fd_custs, new_filedesc);
                    }
                }
                printf("Connection terminated\n");
            }
            else if (user == 2){
                char ch;
                while (1){
                    read(new_filedesc, &ch, sizeof(ch));

                    // Reset file pointers to the beginning of the files
                    lseek(fd, 0, SEEK_SET);
                    lseek(fd_cart, 0, SEEK_SET);
                    lseek(fd_custs, 0, SEEK_SET);

                    if (ch == 'a'){
                        // Add products
                        addProducts(fd, new_filedesc, filedesc_admin);
                    } 
                    else if (ch == 'b'){
                        // Delete product
                        int id;
                        read(new_filedesc, &id, sizeof(int));
                        deleteProduct(fd, new_filedesc, id, filedesc_admin);
                    }
                    else if (ch == 'c'){
                        // Update product - quantity
                        updateProduct(fd, new_filedesc, 1, filedesc_admin);
                    }
                    else if (ch == 'd'){
                        // Update product - price
                        updateProduct(fd, new_filedesc, 2, filedesc_admin);
                    }
                    else if (ch == 'e'){
                        // List all products
                        listProducts(fd, new_filedesc);
                    }
                    else if (ch == 'f'){
                        // Terminate connection with the client and generate admin receipt
                        close(new_filedesc);
                        generateAdminReceipt(filedesc_admin, fd);
                        break;
                    }
                    else{
                        continue;
                    }
                }
            }
            printf("Connection terminated\n");
        }
        else{
            close(new_filedesc);
        }
    }
}