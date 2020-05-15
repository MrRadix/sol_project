#include "cashier.h"
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>


void cashier_mutex_lock(pthread_mutex_t *mtx, int id, void *p) {
    int err;

    if ((err = pthread_mutex_lock(mtx)) != 0) {
        errno = err;
        perror("cashier error on lock");
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
        perror("cashier error on unlock");
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
        perror("cashier error on wait");
        free(p);
        pthread_mutex_destroy(&state_lock[id]);
        pthread_mutex_destroy(&buff_lock[id]);
        pthread_cond_destroy(&buff_empty[id]);
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void send_analytics(int id, int msec) {
    struct timespec *res = (struct timespec *)malloc(sizeof(struct timespec));
    struct timespec *rem = (struct timespec *)malloc(sizeof(struct timespec));
    struct analytics_data *data = (struct analytics_data *)malloc(sizeof(struct analytics_data));

    float nsec = msec*1000000;
    int sec = 0;

    nsec = nsec/1000000000;
    sec = (int)nsec;
    nsec = (nsec-sec)*1000000000;
        
    res->tv_sec = sec;
    res->tv_nsec = (long)nsec;

    if (nanosleep(res, rem) == -1) {
        nanosleep(rem, NULL);
    }

    data->id = id;
    data->n_clients = fifo_tsqueue_n_items(cash_q[id]);

    fifo_tsqueue_push(&analytics_q, (void*)data, sizeof(data));
    
    free(data);
    free(res);
    free(rem);
    
    fprintf(stderr, "analytics queue with %d elements\n", fifo_tsqueue_n_items(analytics_q));

}

void pass_products(long msec, int id) {
    struct timespec *res = (struct timespec *)malloc(sizeof(struct timespec));
    struct timespec *rem = (struct timespec *)malloc(sizeof(struct timespec));
    float nsec = msec*1000000;
    int sec = 0;

    nsec = nsec/1000000000;
    sec = (int)nsec;
    nsec = (nsec-sec)*1000000000;

    res->tv_sec = sec;
    res->tv_nsec = nsec;
    
    if (nanosleep(res, rem) == -1) {
        nanosleep(rem, NULL);
    }

    free(res);
    free(rem);
}

void mask_signals(void) {
    sigset_t set;
    
    sigfillset(&set);
    if(pthread_sigmask(SIG_SETMASK, &set, NULL) != 0) {
        perror("client error during mask all sgnals");
    }
}

void *cashier(void *arg) {
    int id                 = ((struct cashier_args *)arg)->id;
    int time_per_client    = ((struct cashier_args *)arg)->time_per_client;
    long analytics_time    = ((struct cashier_args *)arg)->analytics_time;
    int prod_time          = ((struct cashier_args *)arg)->prod_time;
    int open;
    int is_empty;
    int cpipe;
    long time_spent_client;
    struct timeval start, stop, result;

    void *p = NULL;
    int message;

    free(arg);

    mask_signals();

    //fprintf(stderr, "------------opening cashier: %d-------------\n", id);

    cashier_mutex_lock(&state_lock[id], id, arg);
    open = state[id];
    cashier_mutex_unlock(&state_lock[id], id, arg);

    while (open && !quit) {
        
        gettimeofday(&start, NULL);

        /**
         * waits for a client in queue
         */
        cashier_mutex_lock(cash_q[id].mutex, id, arg);

        //fprintf(stderr, "cahsier %d queue is empty before while: %d\n", id, ISEMPTY(cash_q[id]));

        while (ISEMPTY(cash_q[id])) {
            fprintf(stderr, "cashier %d waiting for someone in queue\n", id);
            cashier_mutex_lock(&state_lock[id], id, arg);
            open = state[id];
            cashier_mutex_unlock(&state_lock[id], id, arg);

            /**
             * checks if director decided to close cash
             */
            if (!open && quit) {
                break;
            }

            cashier_cond_wait(cash_q[id].empty, cash_q[id].mutex, id, arg);
        }
        cashier_mutex_unlock(cash_q[id].mutex, id, arg);

        if (!open && quit) {  
            continue;
        }

        //fprintf(stderr, "cahsier %d queue is empty outside while: %d\n", id, ISEMPTY(cash_q[id]));
        //fprintf(stderr, "=======> cashier %d pooping pipe\n", id);
        //fifo_tsqueue_print(cash_q[id]);
        p = fifo_tsqueue_pop(&cash_q[id]);

        //fprintf(stderr, "cashier %d poped %p\n", id, p);
        //fprintf(stderr, "cahsier %d queue is empty after pop: %d\n", id, ISEMPTY(cash_q[id]));

        if (p == NULL) {
            fprintf(stderr, "cashier %d p == NULL\n", id);
            continue;
        }

        if ((cpipe = *(int*)p) < 0) {
            perror("cashier error during pop from queue");
            pthread_exit((void*)EXIT_FAILURE);
        }
        free(p);

        message = NEXT;
        write(cpipe, &message, sizeof(int));

        //fprintf(stderr, "=========> cashier %d written NEXT\n", id);


        /**
         * waits for data in buffer
         */
        cashier_mutex_lock(&buff_lock[id], id, arg);
        while (buff_is_empty[id]) {
            //fprintf(stderr, "=========> cashier %d buff is empty\n", id);
            cashier_cond_wait(&buff_empty[id], &buff_lock[id], id, arg);
            //fprintf(stderr, "=========> cashier %d waked up\n", id);
        }

        //fprintf(stderr, "=========> cashier %d sleeping...\n", id);
        // sleeps (simulates cshier products scanning period)
        pass_products(time_per_client+prod_time*buff[id].n_products, id);

        //fprintf(stderr, "=========> cashier %d finished sleeping\n", id);

        buff_is_empty[id] = 1;

        //fprintf(stderr, "cashier %d received all data from client %d\n", id, buff[id].id);

        gettimeofday(&stop, NULL);

        fifo_tsqueue_push(&clients_info_q, (void*)&buff[id], sizeof(buff[id]));

        cashier_mutex_unlock(&buff_lock[id], id, arg);

        timersub(&stop, &start, &result);

        time_spent_client = (result.tv_sec*1000000 + result.tv_usec)/1000;

        fprintf(stderr, "cashier %d time spent for clirnt %ld\n", id, time_spent_client);

        if (analytics_time-time_spent_client < 0) {
            send_analytics(id, 0);
        } else {
            send_analytics(id, analytics_time-time_spent_client);
        }

        cashier_mutex_lock(&state_lock[id], id, arg);
        open = state[id];
        cashier_mutex_unlock(&state_lock[id], id, arg);
    }

    /**
     * sending of close message to enqueued customers
     */
    is_empty = fifo_tsqueue_isempty(cash_q[id]);
    while(!is_empty && is_empty >= 0) {
        
        fprintf(stderr, "cahsier %d sending closing message\n", id);

        p = fifo_tsqueue_pop(&cash_q[id]);
        //fprintf(stderr, "cashier %d closing, is empty %d\n", id, is_empty);
        if (p == NULL) {
            is_empty = fifo_tsqueue_isempty(cash_q[id]);
            fprintf(stderr, "cashier %d closing, p is NULL\n", id);
            continue;
        }

        if ((cpipe = *(int*)p) < 0) {
            perror("cashier error during pop from queue");
            pthread_exit((void*)EXIT_FAILURE);
        }
        free(p);
        
        message = CLOSING;
        write(cpipe, &message, sizeof(int));

        is_empty = fifo_tsqueue_isempty(cash_q[id]);
    }
    if (is_empty < 0) {
        perror("cashier error during checking if queue was empty");
        pthread_exit((void*)EXIT_FAILURE);
    }

    //fprintf(stderr, "------------closing cashier: %d-------------\n", id);

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

void cash_queue_clear(fifo_tsqueue_t **queue, int dim) {

    for (int i = 0; i<dim; i++) {
        if (fifo_tsqueue_destroy(&(*queue)[i]) != 0) {
            perror("cashier error during destroy cashes queues");
            free(*queue);
            return;
        }
    }
    free(*queue);
}

void cash_lock_init(pthread_mutex_t **lock, int dim) {
    *lock = (pthread_mutex_t *)malloc(dim*sizeof(pthread_mutex_t));

    for (int i = 0; i < dim; i++) {
        pthread_mutex_init(&(*lock)[i], NULL);
    }
}

void cash_lock_destroy(pthread_mutex_t **lock, int dim) {

    for (int i = 0; i < dim; i++) {
        pthread_mutex_destroy(&(*lock)[i]);
    }

    free(*lock);
}

void cash_cond_init(pthread_cond_t **cond, int dim) {
    *cond = (pthread_cond_t *)malloc(dim*sizeof(pthread_cond_t));

    for (int i = 0; i < dim; i++) {
        pthread_cond_init(&(*cond)[i], NULL);
    }
}

void cash_cond_destroy(pthread_cond_t **cond, int dim) {

    for (int i = 0; i < dim; i++) {
        pthread_cond_destroy(&(*cond)[i]);
    }

    free(*cond);
}


void cashier_thread_init(int n_cashiers, int n_clients) {
    cash_queue_init(&cash_q, n_cashiers);
    cash_lock_init(&state_lock, n_cashiers);
    cash_lock_init(&buff_lock, n_cashiers);
    cash_cond_init(&buff_empty, n_cashiers);
    cash_state_init(&buff_is_empty, n_cashiers, 0);
    cash_state_init(&state, n_cashiers, 0);
    buff = (client_data *)malloc(n_cashiers*sizeof(client_data));

    fifo_tsqueue_init(&clients_info_q);
    fifo_tsqueue_init(&analytics_q);
}

void cashier_thread_clear(int n_cashiers, int n_clients) {
    cash_queue_clear(&cash_q, n_cashiers);
    cash_lock_destroy(&state_lock, n_cashiers);
    cash_lock_destroy(&buff_lock, n_cashiers);
    cash_cond_destroy(&buff_empty, n_cashiers);
    free(buff_is_empty);
    free(state);
    free(buff);

    fifo_tsqueue_destroy(&clients_info_q);
    fifo_tsqueue_destroy(&analytics_q);

}