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

void *clients_handler(void *arg) {
    int c = ((struct clients_handler_args *)arg)->n_clients;
    int e = ((struct clients_handler_args *)arg)->n_clients_group;
    int k = ((struct clients_handler_args *)arg)->n_cashiers;
    int t = ((struct clients_handler_args *)arg)->client_max_time;
    int p = ((struct clients_handler_args *)arg)->n_max_products;

    int id = 0;
    pthread_t client_thread;
    struct client_args *args;

    unsigned int seed = time(NULL) ^ c ^ e;

    while (!quit && !closing) {

        director_mutex_lock(&clients_inside_lock);
        while (clients_inside >= c-e) {
            fprintf(stderr, "inside while: %d\n", clients_inside);
            director_cond_wait(&max_clients_inside, &clients_inside_lock);
            fprintf(stderr, "waked up director by client\n");
        }

        fprintf(stderr, "effective clients: %d\n", clients_inside);

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

        director_mutex_unlock(&clients_inside_lock);
    }

    fprintf(stderr, "end of client handler\n");

    pthread_exit((void*)EXIT_SUCCESS);
}

static int cashiers_with_one_c(int *cashiers, int n) {
    int ret = 0;

    for (int i = 0; i < n; i++) {
        ret += cashiers[i];
    }

    return ret;
}

// int k, int c, int prod_time, int def_number, int s1, int s2
void *cashiers_handler(void *arg) {
    int k               = ((struct cashiers_handler_args *)arg)->n_cashiers;
    int c               = ((struct cashiers_handler_args *)arg)->n_clients;
    int prod_time       = ((struct cashiers_handler_args *)arg)->product_time;
    int def_number      = ((struct cashiers_handler_args *)arg)->def_cashiers_number;
    int analytics_time  = ((struct cashiers_handler_args *)arg)->analytics_t_intervall;
    int s1              = ((struct cashiers_handler_args *)arg)->s1;
    int s2              = ((struct cashiers_handler_args *)arg)->s2;

    /**
     * array that saves if cashier i has at most 1 client waiting
     */
    int *one_client = (int*)calloc(k, sizeof(int));

    /**
     * number of cashiers open
     */
    int open_cashiers = 0;

    struct analytics_data *data;

    struct cashier_args *ca_args;
    pthread_t cashier_thread;
    unsigned int seed = time(NULL) ^ s1 ^ s2;
    
    int cashier_open = 0;
    int cashier_id = 0;
    int i;

    for (i = 0; i<def_number; i++) {
        state[i] = 1;
    }

    for (i = 0; i<def_number; i++) {
        open_cashiers++;

        ca_args = (struct cashier_args*)malloc(sizeof(struct cashier_args));
        ca_args->id = i;
        ca_args->time = (rand_r(&seed) % (MAX_KTIME - MIN_KTIME + 1)) + MIN_KTIME;
        ca_args->analytics_time = analytics_time;
        ca_args->prod_time = prod_time;

        pthread_create(&cashier_thread, NULL, cashier, (void *)ca_args);
    }

    //pthread_join(cashier_thread, NULL);
    /**
     * TODO: solve opening cashier 2 every time
     */
    while (!quit) {

        director_mutex_lock(&analytics_q.mutex);
        while(ISEMPTY(analytics_q))
            director_cond_wait(&analytics_q.empty, &analytics_q.mutex);
        pthread_mutex_unlock(&analytics_q.mutex);
        
        data = (struct analytics_data *)malloc(sizeof(struct analytics_data));
        data = (struct analytics_data *)fifo_tsqueue_pop(&analytics_q);

        
        if (data->n_clients <= 1) {
            one_client[data->id] = 1;
        } else {
            one_client[data->id] = 0;
        }

        /** 
         * closing cashier if number of cashiers 
         * with at most 1 client in queue is equal to s1
         */
        //fprintf(stderr, "cashiers with one client: %d\n", cashiers_with_one_c(one_client, k));
        if (cashiers_with_one_c(one_client, k) >= s1 && open_cashiers > 1) {

            for (i = 0; i < k; i++) {
                one_client[i] = 0;
            }
            
            cashier_open = 0;
            i = 0;
            while (!cashier_open && i < k) {
                pthread_mutex_lock(&state_lock[i]);
                cashier_open = state[i];
                pthread_mutex_unlock(&state_lock[i]);
                i++;
            }
            i--;

            fprintf(stderr, "==========================> closing cashier %d\n", i);
            pthread_mutex_lock(&state_lock[i]);
            state[i] = 0;
            pthread_mutex_unlock(&state_lock[i]);

            open_cashiers--;
        }

        /** 
         * opening cashier if there is a cashier
         * with number client in queue at least of s2
         */
        if (data->n_clients >= s2 && open_cashiers < k) {
            
            cashier_open = 1;
            i = 0;
            while (cashier_open && i < k) {
                pthread_mutex_lock(&state_lock[i]);
                cashier_open = state[i];
                pthread_mutex_unlock(&state_lock[i]);
                i++;
            }
            i--;

            fprintf(stderr, "==========================> opening cashier %d\n", i);
            pthread_mutex_lock(&state_lock[i]);
            state[i] = 1;
            pthread_mutex_unlock(&state_lock[i]);

            open_cashiers++;
            
            ca_args = (struct cashier_args*)malloc(sizeof(struct cashier_args));
            ca_args->id = i;
            ca_args->time = (rand_r(&seed) % (MAX_KTIME - MIN_KTIME + 1)) + MIN_KTIME;
            ca_args->analytics_time = analytics_time;
            ca_args->prod_time = prod_time;

            pthread_create(&cashier_thread, NULL, cashier, (void*)ca_args);   
        }

        free(data);
    }

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
    int analytics_time  = ((struct director_args *)arg)->analytics_t_intervall;
    int s1              = ((struct director_args *)arg)->s1;
    int s2              = ((struct director_args *)arg)->s2;

    quit = 0;
    closing = 0;

    struct clients_handler_args *cl_h_args;
    struct cashiers_handler_args *ca_h_args;

    pthread_t cl_h_thread;
    pthread_t ca_h_thread;

    pthread_mutex_init(&clients_inside_lock, NULL);
    
    cashier_thread_init(k, c);
    client_thread_init();

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
    ca_h_args->analytics_t_intervall = analytics_time;
    ca_h_args->s1 = s1;
    ca_h_args->s2 = s2;

    pthread_create(&ca_h_thread, NULL, cashiers_handler, (void*)ca_h_args);

    pthread_join(cl_h_thread, NULL);
    pthread_join(ca_h_thread, NULL);

    /**
     * TODO: check if arguments from config file are valid
     * TODO: start cashiers
     * done TODO: start clients
     * TODO: get client information from cashiers
     * TODO: closing and opening cashes in base of s1 and s2 parameters
     * TODO: get cashier information when it closes
     * TODO: handle clients with 0 products
     * TODO: quit safely when quit = 1
     */

    pthread_exit((void*)EXIT_SUCCESS);
}