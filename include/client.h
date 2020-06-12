#include <time.h>
#include <pthread.h>
#include "cashier.h"
#define MAXOPIPES 500

struct client_args {
    int id;                 // customer id
    time_t purchase_time;   // time for purchases in ms
    int max_cash;           // max number of cashiers
    int products;           // products number to buy
};

void *client(void *arg);

/**
 * buffer of 1 position where clients store 
 * their informations for director
 */
client_info dir_buff;
int dir_buff_is_empty;
pthread_mutex_t dir_buff_lock;
pthread_cond_t dir_buff_empty;

/**
 * for bounding number of pipes in the same time
 * the limit is specified by MAXOPIPES macro
 */
pthread_cond_t max_opened_pipes;
pthread_mutex_t opened_pipes_lock;
int opened_pipes;

fifo_tsqueue_t zero_products_q;

void client_thread_init();
void client_thread_clear();
