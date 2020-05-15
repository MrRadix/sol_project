#include "director.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define K 10         // cashers number
#define INITKN 5     // initial cashiers number INITKN > 0
#define C 50        // customers number
#define E 3          // customers every group 0 < E < C
#define T 11            // max time for purchases in milliseconds T > 10
#define P 100           // max number of product for each customer P > 0

/**
 * time in milliseconds to process every product
 */
#define PRODTIME 5

#define ANALYTICS_INTERVAL 10

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
    d_args->s1 = 4;
    d_args->s2 = 10;

    pthread_create(&director_thread, NULL, director, (void *)d_args);

    pthread_join(director_thread, NULL);

    return 0;
}