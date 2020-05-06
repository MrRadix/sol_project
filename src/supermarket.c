#include "client.h"
#include "cashier.h"
#include <stdio.h>
#include <stdlib.h>

#define K 2     // cashers number
#define C 100     // customers number
#define E 10    // customers every group
#define T 100   // max time for purchases in milliseconds
#define P 40    // max number of product for each customer

#define INITNK 1

/**
 * time interval in milliseconds between two consecutives 
 * sendings of data from cashier to director
 */
#define INFOINT 500

/**
 * time in milliseconds to process every product
 */
#define PRODTIME 50


int client_cashier_test();


int main(int argc, char const *argv[])
{
    client_cashier_test();
    
    return 0;
}



int client_cashier_test() {
    struct cashier_args *ca_args;
    struct client_args *cl_args;
    pthread_t cashier_t;
    pthread_t client_t;

    cashiers_init(K, C);

    for (int i = 0; i<K; i++) {
        state[i] = 1;
    }

    fprintf(stderr, "STATE: ");
    for (int i = 0; i<K; i++) {
        fprintf(stderr, "%d ", state[i]);
    }
    fprintf(stderr, "\n");

    for (int i = 0; i<K; i++) {

        ca_args = (struct cashier_args*)malloc(sizeof(struct cashier_args));
        ca_args->id = i;
        ca_args->time = 300;
        ca_args->prod_time = PRODTIME;

        pthread_create(&cashier_t, NULL, cashier, (void *)ca_args);
    }

    for (int i = 0; i<C; i++) {

        cl_args = (struct client_args*)malloc(sizeof(struct client_args));
        cl_args->id = i;
        cl_args->p_time = 70;
        cl_args->max_cash = K;
        cl_args->products = 20;

        pthread_create(&client_t, NULL, client, (void *)cl_args);
    }

    for (int i = 0; i<K; i++) {

        pthread_join(cashier_t, NULL);
    }

    fprintf(stderr, "fineeeeeee\n");

    return 0;
}