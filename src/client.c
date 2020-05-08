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
        perror("client mutex lock error");
        free(p);
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void client_mutex_unlock(pthread_mutex_t *mtx, void *p) {
    int err;

    if ((err = pthread_mutex_unlock(mtx)) != 0) {
        errno = err;
        perror("client mutex unlock error");
        free(p);
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void client_cond_signal(pthread_cond_t *cond, void *p) {
    int err;

    if ((err = pthread_cond_signal(cond)) != 0) {
        errno = err;
        perror("client cond signal error");
        free(p);
        pthread_exit((void*)EXIT_FAILURE);
    }
}

void client_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex, void *p) {
    int err;

    if ((err = pthread_cond_wait(cond, mutex)) != 0) {
        errno = err;
        perror("client cond wait error");
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
    int p_time          = ((struct client_args*)arg)->purchase_time;
    int max_cash        = ((struct client_args*)arg)->max_cash;
    int products        = ((struct client_args*)arg)->products;

    unsigned int seed   = time(NULL) ^ id;

    int queues_viewed   = 0;
    int cashier_open    = 0;
    int exit_queue      = 0;

    client_data data;

    time_t enter_queue_time;

    int cashier_id;

    int ca_2_cl[2];
    int response;
    int read_val;

    /**
    client_mutex_lock(&clients_inside_lock, arg);
    clients_inside += 1;
    fprintf(stderr, "increasing clients number to %d\n", clients_inside);
    client_mutex_unlock(&clients_inside_lock, arg);
    */

    client_mutex_lock(&opened_pipes_lock, arg);

    // waits if there are already 500 opened pipes
    while (opened_pipes == MAXOPIPES) {
        pthread_cond_wait(&max_opened_pipes, &opened_pipes_lock);
    }
    
    if (pipe(ca_2_cl) != 0) {
        perror("error during pipe creation");
        free(arg);
        pthread_exit((void*)EXIT_FAILURE);
    }

    opened_pipes += 1;

    client_mutex_unlock(&opened_pipes_lock, arg);

    if (quit){ 
        free(arg);
        pthread_exit((void*)EXIT_SUCCESS);
    }

    // sleeps for p_time ms (simulates client purchase period)
    purchase(p_time);

    if (quit){ 
        free(arg);
        close(ca_2_cl[0]);
        close(ca_2_cl[1]);
        pthread_exit((void*)EXIT_SUCCESS);
    }

    /**
     * TODO: client with 0 product requests exit to director
     */ 
    enter_queue_time = time(NULL);
    while (!exit_queue) {

        // choses a random open cash
        while (!cashier_open) {
            cashier_id = rand_r(&seed) % (max_cash);

            client_mutex_lock(&state_lock[cashier_id], arg);
            cashier_open = state[cashier_id];
            client_mutex_unlock(&state_lock[cashier_id], arg);
        }
        fprintf(stderr, "client %d entered in queue of cashier %d\n", id, cashier_id);

        if (quit){ 
            free(arg);
            close(ca_2_cl[0]);
            close(ca_2_cl[1]);
            pthread_exit((void*)EXIT_SUCCESS);
        }

        if (int_fifo_tsqueue_push(&(cash_q[cashier_id]), ca_2_cl[1]) != 0) {
            perror("client error during queue push operation");
            free(arg);
            close(ca_2_cl[0]);
            close(ca_2_cl[1]);
            pthread_exit((void*)EXIT_FAILURE);
        }

        if (quit){ 
            free(arg);
            close(ca_2_cl[0]);
            close(ca_2_cl[1]);
            pthread_exit((void*)EXIT_SUCCESS);
        }

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
                    close(ca_2_cl[0]);
                    close(ca_2_cl[1]);
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

    fprintf(stderr, "----------- client %d sending all data to cashier %d\n", id, cashier_id);
    
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

    client_mutex_lock(&clients_inside_lock, arg);
    clients_inside--;
    pthread_cond_broadcast(&max_clients_inside);
    fprintf(stderr, "decreasing clients number to %d\n", clients_inside);
    client_mutex_unlock(&clients_inside_lock, arg);

    free(arg);
    close(ca_2_cl[0]);
    close(ca_2_cl[1]);

    client_mutex_lock(&opened_pipes_lock, arg);
    opened_pipes--;

    client_cond_signal(&max_opened_pipes, arg);
    client_mutex_unlock(&opened_pipes_lock, arg);
    fprintf(stderr, "client %d finished\n", id);
    pthread_exit((void*)EXIT_SUCCESS);
}

void client_thread_init() {
    pthread_mutex_init(&clients_inside_lock, NULL);
    pthread_mutex_init(&opened_pipes_lock, NULL);
    pthread_cond_init(&max_opened_pipes, NULL);
    pthread_cond_init(&max_clients_inside, NULL);

    clients_inside = 0;
    opened_pipes = 0;

}
