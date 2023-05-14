# OnlineStore
An online store implemented using system calls and Linux programming in C as a part of the Operating Systems course.

# Client Menu:
You can log in as one of the following:
1. Admin (with permissions to update, add, and delete products)
2. User (with limited permissions to manage items in the cart)

# Admin Functionalities:
1. Add or delete a product
2. Update the quantity or price of a product

# User Functionalities:
1. View the list of available products
2. View the cart
3. Add or remove items from the cart
4. Update item quantities in the cart
5. Pay for items in the cart and generate a receipt

# Program Architecture:
1. The program consists of two files: Server.c and Client.c. Server.c is the server code, while Client.c is the client code used for login.
2. Sockets are utilized for communication between the server and client, and file locking is employed when accessing data files.
3. The program utilizes the following files:
   - customers.txt: Contains a list of registered customers
   - orders.txt: Stores the cart for each customer
   - records.txt: Holds the products in the inventory
   - receipt.txt: Stores receipts generated after successful payment

# Instructions to Run the Program:
1. Open a terminal and execute the following commands:

    ```
    gcc -o server Server.c
    ./server
    ```

2. In a separate terminal, run the following commands:

    ```
    gcc -o client Client.c
    ./client
    ```

3. You can now use the user menu or admin menu as directed by the program to perform operations on products or customers.

Please refer to the project report for implementation details