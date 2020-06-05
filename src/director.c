#include "director.h"
#include "client.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>

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
    void* channel;
    int message;
    int remaining_clients;

    mask_signals();

    int id = 0;
    pthread_t client_thread;
    struct client_args *args;

    unsigned int seed = time(NULL);

    while (!quit && !closing) {

        /**
         * waits if clients inside supermarket are more than c-e
         */
        director_mutex_lock(&clients_inside_lock);
        while (clients_inside >= c-e && !closing && !quit) {
            fprintf(stderr, "inside while: %d\n", clients_inside);
            director_cond_wait(&max_clients_inside, &clients_inside_lock);
            //fprintf(stderr, "waked up director by client\n");
        }
        director_mutex_unlock(&clients_inside_lock);

        if (closing || quit) {
            continue;
        }

        director_mutex_lock(&clients_inside_lock);

        //fprintf(stderr, "effective clients: %d\n", clients_inside);

        /**
         * starts c-clients_inside clients
         */
        for (int i = clients_inside; i < c; i++) {
            args = (struct client_args *)malloc(sizeof(struct client_args));
            args->id = id;
            args->max_cash = k;
            args->purchase_time = (rand_r(&seed) % (t - 9)) + 10;

            args->products = rand_r(&seed) % (p+1); 

            fprintf(stderr, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ %d\n", args->products);           
            pthread_create(&client_thread, NULL, client, (void*)args);

            pthread_detach(client_thread);

            clients_inside++;

            //fprintf(stderr, "added client %d\n", id);
            id++;
        }

        director_mutex_unlock(&clients_inside_lock);

        /**
         * handles clients with 0 products
         */
        while (!fifo_tsqueue_isempty(zero_products_q)) {
            fprintf(stderr, "bbbbbbbbbbbbbbbbbbbb director exiting client with 0 product");
            channel = fifo_tsqueue_pop(&zero_products_q);
            message = D_EXIT_MESSAGE;

            if (write(*(int*)channel, &message, sizeof(int)) < 0) {
                perror("director error during sendinng exit message to client");
                pthread_exit((void*)EXIT_FAILURE);
            }
            free(channel);

            director_mutex_lock(&dir_buff_lock);
            while(dir_buff_is_empty) {
                director_cond_wait(&dir_buff_empty, &dir_buff_lock);
            }

            fifo_tsqueue_push(&clients_info_q, (void*)dir_buff, sizeof(dir_buff));

            dir_buff_is_empty = 1;
            director_mutex_unlock(&dir_buff_lock);
        }
    }

    // waits for ending of all clients
    director_mutex_lock(&clients_inside_lock);
    remaining_clients = clients_inside;
    director_mutex_unlock(&clients_inside_lock);
    while (remaining_clients > 0) {
        sched_yield();
        
        director_mutex_lock(&clients_inside_lock);
        remaining_clients = clients_inside;
        director_mutex_unlock(&clients_inside_lock);
    }

    //fprintf(stderr, "end of client handler\n");

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
    int prod_time       = ((struct cashiers_handler_args *)arg)->product_time;
    int def_number      = ((struct cashiers_handler_args *)arg)->def_cashiers_number;
    int analytics_time  = ((struct cashiers_handler_args *)arg)->analytics_t_intervall;
    int s1              = ((struct cashiers_handler_args *)arg)->s1;
    int s2              = ((struct cashiers_handler_args *)arg)->s2;

    /**
     * array that saves if cashier i has at most 1 client waiting
     */
    int *one_client = (int*)calloc(k, sizeof(int));

    int remaining_clients;

    /**
     * number of cashiers open
     */
    int open_cashiers = 0;

    struct analytics_data *data;

    struct cashier_args *ca_args;
    pthread_t cashier_thread;
    unsigned int seed = time(NULL) ^ s1 ^ s2;
    
    /**
     * auxiliary variable for checking if a cashier is open
     * used in cashier chosing whiles
     */
    int cashier_open = 0;
    int i;

    mask_signals();

    for (i = 0; i<def_number; i++) {
        state[i] = 1;
    }

    /**
     * starts default cashiers number
     */
    for (i = 0; i<def_number; i++) {
        open_cashiers++;

        ca_args = (struct cashier_args*)malloc(sizeof(struct cashier_args));
        ca_args->id = i;
        ca_args->time_per_client = (rand_r(&seed) % (MAX_KTIME - MIN_KTIME + 1)) + MIN_KTIME;
        ca_args->analytics_time = analytics_time;
        ca_args->prod_time = prod_time;

        pthread_create(&cashier_thread, NULL, cashier, (void *)ca_args);
        pthread_detach(cashier_thread);
    }

    //pthread_join(cashier_thread, NULL);
    while (!quit) {

        director_mutex_lock(&clients_inside_lock);
        remaining_clients = clients_inside;
        director_mutex_unlock(&clients_inside_lock);
        
        if (closing && remaining_clients == 0) {
            break;
        }

        /**
         * waits for data in analytics queue
         */
        director_mutex_lock(analytics_q.mutex);
        while(ISEMPTY(analytics_q) && !quit) {
            fprintf(stderr, "analytics_q is empty\n");
            director_cond_wait(analytics_q.empty, analytics_q.mutex);
            fprintf(stderr, "director waked up by cashier\n");
        }
        director_mutex_unlock(analytics_q.mutex);

        if (quit) {
            continue;
        }
    
        data = (struct analytics_data *)fifo_tsqueue_pop(&analytics_q);

        fprintf(stderr, "aaaaaaaaaaaaaaaaaaaaaaaaaaa director received data from cashier %d n clients %d\n", data->id, data->n_clients);

        
        if (data->n_clients <= 1) {
            one_client[data->id] = 1;
        } else {
            one_client[data->id] = 0;
        }

        /** 
         * closing cashier if number of cashiers 
         * with at most 1 client in queue is equal to s1
         */
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
            pthread_cond_signal(cash_q[i].empty);
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
            ca_args->time_per_client = (rand_r(&seed) % (MAX_KTIME - MIN_KTIME + 1)) + MIN_KTIME;
            ca_args->analytics_time = analytics_time;
            ca_args->prod_time = prod_time;

            pthread_create(&cashier_thread, NULL, cashier, (void*)ca_args);

            pthread_detach(cashier_thread);   
        }

        free(data);
    }

    // signaling all waiting cashiers
    for (i = 0; i < k; i++) {
        pthread_mutex_lock(&state_lock[i]);
        cashier_open = state[i];
        pthread_mutex_unlock(&state_lock[i]);

        if (cashier_open) {
            pthread_mutex_lock(&state_lock[i]);
            state[i] = 0;
            pthread_mutex_unlock(&state_lock[i]);
            pthread_cond_signal(cash_q[i].empty);
        }
    }

    // waits for closing of all cashiers
    open_cashiers = 1;
    while (open_cashiers > 0) {

        for (i = 0; i<k; i++) {
            if (state[i] == 1) {
                open_cashiers = 1;
                fprintf(stderr, "%d ", i);
                break;
            }
        }
        fprintf(stderr, "\n");
        if (i == k) {
            open_cashiers = 0;
        } else {
            sched_yield();
        }
    }

    fprintf(stderr, "director cashier handler finished\n\n");

    free(one_client);

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

    mask_signals();

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

    free(ca_h_args);
    free(cl_h_args);

    cashier_thread_clear(k,c);
    client_thread_clear();

    /**
     * TODO: check if arguments from config file are valid
     * done TODO: start cashiers
     * done TODO: start clients
     * done TODO: hanlde clients with 0 products
     * done TODO: get client information from cashiers
     * TODO: write log file at the end
     * done TODO: closing and opening cashes in base of s1 and s2 parameters
     * TODO: get cashier information when it closes
     * done TODO: quit safely when quit = 1
     * TODO: solve lock when SIGHUP
     */

    pthread_exit((void*)EXIT_SUCCESS);
}