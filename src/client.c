#include "client.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>

void client_mutex_lock(pthread_mutex_t *mtx) {
    int err;

    if ((err = pthread_mutex_lock(mtx)) != 0) {
        errno = err;
        perror("client mutex lock error");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void client_mutex_unlock(pthread_mutex_t *mtx) {
    int err;

    if ((err = pthread_mutex_unlock(mtx)) != 0) {
        errno = err;
        perror("client mutex unlock error");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void client_cond_signal(pthread_cond_t *cond) {
    int err;

    if ((err = pthread_cond_signal(cond)) != 0) {
        errno = err;
        perror("client cond signal error");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void client_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    int err;

    if ((err = pthread_cond_wait(cond, mutex)) != 0) {
        errno = err;
        perror("client cond wait error");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void purchase(int msec) {
    struct timespec *res = (struct timespec *)malloc(sizeof(struct timespec));
    struct timespec *rem = (struct timespec *)malloc(sizeof(struct timespec));
    float nsec = msec*1000000;
    int sec = 0;

    nsec = nsec/1000000000;
    sec = (int)nsec;
    nsec = (nsec-sec)*1000000000;

    res->tv_sec = sec;
    res->tv_nsec = nsec;

    // purchasing products...
    if (nanosleep(res, rem) == -1) {
        nanosleep(rem, NULL);
    }

    free(res);
    free(rem);
}

void client_cleanup(void *cpipe) {

    close(((int*)cpipe)[0]);
    close(((int*)cpipe)[1]);

    free(cpipe);

    client_mutex_lock(&opened_pipes_lock);
    opened_pipes--;

    client_cond_signal(&max_opened_pipes);
    client_mutex_unlock(&opened_pipes_lock);

    client_mutex_lock(&clients_inside_lock);
    clients_inside--;
    client_cond_signal(&max_clients_inside);
    client_mutex_unlock(&clients_inside_lock);

}

void *client(void *arg) {
    int id              = ((struct client_args*)arg)->id;
    int p_time          = ((struct client_args*)arg)->purchase_time;
    int max_cash        = ((struct client_args*)arg)->max_cash;
    int products        = ((struct client_args*)arg)->products;

    unsigned int seed   = time(NULL) ^ id;

    struct timeval sm_start, sm_stop, sm_result;
    struct timeval queue_start, queue_stop, queue_result;
    
    long sm_time;       // total time inside supermarket in milliseconds
    long queue_time;    // total time inside queue in milliseconds 

    int queues_viewed   = 0;
    int cashier_open    = 0;    //aux variable
    int exit_queue      = 0;

    int cashier_id;

    int ca_2_cl[2];
    int response;
    int read_val;

    int* cleanup_arg = (int*)malloc(2*sizeof(int));

    free(arg);

    client_mutex_lock(&opened_pipes_lock);

    // waits if there are already 500 opened pipes
    while (opened_pipes == MAXOPIPES) {
        client_cond_wait(&max_opened_pipes, &opened_pipes_lock);
    }

    gettimeofday(&sm_start, NULL);
    
    if (pipe(ca_2_cl) != 0) {
        perror("error during pipe creation");
        pthread_exit((void*)EXIT_FAILURE);
    }

    opened_pipes += 1;

    client_mutex_unlock(&opened_pipes_lock);

    cleanup_arg[0] = ca_2_cl[0];
    cleanup_arg[1] = ca_2_cl[1];

    pthread_cleanup_push(client_cleanup, (void*)cleanup_arg);

    if (quit){ 
        pthread_exit((void*)EXIT_SUCCESS);
    }

    // sleeps for p_time ms (simulates client purchase period)
    purchase(p_time);

    if (quit){ 
        pthread_exit((void*)EXIT_SUCCESS);
    }

    if (products == 0) {
        
        if (fifo_tsqueue_push(&zero_products_q, (void*)&ca_2_cl[1], sizeof(ca_2_cl[1])) != 0) {
            perror("client error during queue push operation");
            pthread_exit((void*)EXIT_FAILURE);
        }

        gettimeofday(&sm_stop, NULL);
        timersub(&sm_stop, &sm_start, &sm_result);

        sm_time = (sm_result.tv_sec*1000000 + sm_result.tv_usec)/1000;

        read_val = read(ca_2_cl[0], &response, sizeof(int));
        if (read_val < 0) {
            perror("error during reading");
            pthread_exit((void*)EXIT_FAILURE);
        }

        client_mutex_lock(&dir_buff_lock);
        dir_buff.id = id;
        dir_buff.n_products = products;
        dir_buff.q_time = 0;
        dir_buff.q_viewed = 0;
        dir_buff.sm_time = sm_time;
        
        dir_buff_is_empty = 0;
        client_cond_signal(&dir_buff_empty);
        client_mutex_unlock(&dir_buff_lock);

        pthread_exit((void*)EXIT_SUCCESS);
    }
 
    gettimeofday(&queue_start, NULL);
    while (!exit_queue) {

        if (quit){ 
            pthread_exit((void*)EXIT_SUCCESS); 
        }

        // choses a random open cash
        while (!cashier_open) {
            cashier_id = rand_r(&seed) % (max_cash);
            
            client_mutex_lock(&state_lock[cashier_id]);
            cashier_open = state[cashier_id];
            client_mutex_unlock(&state_lock[cashier_id]);
        }

        if (quit){ 
            pthread_exit((void*)EXIT_SUCCESS);
        }

        if (fifo_tsqueue_push(&cash_q[cashier_id], (void*)&ca_2_cl[1], sizeof(ca_2_cl[1])) != 0) {
            perror("client error during queue push operation");
            close(ca_2_cl[0]);
            close(ca_2_cl[1]);
            pthread_exit((void*)EXIT_FAILURE);
        }

        queues_viewed++;

        // waits for his turn
        read_val = read(ca_2_cl[0], &response, sizeof(int));
        if (read_val < 0) {
            perror("error during reading");
            pthread_exit((void*)EXIT_FAILURE);
        }

        /**
         * if current cashier is closing
         * return to search for an open cashier
         */
        if (response == CLOSING) {
            cashier_open = 0;
            continue;
        }

        exit_queue = 1;
    }
    
    gettimeofday(&queue_stop, NULL);
    gettimeofday(&sm_stop, NULL);

    timersub(&queue_stop, &queue_start, &queue_result);
    timersub(&sm_stop, &sm_start, &sm_result);

    queue_time = (queue_result.tv_sec*1000000 + queue_result.tv_usec)/1000;
    sm_time = (sm_result.tv_sec*1000000 + sm_result.tv_usec)/1000;

    client_mutex_lock(&buff_lock[cashier_id]);

    buff[cashier_id].q_time = queue_time;
    buff[cashier_id].sm_time = sm_time;
    buff[cashier_id].id = id;
    buff[cashier_id].n_products = products;
    buff[cashier_id].q_viewed = queues_viewed;

    buff_is_empty[cashier_id] = 0;

    client_cond_signal(&buff_empty[cashier_id]);
    client_mutex_unlock(&buff_lock[cashier_id]);

    pthread_cleanup_pop(1);
    pthread_exit((void*)EXIT_SUCCESS);
}

void client_thread_init() {
    pthread_mutex_init(&opened_pipes_lock, NULL);
    pthread_cond_init(&max_opened_pipes, NULL);

    fifo_tsqueue_init(&zero_products_q);

    dir_buff_is_empty = 1;
    pthread_mutex_init(&dir_buff_lock, NULL);
    pthread_cond_init(&dir_buff_empty, NULL);

    opened_pipes = 0;

}

void client_thread_clear() {
    fifo_tsqueue_destroy(&zero_products_q);
}
