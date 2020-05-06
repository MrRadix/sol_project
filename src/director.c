#include "director.h"


void *director(void *arg) {
    int c       = ((struct director_args *)arg)->n_clients;
    int k       = ((struct director_args *)arg)->n_cashiers;
    int e       = ((struct director_args *)arg)->n_client_group;
    int p_time  = ((struct director_args *)arg)->product_time;
    int t       = ((struct director_args *)arg)->client_max_time;
    int p       = ((struct director_args *)arg)->n_max_product;
    int s1      = ((struct director_args *)arg)->s1;
    int s2      = ((struct director_args *)arg)->s2;

    /**
     * TODO: start cashiers
     * TODO: start clients
     * TODO: get client information from cashiers
     * TODO: closing and opening cashes in base of s1 and s2 parameters
     * TODO: get cashier information when it closes
     * TODO: handle clients with 0 products
     * TODO: quit safely when quit = 1
     */
}