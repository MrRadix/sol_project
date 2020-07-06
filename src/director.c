#include "director.h"
#include "client.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <string.h>
#include "colors.h"

#ifdef _DEBUG
#define DEBUG_PRINT(s, color) \
            printf ("\033%s", color); \
            printf s; \
            printf ("\033%s", COLOR_RESET); \
            fflush(stdout);
#endif

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

void director_cond_signal(pthread_cond_t *cond) {
    int err;

    if ((err = pthread_cond_signal(cond)) != 0) {
        errno = err;
        perror("director error unlock");
        pthread_exit((void*)EXIT_FAILURE);
    }
}

static void mask_signals(void) {
    sigset_t set;
    
    sigfillset(&set);
    if(pthread_sigmask(SIG_SETMASK, &set, NULL) != 0) {
        perror("client error during mask all sgnals");
        exit(EXIT_FAILURE);
    }
}

void zero_products_clients_handler() {
    void* channel;
    int message;

    while (!fifo_tsqueue_isempty(zero_products_q)) {

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

        fifo_tsqueue_push(&clients_info_q, (void*)&dir_buff, sizeof(dir_buff));

        dir_buff_is_empty = 1;
        director_mutex_unlock(&dir_buff_lock);

        director_mutex_lock(&n_clients_lock);
        n_total_clients += 1;
        director_mutex_unlock(&n_clients_lock);
    }
}

void *clients_handler(void *arg) {
    int c = ((struct clients_handler_args *)arg)->n_clients;
    int e = ((struct clients_handler_args *)arg)->n_clients_group;
    int k = ((struct clients_handler_args *)arg)->n_cashiers;
    int t = ((struct clients_handler_args *)arg)->client_max_time;
    int p = ((struct clients_handler_args *)arg)->n_max_products;
    int remaining_clients;

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
            director_cond_wait(&max_clients_inside, &clients_inside_lock);
        }
        director_mutex_unlock(&clients_inside_lock);

        if (closing || quit) {
            continue;
        }

        director_mutex_lock(&clients_inside_lock);

        /**
         * starts c-clients_inside clients
         */
        for (int i = clients_inside; i < c; i++) {
            args = (struct client_args *)malloc(sizeof(struct client_args));
            args->id = id;
            args->max_cash = k;
            args->purchase_time = (rand_r(&seed) % (t - 9)) + 10;

            args->products = rand_r(&seed) % (p+1); 

            pthread_create(&client_thread, NULL, client, (void*)args);
            pthread_detach(client_thread);

            clients_inside++;
            id++;
        }

        director_mutex_unlock(&clients_inside_lock);

        /**
         * handles clients with 0 products
         */
        zero_products_clients_handler();
    }

    // waits for ending of all clients
    director_mutex_lock(&clients_inside_lock);
    remaining_clients = clients_inside;
    director_mutex_unlock(&clients_inside_lock);
    while (remaining_clients > 0) {

        /**
         * handles clients with 0 products
         */
        zero_products_clients_handler();

        sched_yield();
        
        director_mutex_lock(&clients_inside_lock);
        remaining_clients = clients_inside;
        director_mutex_unlock(&clients_inside_lock);
    }

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
    int analytics_diff  = ((struct cashiers_handler_args *)arg)->analytics_time_diff;
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
    unsigned int seed = time(NULL) ^ s1 ^ s2;

    pthread_t *cashiers_thread  = (pthread_t *)malloc(k*sizeof(pthread_t));
    
    /**
     * auxiliary variable for checking if a cashier is open
     * used in cashier chosing whiles
     */
    int cashier_open = 0;
    int i;

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

        director_mutex_lock(&state_lock[i]);
        state[i] = 1;
        director_mutex_unlock(&state_lock[i]);

        pthread_create(&cashiers_thread[i], NULL, cashier, (void *)ca_args);
        
        DEBUG_PRINT(("[+] Opened cashier: %d\n", i), GREEN);
    }

    while (!quit) {

        director_mutex_lock(&clients_inside_lock);
        remaining_clients = clients_inside;
        director_mutex_unlock(&clients_inside_lock);

        DEBUG_PRINT(("[+] Remaining clients: %d\n", remaining_clients), CYAN);
        
        if (closing && remaining_clients == 0) {
            break;
        }

        /**
         * waits for data in analytics queue
         */
        director_mutex_lock(analytics_q.mutex);
        while(ISEMPTY(analytics_q) && !quit) {
            director_cond_wait(analytics_q.empty, analytics_q.mutex);
        }
        director_mutex_unlock(analytics_q.mutex);

        if (quit) {
            continue;
        }
    
        data = (struct analytics_data *)fifo_tsqueue_pop(&analytics_q);

        director_mutex_lock(&state_lock[i]);
        cashier_open = state[data->id];
        director_mutex_unlock(&state_lock[i]);


        if (!cashier_open) continue;

        if ((time(NULL) - data->timestamp) > analytics_diff) {
            free(data);
            continue;
        }
        
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

            // choses cashier to close
            for (i = 0; i < k; i++) {
                director_mutex_lock(&state_lock[i]);
                cashier_open = state[i];
                director_mutex_unlock(&state_lock[i]);
                
                if (one_client[i] && cashier_open) {
                    one_client[i] = 0;
                    break;
                }
            }

            director_mutex_lock(&state_lock[i]);
            state[i] = 0;
            director_mutex_unlock(&state_lock[i]);
            director_cond_signal(cash_q[i].empty);

            open_cashiers--;

            pthread_join(cashiers_thread[i], NULL);

            DEBUG_PRINT(("[+] Closed cashier: %d\n", i), RED);
        }

        /** 
         * opening cashier if there is a cashier
         * with number client in queue at least of s2
         */
        if (data->n_clients >= s2 && open_cashiers < k) {
            
            // choses cashier to open
            cashier_open = 1;
            i = 0;
            while (cashier_open && i < k) {
                director_mutex_lock(&state_lock[i]);
                cashier_open = state[i];
                director_mutex_unlock(&state_lock[i]);
                i++;
            }
            i--;

            director_mutex_lock(&state_lock[i]);
            state[i] = 1;
            director_mutex_unlock(&state_lock[i]);

            open_cashiers++;
            
            ca_args = (struct cashier_args*)malloc(sizeof(struct cashier_args));
            ca_args->id = i;
            ca_args->time_per_client = (rand_r(&seed) % (MAX_KTIME - MIN_KTIME + 1)) + MIN_KTIME;
            ca_args->analytics_time = analytics_time;
            ca_args->prod_time = prod_time;

            pthread_create(&cashiers_thread[i], NULL, cashier, (void*)ca_args);
            
            DEBUG_PRINT(("[+] Opened cashier: %d\n", i), GREEN);
        }

        free(data);
    }
    // quit detected or closing and no client inside

    // signaling all waiting cashiers
    for (i = 0; i < k; i++) {
        director_mutex_lock(&state_lock[i]);
        cashier_open = state[i];
        director_mutex_unlock(&state_lock[i]);

        if (cashier_open) {
            director_mutex_lock(&state_lock[i]);
            state[i] = 0;
            director_mutex_unlock(&state_lock[i]);
            director_cond_signal(cash_q[i].empty);

            pthread_join(cashiers_thread[i], NULL);
            
            DEBUG_PRINT(("[+] Closed cashier: %d\n", i), RED);
        }
    }

    free(cashiers_thread);
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
    int analytics_diff  = ((struct director_args *)arg)->analytics_time_diff;
    int s1              = ((struct director_args *)arg)->s1;
    int s2              = ((struct director_args *)arg)->s2;
    char *log_file_name  = ((struct director_args *)arg)->log_file_name;

    char *string = (char*)malloc(1024*sizeof(char));
    FILE *log_file;
    client_info *c_info;
    int tmp;

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
    ca_h_args->analytics_time_diff = analytics_diff;
    ca_h_args->s1 = s1;
    ca_h_args->s2 = s2;

    pthread_create(&ca_h_thread, NULL, cashiers_handler, (void*)ca_h_args);

    pthread_join(cl_h_thread, NULL);
    pthread_join(ca_h_thread, NULL);

    free(ca_h_args);
    free(cl_h_args);

    DEBUG_PRINT(("[+] Creating log\n"), BLUE);

    /**
     * writing log file
     */
    log_file = fopen(log_file_name, "w");

    fwrite(
        "# client id supermarket_time queue_time queue_viewed n_products\n",
        sizeof(char), 64, log_file
    );

    fwrite(
        "# cash id n_products n_clients n_closings time_forall_clients time_forall_openings\n\n",
        sizeof(char), 84, log_file
    );

    // writing total clients
    sprintf(string, "tot_clients %d\n", n_total_clients);
    fwrite(string, sizeof(char), strlen(string), log_file);

    // writing total products
    sprintf(string, "tot_products %d\n", n_total_products);
    fwrite(string, sizeof(char), strlen(string), log_file);

    // writing clients informations
    while ((c_info = (client_info*)fifo_tsqueue_pop(&clients_info_q)) != NULL) {
        
        sprintf(
            string, 
            "client %d %ld %ld %d %d\n", 
            c_info->id, c_info->sm_time, c_info->q_time, c_info->q_viewed, c_info->n_products
        );
        fwrite(string, sizeof(char), strlen(string), log_file);

        free(c_info);
    }

    // writing cashiers informations
    for (int i = 0; i<k; i++) {
        sprintf(
            string, 
            "cash %d %d %d %d ", 
            i, cashiers_info[i].n_products, cashiers_info[i].n_clients, cashiers_info[i].n_closings
        );
        fwrite(string, sizeof(char), strlen(string), log_file);


        
        tmp = next(cashiers_info[i].time_per_client);
        if (tmp >= 0) {
            sprintf(string, "%d", tmp);
            fwrite(string, sizeof(char), strlen(string), log_file);
        } else {
            fwrite("null", sizeof(char), 4, log_file);
        }
        tmp = next(NULL);
        
        while(tmp >= 0) {
            sprintf(string, ",%d", tmp);
            fwrite(string, sizeof(char), strlen(string), log_file);
            tmp = next(NULL);
        }
        fwrite(" ", sizeof(char), 1, log_file);



        tmp = next(cashiers_info[i].time_per_operiod);
        if (tmp >= 0) {
            sprintf(string, "%d", tmp);
            fwrite(string, sizeof(char), strlen(string), log_file);
        } else {
            fwrite("null", sizeof(char), 4, log_file);
        }
        tmp = next(NULL);
        
        while(tmp >= 0) {
            sprintf(string, ",%d", tmp);
            fwrite(string, sizeof(char), strlen(string), log_file);
            tmp = next(NULL);
        }
        fwrite("\n", sizeof(char), 1, log_file);

    }

    fclose(log_file);
    free(string);

    cashier_thread_clear(k,c);
    client_thread_clear();

    pthread_exit((void*)EXIT_SUCCESS);
}
