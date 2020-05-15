#include "client.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>

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

void client_quit(int cpipe[2], void *arg, int id) {

    free(arg);
    close(cpipe[0]);
    close(cpipe[1]);

    client_mutex_lock(&opened_pipes_lock, arg);
    opened_pipes--;

    client_cond_signal(&max_opened_pipes, arg);
    client_mutex_unlock(&opened_pipes_lock, arg);
    //fprintf(stderr, "client %d finished\n", id);
    pthread_exit((void*)EXIT_SUCCESS);

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

    time_t enter_queue_time;

    int cashier_id;

    int ca_2_cl[2];
    int response;
    int read_val;

    gettimeofday(&sm_start, NULL);

    //fprintf(stderr, "started client %d\n", id);

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
        client_mutex_lock(&clients_inside_lock, arg);
        clients_inside--;
        client_cond_signal(&max_clients_inside, arg);
        client_mutex_unlock(&clients_inside_lock, arg);
        
        client_quit(ca_2_cl, arg, id);
    }

    //fprintf(stderr, "client %d starting purchases\n", id);

    // sleeps for p_time ms (simulates client purchase period)
    purchase(p_time);

    if (quit){ 
        client_mutex_lock(&clients_inside_lock, arg);
        clients_inside--;
        client_cond_signal(&max_clients_inside, arg);
        client_mutex_unlock(&clients_inside_lock, arg);
        
        client_quit(ca_2_cl, arg, id);
    }

    if (products == 0) {
        
        if (fifo_tsqueue_push(&zero_products_q, (void*)&ca_2_cl[1], sizeof(ca_2_cl[1])) != 0) {
            perror("client error during queue push operation");
            free(arg);
            close(ca_2_cl[0]);
            close(ca_2_cl[1]);
            pthread_exit((void*)EXIT_FAILURE);
        }

        gettimeofday(&sm_stop, NULL);
        timersub(&sm_stop, &sm_start, &sm_result);

        sm_time = (sm_result.tv_sec*1000000 + sm_result.tv_usec)/1000;

        client_mutex_lock(&clients_inside_lock, arg);
        clients_inside--;
        client_cond_signal(&max_clients_inside, arg);
        client_mutex_unlock(&clients_inside_lock, arg);

        read_val = read(ca_2_cl[0], &response, sizeof(int));
        if (read_val < 0) {
            perror("error during reading");
            free(arg);
            pthread_exit((void*)EXIT_FAILURE);
        }

        client_mutex_lock(&dir_buff_lock, arg);
        dir_buff->id = id;
        dir_buff->n_products = products;
        dir_buff->q_time = 0;
        dir_buff->q_viewed = 0;
        dir_buff->sm_time = sm_time;
        
        dir_buff_is_empty = 0;
        client_cond_signal(&dir_buff_empty, arg);
        client_mutex_unlock(&dir_buff_lock, arg);

        client_quit(ca_2_cl, arg, id);
    }
    

    //fprintf(stderr, "client %d finished purchases\n", id);

    /**
     * TODO: client with 0 product requests exit to director
     */ 
    //enter_queue_time = time(NULL);
    gettimeofday(&queue_start, NULL);
    while (!exit_queue) {

        if (quit){ 
            client_mutex_lock(&clients_inside_lock, arg);
            clients_inside--;
            client_cond_signal(&max_clients_inside, arg);
            client_mutex_unlock(&clients_inside_lock, arg);
        
            client_quit(ca_2_cl, arg, id);  
        }

        // choses a random open cash
        while (!cashier_open) {
            cashier_id = rand_r(&seed) % (max_cash);
            
            fprintf(stderr, "client %d trying cahsier %d\n", id, cashier_id);
            client_mutex_lock(&state_lock[cashier_id], arg);
            cashier_open = state[cashier_id];
            client_mutex_unlock(&state_lock[cashier_id], arg);
        }
        //fprintf(stderr, "client %d entered in queue of cashier %d\n", id, cashier_id);

        if (quit){ 
            client_mutex_lock(&clients_inside_lock, arg);
            clients_inside--;
            client_cond_signal(&max_clients_inside, arg);
            client_mutex_unlock(&clients_inside_lock, arg);
        
            client_quit(ca_2_cl, arg, id);
        }

        //fprintf(stderr, "client %d pushing in %d queue pfd: %d\n", id, cashier_id, ca_2_cl[1]);
        if (fifo_tsqueue_push(&cash_q[cashier_id], (void*)&ca_2_cl[1], sizeof(ca_2_cl[1])) != 0) {
            perror("client error during queue push operation");
            free(arg);
            close(ca_2_cl[0]);
            close(ca_2_cl[1]);
            pthread_exit((void*)EXIT_FAILURE);
        }

        queues_viewed++;

        // waits for his turn
        read_val = read(ca_2_cl[0], &response, sizeof(int));
        if (read_val < 0) {
            perror("error during reading");
            free(arg);
            pthread_exit((void*)EXIT_FAILURE);
        }

        /**
         * if current cashier is closing
         * return to search for an open cashier
         */
        if (response == CLOSING) {
            cashier_open = 0;
            fprintf(stderr, "client %d received closing message from cashier %d\n", id, cashier_id);
            continue;
        }

        exit_queue = 1;
    }

    fprintf(stderr, "----------- client %d sending all data to cashier %d\n", id, cashier_id);
    
    gettimeofday(&queue_stop, NULL);
    gettimeofday(&sm_stop, NULL);

    timersub(&sm_stop, &sm_start, &sm_result);

    timersub(&queue_stop, &queue_start, &queue_result);
    timersub(&sm_stop, &sm_start, &sm_result);

    queue_time = (queue_result.tv_sec*1000000 + queue_result.tv_usec)/1000;
    sm_time = (sm_result.tv_sec*1000000 + sm_result.tv_usec)/1000;

    client_mutex_lock(&buff_lock[cashier_id], arg);

    buff[cashier_id].q_time = queue_time;
    buff[cashier_id].sm_time = sm_time;
    buff[cashier_id].id = id;
    buff[cashier_id].n_products = products;
    buff[cashier_id].q_viewed = queues_viewed;

    buff_is_empty[cashier_id] = 0;

    client_cond_signal(&buff_empty[cashier_id], arg);
    client_mutex_unlock(&buff_lock[cashier_id], arg);

    client_mutex_lock(&clients_inside_lock, arg);
    clients_inside--;
    client_cond_signal(&max_clients_inside, arg);
    client_mutex_unlock(&clients_inside_lock, arg);

    client_quit(ca_2_cl, arg, id);
}

void client_thread_init() {
    pthread_mutex_init(&clients_inside_lock, NULL);
    pthread_mutex_init(&opened_pipes_lock, NULL);
    pthread_cond_init(&max_opened_pipes, NULL);
    pthread_cond_init(&max_clients_inside, NULL);

    fifo_tsqueue_init(&zero_products_q);

    dir_buff = (client_data *)malloc(sizeof(client_data));
    dir_buff_is_empty = 1;
    pthread_mutex_init(&dir_buff_lock, NULL);
    pthread_cond_init(&dir_buff_empty, NULL);

    clients_inside = 0;
    opened_pipes = 0;

}
