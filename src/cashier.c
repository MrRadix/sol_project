#include "cashier.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

void cashier_mutex_lock(pthread_mutex_t *mtx, int id, void *p) {
    int err;

    if ((err = pthread_mutex_lock(mtx)) != 0) {
        errno = err;
        perror("lock");
        free(p);
        pthread_mutex_destroy(&state_lock[id]);
        pthread_mutex_destroy(&buff_lock[id]);
        pthread_cond_destroy(&buff_empty[id]);
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void cashier_mutex_unlock(pthread_mutex_t *mtx, int id, void *p) {
    int err;

    if ((err = pthread_mutex_unlock(mtx)) != 0) {
        errno = err;
        perror("lock");
        free(p);
        pthread_mutex_destroy(&state_lock[id]);
        pthread_mutex_destroy(&buff_lock[id]);
        pthread_cond_destroy(&buff_empty[id]);
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void cashier_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx, int id, void *p) {
    int err;

    if ((err = pthread_cond_wait(cond, mtx)) != 0) {
        errno = err;
        perror("lock");
        free(p);
        pthread_mutex_destroy(&state_lock[id]);
        pthread_mutex_destroy(&buff_lock[id]);
        pthread_cond_destroy(&buff_empty[id]);
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void *analytics(void *arg) {
    int intervall   = ((struct analytics_args*)arg)->intervall;
    int id          = ((struct analytics_args*)arg)->id;

    struct timespec *res = (struct timespec *)malloc(sizeof(struct timespec));
    struct timespec *rem = (struct timespec *)malloc(sizeof(struct timespec));

    struct analytics_data *data;

    while (!quit && !closing) {

        res->tv_sec = 0;
        res->tv_nsec = intervall;

        if (nanosleep(res, rem) == -1) {
            nanosleep(rem, NULL);
        }

        data = (struct analytics_data *)malloc(sizeof(struct analytics_data));
        data->id = id;
        data->n_clients = fifo_tsqueue_n_items(&cash_q[id]);

        fifo_tsqueue_push(&analytics_q, (void*)data, sizeof(data));

        fprintf(stderr, "analytics queue with %d elements\n", fifo_tsqueue_n_items(&analytics_q));
    }

}

void pass_products(long nsec) {
    struct timespec *res = (struct timespec *)malloc(sizeof(struct timespec));
    struct timespec *rem = (struct timespec *)malloc(sizeof(struct timespec));

    res->tv_sec = 0;
    res->tv_nsec = nsec;

    if (nanosleep(res, rem) == -1) {
        nanosleep(rem, NULL);
    }

    free(res);
    free(rem);
} 

void *cashier(void *arg) {
    int id              = ((struct cashier_args *)arg)->id;
    int time            = ((struct cashier_args *)arg)->time;
    int analytics_time  = ((struct cashier_args *)arg)->analytics_time;
    int prod_time       = ((struct cashier_args *)arg)->prod_time;
    int open            = 1;
    int is_empty;
    int cpipe;
    client_data data;
    pthread_t analytics_thread;
    
    struct analytics_args *a_args = (struct analytics_args*)malloc(sizeof(struct analytics_args));
    a_args->id = id;
    a_args->intervall = analytics_time;

    pthread_create(&analytics_thread, NULL, analytics, (void*)a_args);
    /**
     * done TODO: start analytics thread
     */

    /**
     * TODO: send to director client data
     */ 

    fprintf(stderr, "lkasjldkjaslkajsd\n");

    while (open) {

        /**
         * waits for a client in queue
         */
        cashier_mutex_lock(&cash_q[id].mutex, id, arg);
        
        while (ISEMPTY(cash_q[id])) {
            fprintf(stderr, "cashier %d waiting for someone in queue\n", id);
            cashier_mutex_lock(&state_lock[id], id, arg);
            open = state[id];
            cashier_mutex_unlock(&state_lock[id], id, arg);

            /**
             * checks if director decided to close cash
             */
            if (!open) {
                break;
            }

            cashier_cond_wait(&cash_q[id].empty, &cash_q[id].mutex, id, arg);
        }
        cashier_mutex_unlock(&cash_q[id].mutex, id, arg);

        if (!open) {  
            break;
        }

        if ((cpipe = *(int*)fifo_tsqueue_pop(&cash_q[id])) < 0) {
            perror("cashier error during pop from queue");
            free(arg);
            pthread_exit((void*)EXIT_FAILURE);
        }

        int message = NEXT;
        write(cpipe, &message, sizeof(int));


        /**
         * waits for data in buffer
         */
        cashier_mutex_lock(&buff_lock[id], id, arg);
        while (buff_is_empty[id]) {
            cashier_cond_wait(&buff_empty[id], &buff_lock[id], id, arg);
        }

        // sleeps (simulates cshier products scanning period)
        pass_products(time+prod_time*buff[id].n_products);

        data.id = buff[id].id;
        data.n_products = buff[id].n_products;
        data.q_time = buff[id].q_time;
        data.q_viewed = buff[id].q_viewed;
        data.sm_time = buff[id].sm_time;

        buff_is_empty[id] = 1;

        cashier_mutex_unlock(&buff_lock[id], id, arg);

        fprintf(stderr, "cashier %d received all data from client %d\n", id, buff[id].id);

        cashier_mutex_lock(&state_lock[id], id, arg);
        open = state[id];
        cashier_mutex_unlock(&state_lock[id], id, arg);
    }

    /**
     * sending of close message to enqueued customers
     */
    is_empty = fifo_tsqueue_isempty(cash_q[id]);
    while(!is_empty && is_empty >= 0) {
        
        if ((cpipe = *(int*)fifo_tsqueue_pop(&cash_q[id]) < 0)) {
            perror("cashier error during pop from queue");
            free(arg);
            pthread_exit((void*)EXIT_FAILURE);
        }

        write(cpipe, CLOSING, sizeof(int));

        is_empty = fifo_tsqueue_isempty(cash_q[id]);
    }
    if (is_empty < 0) {
        perror("cashier error during checking if queue was empty");
        free(arg);
        pthread_exit((void*)EXIT_FAILURE);
    }

    free(arg);

    pthread_exit((void*)EXIT_SUCCESS);
}

void cash_state_init(int **st, int dim, int val) {
    *st = (int*)malloc(dim*sizeof(int));

    for (int i = 0; i<dim; i++) {
        (*st)[i] = val;
    }
}

void cash_queue_init(fifo_tsqueue_t **queue, int dim) {
    *queue = (fifo_tsqueue_t *)malloc(dim*sizeof(fifo_tsqueue_t));

    for (int i = 0; i<dim; i++) {
        if (fifo_tsqueue_init(&(*queue)[i]) != 0) {
            perror("error during initialize cashes queues");
            free(*queue);
            return;
        }
    }
}

void cash_lock_init(pthread_mutex_t **lock, int dim) {
    *lock = (pthread_mutex_t *)malloc(dim*sizeof(pthread_mutex_t));

    for (int i = 0; i < dim; i++) {
        pthread_mutex_init(&(*lock)[i], NULL);
    }
}

void cash_cond_init(pthread_cond_t **cond, int dim) {
    *cond = (pthread_cond_t *)malloc(dim*sizeof(pthread_cond_t));

    for (int i = 0; i < dim; i++) {
        pthread_cond_init(&(*cond)[i], NULL);
    }
}

void cash_buff_init(client_data **buff, int dim) {
    *buff = (client_data *)malloc(dim*sizeof(client_data));
}

void cashier_thread_init(int n_cashiers, int n_clients) {
    cash_queue_init(&cash_q, n_cashiers);
    cash_lock_init(&state_lock, n_cashiers);
    cash_lock_init(&buff_lock, n_cashiers);
    cash_cond_init(&buff_empty, n_cashiers);
    cash_state_init(&buff_is_empty, n_cashiers, 1);
    cash_state_init(&state, n_cashiers, 0);
    cash_buff_init(&buff, n_cashiers);

    fifo_tsqueue_init(&analytics_q);
}