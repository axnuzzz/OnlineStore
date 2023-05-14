#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "headers.h"
#include <fcntl.h>
#include <sys/stat.h>

void displayMenuUser(){
    // Display menu options for the user
    printf("Menu to choose from\n");
    printf("a. To exit the menu\n");
    printf("b. To see all the products available\n");
    printf("c. To see your cart\n");
    printf("d. To add products to your cart\n");
    printf("e. To edit an existing product in your cart\n");
    printf("f. To proceed for payment\n");
    printf("g. To register a new customer\n");
    printf("Please enter your choice\n");
}

void displayMenuAdmin(){
    // Display menu options for the admin
    printf("Menu to choose from\n");
    printf("a. To add a product\n");
    printf("b. To delete a product\n");
    printf("c. To update the price of an existing product\n");
    printf("d. To update the quantity of an existing product\n");
    printf("e. To see your inventory\n");
    printf("f. To exit the program\n");
    printf("Please enter your choice\n");
}

void printProduct(struct product p){
    // Print the details of a product
    if (p.id != -1 && p.qty > 0){
        printf("%d\t%s\t%d\t%d\n", p.id, p.name, p.qty, p.price);
    }
}

void getInventory(int sockfd){
    // Fetch and display the inventory from the server
    printf("Fetching data\n");
    printf("ProductID\tProductName\tQuantityInStock\tPrice\n");
    while (1){
        struct product p;
        read(sockfd, &p, sizeof(struct product));
        if (p.id != -1){
            printProduct(p);
        }else{
            break;
        }
    }
}


int calculateTotal(struct cart c){
    // Calculate the total price of the products in the cart
    int total = 0;
    for (int i=0; i<MAX_PROD; i++){
        if (c.products[i].id != -1){
            total += c.products[i].qty * c.products[i].price;
        }
    }

    return total;
}

void generateReceipt(int total, struct cart c, int sockfd){
    // Generate and send the receipt to the client via the socket connection
    write(sockfd, &total, sizeof(int));
    write(sockfd, &c, sizeof(struct cart));
}

// Input functions

int custIdTaker(){
    // Take customer ID as input from the user
    int custId = -1;
    while (1){
        printf("Enter customer id\n");
        scanf("%d", &custId);

        if (custId < 0){
            printf("Customer id can't be negative, try again\n");
        }else{
            break;
        }
    }
    return custId;
}

int prodIdTaker(){
    // Take product ID as input from the user
    int prodId = -1;
    while (1){
        printf("Enter product id\n");
        scanf("%d", &prodId);

        if (prodId < 0){
            printf("Product id can't be negative, try again\n");
        }else{
            break;
        }
    }
    return prodId;
}


int priceTaker(){
    // Take the price as input from the user
    int price = -1;
    while (1){
        printf("Enter price\n");
        scanf("%d", &price);

        if (price < 0){
            printf("Price can't be negative, try again\n");
        }else{
            break;
        }
    }
    return price;
}

int quantityTaker(){
    // Take the quantity as input from the user
    int qty = -1;
    while (1){
        printf("Enter quantity\n");
        scanf("%d", &qty);

        if (qty < 0){
            printf("Quantity can't be negative, try again\n");
        }else{
            break;
        }
    }
    return qty;
}


int main(){
    printf("Establishing connection to server\n");
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1){
        perror("Error: ");
        return -1;
    }

    struct sockaddr_in serv;
    
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(5555);

    if (connect(sockfd, (struct sockaddr *)&serv, sizeof(serv)) == -1){
        perror("Error: ");
        return -1;
    }

    printf("Success\n");
    printf("Are you a user or the admin? Press 1 for user, 2 for admin\n");
    int user;
    scanf("%d", &user);
    write(sockfd, &user, sizeof(user));

    if (user == 1){
        while (1){
            displayMenuUser();
            char ch;
            scanf("%c",&ch);
            scanf("%c",&ch);

            write(sockfd, &ch, sizeof(char));

            if (ch == 'a'){
                break;
            }
            else if (ch == 'b'){
                getInventory(sockfd);
            }
            else if (ch == 'c'){
                int cusid = custIdTaker();
                
                write(sockfd, &cusid, sizeof(int));

                struct cart o;
                read(sockfd, &o, sizeof(struct cart));

                if (o.custid != -1){
                    printf("Customer ID %d\n", o.custid);
                    printf("ProductID\tProductName\tQuantityInStock\tPrice\n");
                    for (int i=0; i<MAX_PROD; i++){
                        printProduct(o.products[i]);
                    }
                }else{
                    printf("Wrong customer id provided\n");
                }
            }

            else if (ch == 'd'){
                int cusid = custIdTaker();
                
                write(sockfd, &cusid, sizeof(int));

                int res;
                read(sockfd, &res, sizeof(int));
                if (res == -1){
                    printf("Invalid customer id\n");
                    continue;
                }
                char response[80];
                int pid, qty;
                pid = prodIdTaker();
                
                while (1){
                    printf("Enter quantity\n");
                    scanf("%d", &qty);

                    if (qty <= 0){
                        printf("Quantity can't be <= 0, try again\n");
                    }else{
                        break;
                    }
                }

                struct product p;
                p.id = pid;
                p.qty = qty;

                write(sockfd, &p, sizeof(struct product));
                read(sockfd, response, sizeof(response));
                printf("%s", response);
                
            }

            else if (ch == 'e'){
                int cusid = custIdTaker();
                
                write(sockfd, &cusid, sizeof(int));

                int res;
                read(sockfd, &res, sizeof(int));
                if (res == -1){
                    printf("Invalid customer id\n");
                    continue;
                }

                int pid, qty;
                pid = prodIdTaker();
                qty = quantityTaker();
                
                struct product p;
                p.id = pid;
                p.qty = qty;

                write(sockfd, &p, sizeof(struct product));

                char response[80];
                read(sockfd, response, sizeof(response));
                printf("%s", response);
            }

            else if (ch == 'f'){
                int cusid = custIdTaker();
                write(sockfd, &cusid, sizeof(int));

                int res;
                read(sockfd, &res, sizeof(int));
                if (res == -1){
                    printf("Invalid customer id\n");
                    continue;
                }

                struct cart c;
                read(sockfd, &c, sizeof(struct cart));

                int ordered, instock, price;
                for (int i=0; i<MAX_PROD; i++){

                    if (c.products[i].id != -1){
                        read(sockfd, &ordered, sizeof(int));
                        read(sockfd, &instock, sizeof(int));
                        read(sockfd, &price, sizeof(int));
                        printf("Product id- %d\n", c.products[i].id);
                        printf("Ordered - %d; In stock - %d; Price - %d\n", ordered, instock, price);
                        c.products[i].qty = instock;
                        c.products[i].price = price;
                    }
                }

                int total = calculateTotal(c);
                
                printf("Total in your cart\n");
                printf("%d\n", total);
                int payment;

                while (1){
                    printf("Please enter the amount to pay\n");
                    scanf("%d", &payment);

                    if (payment != total){
                        printf("Wrong total entered, enter again\n");
                    }else{
                        break;
                    }
                }

                char ch = 'y';
                printf("Payment recorded, order placed\n");
                write(sockfd, &ch, sizeof(char));
                read(sockfd, &ch, sizeof(char));
                generateReceipt(total, c, sockfd);
            }

            else if (ch == 'g'){
                char conf;
                printf("Press y/n if you want to continue\n");
                scanf("%c", &conf);
                scanf("%c", &conf);

                write(sockfd, &conf, sizeof(char));
                if (conf == 'y'){

                    int id;
                    read(sockfd, &id, sizeof(int));
                    printf("Your new customer id : %d\n", id);
                    
                }else{
                    printf("Request aborted\n");
                }
            }
            else{
                printf("Invalid choice, try again\n");
            }

            
        }
    }
    else if (user == 2){
        
        while (1){
            displayMenuAdmin();
            char ch;
            scanf("%c",&ch);
            scanf("%c",&ch);
            write(sockfd, &ch, sizeof(ch));

            if (ch == 'a'){
                //add a product
                int id, qty, price;
                char name[50];

                printf("Enter product name\n");
                scanf("%s", name);
                id = prodIdTaker();
                qty = quantityTaker();
                price = priceTaker();
                
                struct product p;
                p.id = id;
                strcpy(p.name, name);
                p.qty = qty;
                p.price = price;

                int n1 = write(sockfd, &p, sizeof(struct product));

                char response[80];
                int n = read(sockfd, response, sizeof(response));
                response[n] = '\0';

                printf("%s", response);
            }

            else if (ch == 'b'){
                // printf("Enter product id to be deleted\n");
                int id = prodIdTaker();
                
                write(sockfd, &id, sizeof(int));
                //deleting is equivalent to setting everything as -1

                char response[80];
                read(sockfd, response, sizeof(response));
                printf("%s\n", response);
            }

            else if (ch == 'c'){
                int id = prodIdTaker();

                int price = priceTaker();
                
                struct product p;
                p.id = id;
                p.price = price;
                write(sockfd, &p, sizeof(struct product));

                char response[80];
                read(sockfd, response, sizeof(response));
                printf("%s\n", response);
            }

            else if (ch == 'd'){
                int id = prodIdTaker();
                int qty = quantityTaker();

                struct product p;
                p.id = id;
                p.qty = qty;
                write(sockfd, &p, sizeof(struct product));

                char response[80];
                read(sockfd, response, sizeof(response));
                printf("%s\n", response);

            }

            else if (ch == 'e'){
                getInventory(sockfd);
            }

            else if (ch == 'f'){
                break;
            }

            else{
                printf("Invalid choice, try again\n");
            }
        }
    }

    printf("Exiting program\n");
    close(sockfd);
    return 0;
}