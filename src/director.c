#include "director.h"
#include "cashier.h"
#include "client.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void director_mutex_lock(pthread_mutex_t *mtx) {
    int err;

    if ((err = pthread_mutex_lock(mtx)) != 0) {
        errno = err;
        perror("director error lock");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void director_mutex_unlock(pthread_mutex_t *mtx) {
    int err;

    if ((err = pthread_mutex_unlock(mtx)) != 0) {
        errno = err;
        perror("director error unlock");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void director_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx) {
    int err;

    if ((err = pthread_cond_wait(cond, mtx)) != 0) {
        errno = err;
        perror("director error unlock");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

//int c, int e, int k, int t, int p
void *clients_handler(void *arg) {
    int c = ((struct clients_handler_args *)arg)->n_clients;
    int e = ((struct clients_handler_args *)arg)->n_clients_group;
    int k = ((struct clients_handler_args *)arg)->n_cashiers;
    int t = ((struct clients_handler_args *)arg)->client_max_time;
    int p = ((struct clients_handler_args *)arg)->n_max_products;

    int effective_clients;
    int id = 0;
    pthread_t client_thread;
    struct client_args *args;

    unsigned int seed = time(NULL) ^ c ^ e ^ k;

    client_thread_init();

    while (!quit && !closing) {

        director_mutex_lock(&clients_inside_lock);
        while (clients_inside >= c-e) {
            fprintf(stderr, "inside while: %d\n", clients_inside);
            director_cond_wait(&max_clients_inside, &clients_inside_lock);
        }
        
        fprintf(stderr, "effective clients: %d\n", clients_inside);

        effective_clients = clients_inside;
        for (int i = clients_inside; i < c; i++) {
            args = (struct client_args *)malloc(sizeof(struct client_args));
            args->id = id;
            args->max_cash = k;
            args->purchase_time = (rand_r(&seed) % (t - 9)) + 10;
            args->products = rand_r(&seed) % p+1;
            
            pthread_create(&client_thread, NULL, client, (void*)args);
            clients_inside++;

            fprintf(stderr, "added client %d\n", id);
            id++;
        }

        fprintf(stderr, "added %d clients\n", clients_inside-effective_clients);

        director_mutex_unlock(&clients_inside_lock);
    }

    fprintf(stderr, "end of client handler\n");

    pthread_exit((void*)EXIT_SUCCESS);
}

// int k, int c, int prod_time, int def_number, int s1, int s2
void *cashiers_handler(void *arg) {
    int k           = ((struct cashiers_handler_args *)arg)->n_cashiers;
    int c           = ((struct cashiers_handler_args *)arg)->n_clients;
    int prod_time   = ((struct cashiers_handler_args *)arg)->product_time;
    int def_number  = ((struct cashiers_handler_args *)arg)->def_cashiers_number;
    int s1          = ((struct cashiers_handler_args *)arg)->s1;
    int s2          = ((struct cashiers_handler_args *)arg)->s2;

    struct cashier_args *ca_args;
    pthread_t cashier_thread;
    unsigned int seed = time(NULL) ^ s1 ^ s2;

    cashier_thread_init(k, c);

    for (int i = 0; i<def_number; i++) {
        state[i] = 1;
    }

    for (int i = 0; i<def_number; i++) {

        ca_args = (struct cashier_args*)malloc(sizeof(struct cashier_args));
        ca_args->id = i;
        ca_args->time = (rand_r(&seed) % (MAX_KTIME - MIN_KTIME + 1)) + MIN_KTIME;
        ca_args->prod_time = prod_time;

        pthread_create(&cashier_thread, NULL, cashier, (void *)ca_args);
    }

    pthread_join(cashier_thread, NULL);

    pthread_exit((void*)EXIT_SUCCESS);

}


void *director(void *arg) {
    int c               = ((struct director_args *)arg)->n_clients;
    int k               = ((struct director_args *)arg)->n_cashiers;
    int e               = ((struct director_args *)arg)->n_client_group;
    int prod_time       = ((struct director_args *)arg)->product_time;
    int t               = ((struct director_args *)arg)->client_max_time;
    int p               = ((struct director_args *)arg)->n_max_product;
    int def_cash_n      = ((struct director_args *)arg)->def_cashiers_number;
    int s1              = ((struct director_args *)arg)->s1;
    int s2              = ((struct director_args *)arg)->s2;

    quit = 0;
    closing = 0;

    struct clients_handler_args *cl_h_args;
    struct cashiers_handler_args *ca_h_args;

    pthread_t cl_h_thread;
    pthread_t ca_h_thread;

    pthread_mutex_init(&clients_inside_lock, NULL);


    // starting clients handler

    cl_h_args = (struct clients_handler_args *)malloc(sizeof(struct clients_handler_args));
    cl_h_args->n_clients = c;
    cl_h_args->n_cashiers = k;
    cl_h_args->n_clients_group = e;
    cl_h_args->n_max_products = p;
    cl_h_args->client_max_time = t;

    pthread_create(&cl_h_thread, NULL, clients_handler, (void*)cl_h_args);


    // starting cashiers handler

    ca_h_args = (struct cashiers_handler_args *)malloc(sizeof(struct cashiers_handler_args));
    ca_h_args->n_cashiers = k;
    ca_h_args->n_clients = c;
    ca_h_args->def_cashiers_number = def_cash_n;
    ca_h_args->product_time = prod_time;
    ca_h_args->s1 = s1;
    ca_h_args->s2 = s2;

    pthread_create(&ca_h_thread, NULL, cashiers_handler, (void*)ca_h_args);

    pthread_join(cl_h_thread, NULL);
    pthread_join(ca_h_thread, NULL);

    /**
     * TODO: check if arguments from config file are valid
     * TODO: start cashiers
     * TODO: start clients
     * TODO: get client information from cashiers
     * TODO: closing and opening cashes in base of s1 and s2 parameters
     * TODO: get cashier information when it closes
     * TODO: handle clients with 0 products
     * TODO: quit safely when quit = 1
     */

    pthread_exit((void*)EXIT_SUCCESS);
}