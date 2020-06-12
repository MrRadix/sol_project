#include "cashier.h"
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>


void cashier_mutex_lock(pthread_mutex_t *mtx) {
    int err;

    if ((err = pthread_mutex_lock(mtx)) != 0) {
        errno = err;
        perror("cashier error on lock");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void cashier_mutex_unlock(pthread_mutex_t *mtx) {
    int err;

    if ((err = pthread_mutex_unlock(mtx)) != 0) {
        errno = err;
        perror("cashier error on unlock");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void cashier_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx) {
    int err;

    if ((err = pthread_cond_wait(cond, mtx)) != 0) {
        errno = err;
        perror("cashier error on wait");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void *send_analytics(void *arg) {
    int id      = ((struct analytics_args*)arg)->id;
    int msec    = ((struct analytics_args*)arg)->msec;
    struct timespec *res = (struct timespec *)malloc(sizeof(struct timespec));
    struct timespec *rem = (struct timespec *)malloc(sizeof(struct timespec));
    struct analytics_data *data;
    int remaining_clients;
    int open;

    float nsec;
    long sec;

    nsec = (float)msec*1000000;
    sec = 0;

    nsec = nsec/1000000000;
    sec = (long)nsec;
    nsec = (nsec-sec)*1000000000;

    res->tv_sec = sec;
    res->tv_nsec = (long)nsec;

    cashier_mutex_lock(&state_lock[id]);
    open = state[id];
    cashier_mutex_unlock(&state_lock[id]);


    while (open && !quit) {

        cashier_mutex_lock(&clients_inside_lock);
        remaining_clients = clients_inside;
        cashier_mutex_unlock(&clients_inside_lock);
        
        if (closing && remaining_clients == 0) {
            break;
        }

        if (nanosleep(res, rem) == -1) {
            nanosleep(rem, NULL);
        }

        data = (struct analytics_data *)malloc(sizeof(struct analytics_data));
        data->id = id;
        data->timestamp = time(NULL);
        data->n_clients = fifo_tsqueue_n_items(cash_q[id]);

        fifo_tsqueue_push(&analytics_q, (void*)data, sizeof(*data));
        free(data);

        cashier_mutex_lock(&state_lock[id]);
        open = state[id];
        cashier_mutex_unlock(&state_lock[id]);
    }
    
    free(res);
    free(rem);
    free(arg);

    pthread_exit((void*)EXIT_SUCCESS);
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
        exit(EXIT_FAILURE);
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
    pthread_t analytics_thread;
    struct analytics_args *a_args = (struct analytics_args*)malloc(sizeof(struct analytics_args));
    
    struct timeval opening_start, opening_stop, opening_result;
    struct timeval client_time_start, client_time_stop, client_time_result;

    long opening_time;
    long client_time;

    void *p = NULL;
    int message;

    free(arg);

    mask_signals();

    a_args->id = id;
    a_args->msec = analytics_time;
    pthread_create(&analytics_thread, NULL, send_analytics, (void*)a_args);


    cashier_mutex_lock(&state_lock[id]);
    open = state[id];
    cashier_mutex_unlock(&state_lock[id]);

    // starts count time of opening
    gettimeofday(&opening_start, NULL);

    while (open && !quit) {

        /**
         * waits for a client in queue
         */
        cashier_mutex_lock(cash_q[id].mutex);

        while (ISEMPTY(cash_q[id]) && open && !quit) {

            cashier_cond_wait(cash_q[id].empty, cash_q[id].mutex);
            
            cashier_mutex_lock(&state_lock[id]);
            open = state[id];
            cashier_mutex_unlock(&state_lock[id]);
        }
        cashier_mutex_unlock(cash_q[id].mutex);

        if (!open || quit) {  
            continue;
        }

        // gets next waiting client
        p = fifo_tsqueue_pop(&cash_q[id]);

        if (p == NULL) {
            continue;
        }

        if ((cpipe = *(int*)p) < 0) {
            perror("cashier error during pop from queue");
            pthread_exit((void*)EXIT_FAILURE);
        }
        free(p);

        message = NEXT;
        write(cpipe, &message, sizeof(int));

        // start counting time for client
        gettimeofday(&client_time_start, NULL);

        /**
         * waits for data in buffer
         */
        cashier_mutex_lock(&buff_lock[id]);
        while (buff_is_empty[id]) {
            cashier_cond_wait(&buff_empty[id], &buff_lock[id]);
        }

        // sleeps (simulates cshier products scanning period)
        pass_products(time_per_client+prod_time*buff[id].n_products, id);

        cashier_mutex_lock(&n_prod_lock);
        n_total_products += buff[id].n_products;
        cashier_mutex_unlock(&n_prod_lock);

        fifo_tsqueue_push(&clients_info_q, (void*)&buff[id], sizeof(buff[id]));

        // stop counting time for client
        gettimeofday(&client_time_stop, NULL);

        timersub(&client_time_stop, &client_time_start, &client_time_result);
        client_time = (client_time_result.tv_sec*1000000 + client_time_result.tv_usec)/1000;

        /**
         * updating cashier info
         */
        cashier_mutex_lock(&cashiers_info_lock[id]);
        cashiers_info[id].n_clients += 1;
        cashiers_info[id].n_products += buff[id].n_products; 
        add(&(cashiers_info[id].time_per_client), client_time);
        cashier_mutex_unlock(&cashiers_info_lock[id]);

        buff_is_empty[id] = 1;

        cashier_mutex_unlock(&buff_lock[id]);

        cashier_mutex_lock(&n_clients_lock);
        n_total_clients += 1;
        cashier_mutex_unlock(&n_clients_lock);

        cashier_mutex_lock(&state_lock[id]);
        open = state[id];
        cashier_mutex_unlock(&state_lock[id]);
    }
    
    /**
     * sending of close message to enqueued customers
     */
    is_empty = fifo_tsqueue_isempty(cash_q[id]);
    while(!is_empty && is_empty >= 0) {
        
        p = fifo_tsqueue_pop(&cash_q[id]);
        if (p == NULL) {
            is_empty = fifo_tsqueue_isempty(cash_q[id]);
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

    pthread_join(analytics_thread, NULL);

    // calculating opening time
    gettimeofday(&opening_stop, NULL);
    timersub(&opening_stop, &opening_start, &opening_result);
    opening_time = (opening_result.tv_sec*1000000 + opening_result.tv_usec)/1000;

    // adding closing time to cashier info
    cashier_mutex_lock(&cashiers_info_lock[id]);
    cashiers_info[id].n_closings += 1;
    add(&(cashiers_info[id].time_per_operiod), opening_time);
    cashier_mutex_unlock(&cashiers_info_lock[id]);

    pthread_exit((void*)EXIT_SUCCESS);
}

static void cash_state_init(int **st, int dim, int val) {
    *st = (int*)malloc(dim*sizeof(int));

    for (int i = 0; i<dim; i++) {
        (*st)[i] = val;
    }
}

static void cash_queue_init(fifo_tsqueue_t **queue, int dim) {
    *queue = (fifo_tsqueue_t *)malloc(dim*sizeof(fifo_tsqueue_t));

    for (int i = 0; i<dim; i++) {
        if (fifo_tsqueue_init(&(*queue)[i]) != 0) {
            perror("error during initialize cashes queues");
            free(*queue);
            return;
        }
    }
}

static void cashiers_info_init(struct cashier_info **cinfo, int dim) {
    *cinfo = (struct cashier_info *)malloc(dim*sizeof(struct cashier_info));

    for (int i = 0; i<dim; i++) {
        (*cinfo)[i].n_clients = 0;
        (*cinfo)[i].n_products = 0;
        (*cinfo)[i].n_closings = 0;
        (*cinfo)[i].time_per_operiod = NULL;
        (*cinfo)[i].time_per_client = NULL;
    }
}

static void cashiers_info_clear(struct cashier_info **cinfo, int dim) {

    for (int i = 0; i<dim; i++) {
        destroy((*cinfo)[i].time_per_operiod);
        destroy((*cinfo)[i].time_per_client);
    }
}

static void cash_queue_clear(fifo_tsqueue_t **queue, int dim) {

    for (int i = 0; i<dim; i++) {
        if (fifo_tsqueue_destroy(&(*queue)[i]) != 0) {
            perror("cashier error during destroy cashes queues");
            free(*queue);
            return;
        }
    }
    free(*queue);
}

static void cash_lock_init(pthread_mutex_t **lock, int dim) {
    *lock = (pthread_mutex_t *)malloc(dim*sizeof(pthread_mutex_t));

    for (int i = 0; i < dim; i++) {
        pthread_mutex_init(&(*lock)[i], NULL);
    }
}


static void cash_cond_init(pthread_cond_t **cond, int dim) {
    *cond = (pthread_cond_t *)malloc(dim*sizeof(pthread_cond_t));

    for (int i = 0; i < dim; i++) {
        pthread_cond_init(&(*cond)[i], NULL);
    }
}


void cashier_thread_init(int n_cashiers, int n_clients) {
    cash_queue_init(&cash_q, n_cashiers);
    cash_lock_init(&state_lock, n_cashiers);
    cash_lock_init(&buff_lock, n_cashiers);
    cash_cond_init(&buff_empty, n_cashiers);
    cash_state_init(&buff_is_empty, n_cashiers, 1);
    cash_state_init(&state, n_cashiers, 0);
    buff = (client_info *)malloc(n_cashiers*sizeof(client_info));

    cashiers_info_init(&cashiers_info, n_cashiers);
    cash_lock_init(&cashiers_info_lock, n_cashiers);

    pthread_mutex_init(&clients_inside_lock, NULL);
    pthread_cond_init(&max_clients_inside, NULL);
    clients_inside = 0;

    pthread_mutex_init(&n_prod_lock, NULL);
    n_total_products = 0;

    pthread_mutex_init(&n_clients_lock, NULL);
    n_total_clients = 0;

    fifo_tsqueue_init(&clients_info_q);
    fifo_tsqueue_init(&analytics_q);
}

void cashier_thread_clear(int n_cashiers, int n_clients) {
    cash_queue_clear(&cash_q, n_cashiers);
    cashiers_info_clear(&cashiers_info, n_cashiers);
    free(state_lock);
    free(buff_lock);
    free(buff_empty);
    free(buff_is_empty);
    free(state);
    free(buff);

    fifo_tsqueue_destroy(&clients_info_q);
    fifo_tsqueue_destroy(&analytics_q);

}