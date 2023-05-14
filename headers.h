#include <stdio.h>
#define MAX_PROD 20

struct product
{
    int id;
    char name[50];
    int qty;
    int price;
};
struct cart
{
    int custid;
    struct product products[MAX_PROD];
};
struct index
{
    int custid;
    int offset;
};