#include "client.h"
#include "cashier.h"
#include "director.h"
#include <stdio.h>
#include <stdlib.h>

#define K 6     // cashers number
#define INITKN 1 // initial cashiers number INITKN > 0
#define C 500        // customers number
#define E 499        // customers every group 0 < E < C
#define T 11         // max time for purchases in milliseconds T > 10
#define P 100        // max number of product for each customer P >= 0

/**
 * time in milliseconds to process every product
 */
#define PRODTIME 5

#define ANALYTICS_INTERVAL 100

int client_cashier_test();

int main(int argc, char const *argv[])
{
    pthread_t director_thread;
    struct director_args *d_args = (struct director_args *)malloc(sizeof(struct director_args));
    d_args->client_max_time = T;
    d_args->def_cashiers_number = INITKN;
    d_args->n_cashiers = K;
    d_args->n_clients = C;
    d_args->n_client_group = E;
    d_args->n_max_product = P;
    d_args->product_time = PRODTIME;
    d_args->analytics_t_intervall = ANALYTICS_INTERVAL;
    d_args->s1 = 1;
    d_args->s2 = 2;

    pthread_create(&director_thread, NULL, director, (void *)d_args);

    pthread_join(director_thread, NULL);

    //client_cashier_test();

    return 0;
}

int client_cashier_test()
{
    struct cashier_args *ca_args;
    struct client_args *cl_args;
    pthread_t cashier_t;
    pthread_t client_t;

    client_thread_init();
    cashier_thread_init(K, C);

    for (int i = 0; i < K; i++)
    {
        state[i] = 1;
    }

    for (int i = 0; i < K; i++)
    {

        ca_args = (struct cashier_args *)malloc(sizeof(struct cashier_args));
        ca_args->id = i;
        ca_args->time = 300;
        ca_args->prod_time = PRODTIME;

        pthread_create(&cashier_t, NULL, cashier, (void *)ca_args);
    }

    for (int i = 0; i < C; i++)
    {

        cl_args = (struct client_args *)malloc(sizeof(struct client_args));
        cl_args->id = i;
        cl_args->purchase_time = 70;
        cl_args->max_cash = K;
        cl_args->products = 20;

        pthread_create(&client_t, NULL, client, (void *)cl_args);
    }

    pthread_join(cashier_t, NULL);

    fprintf(stderr, "fineeeeeee\n");

    return 0;
}