#include "client.h"
#include "cashier.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

void client_mutex_lock(pthread_mutex_t *mtx, void *p) {
    int err;

    if ((err = pthread_mutex_lock(mtx)) != 0) {
        errno = err;
        perror("lock");
        free(p);
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void client_mutex_unlock(pthread_mutex_t *mtx, void *p) {
    int err;

    if ((err = pthread_mutex_unlock(mtx)) != 0) {
        errno = err;
        perror("lock");
        free(p);
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void client_cond_signal(pthread_cond_t *cond, void *p) {
    int err;

    if ((err = pthread_cond_signal(cond)) != 0) {
        errno = err;
        perror("lock");
        free(p);
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void purchase(int nsec) {
    struct timespec *res = (struct timespec *)malloc(sizeof(struct timespec));
    struct timespec *rem = (struct timespec *)malloc(sizeof(struct timespec));

    res->tv_sec = 0;
    res->tv_nsec = nsec;

    // purchasing products...
    if (nanosleep(res, rem) == -1) {
        nanosleep(rem, NULL);
    }

    free(res);
    free(rem);
}

void *client(void *arg) {
    time_t enter_time   = time(NULL);

    int id              = ((struct client_args*)arg)->id;
    int p_time          = ((struct client_args*)arg)->p_time;
    int max_cash        = ((struct client_args*)arg)->max_cash;
    int products        = ((struct client_args*)arg)->products;

    unsigned int seed   = id;

    int queues_viewed   = 0;
    int cashier_open    = 0;
    int exit_queue      = 0;

    client_data data;

    time_t enter_queue_time;

    int cashier_id;

    int ca_2_cl[2];
    int response;
    int read_val;

    fprintf(stderr, "started client %d\n", id);

    if (pipe(ca_2_cl) != 0) {
        perror("error during pipe creation");
        free(arg);
        pthread_exit((void*)EXIT_FAILURE);
    }

    if (quit){ 
        free(arg);
        pthread_exit((void*)EXIT_SUCCESS);
    }

    // sleeps for p_time ms (simulates client purchase period)
    purchase(p_time);

    if (quit){ 
        free(arg);
        pthread_exit((void*)EXIT_SUCCESS);
    }

    enter_queue_time = time(NULL);
    while (!exit_queue) {

        // choses a random open cash
        while (!cashier_open) {
            cashier_id = rand_r(&seed) % (max_cash);

            client_mutex_lock(&state_lock[cashier_id], arg);
            cashier_open = state[cashier_id];
            client_mutex_unlock(&state_lock[cashier_id], arg);
        }

        if (quit){ 
            free(arg);
            pthread_exit((void*)EXIT_SUCCESS);
        }

        int_fifo_tsqueue_push(&cash_q[cashier_id], ca_2_cl[1]);

        if (quit){ 
            free(arg);
            pthread_exit((void*)EXIT_SUCCESS);
        }

        fprintf(stderr, "client %d entered in queue of cashier %d\n", id, cashier_id);

        queues_viewed++;

        // waits for his turn
        while(1) {
            read_val = read(ca_2_cl[0], &response, sizeof(int));

            /**
             * if read was blocked by a signal restart waiting
             * for something to read 
             * else quit
             */
            if (errno != 0) {
                if (read_val != EINTR) {
                    perror("error during read");
                    free(arg);
                    pthread_exit((void*)EXIT_FAILURE);
                } else {
                    continue;
                }
            /**
             * if there was no error exit from loop
             */
            } else {
                break;
            }
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
    fprintf(stderr, "client %d sending data to cashier %d...\n", id, cashier_id);
    
    data.q_time = enter_queue_time-time(NULL);
    data.id = id;
    data.n_products = products;
    data.q_viewed = queues_viewed;
    data.sm_time = enter_time-time(NULL);

    client_mutex_lock(&buff_lock[cashier_id], arg);

    buff[cashier_id].id = data.id;
    buff[cashier_id].q_time = data.q_time;
    buff[cashier_id].n_products = data.n_products;
    buff[cashier_id].q_viewed = data.q_viewed;
    buff[cashier_id].sm_time = data.sm_time;

    buff_is_empty[cashier_id] = 0;

    client_cond_signal(&buff_empty[cashier_id], arg);
    client_mutex_unlock(&buff_lock[cashier_id], arg);

    free(arg);
    pthread_exit((void*)EXIT_SUCCESS);
}