#include "tsqueue.h"
#include <time.h>
#include <signal.h>
#define NEXT 1
#define CLOSING 0

/*
typedef struct _per_client_info {

} per_client_info;

typedef struct _cashier_data {
    int n_clients;
    
} cashier_data;
*/

struct cashier_args {
    int id;
    int time;
    int prod_time;
};

typedef struct _client_data {
    int id;
    int n_products;     // number of products purchased
    time_t sm_time;     // time spent inside supermarket
    time_t q_time;      // time spent in queue
    int q_viewed;       // number of queue viewed
} client_data;

/**
 * each cell of array correspond to a cashier id
 * for each cell:
 *  1 stands for cash opened
 *  0 stands for cash closed
 */
int *state;

/**
 * lock for each cashier to modify safely the state
 * state[i] lock is initialized by director if starts cashier i
 * state[i] lock is destroyed when cashier i closes
 */
pthread_mutex_t *state_lock;


/**
 * comunication buffer between cashier and customer
 */
client_data *buff;

int *buff_is_empty;
pthread_mutex_t *buff_lock;
pthread_cond_t *buff_empty;

/**
 * for each cashier: queue where customers waits
 */
int_fifo_tsqueue_t *cash_q;

/**
 * if quit = 1 clients quits immediatly for supermarket closing
 * else if quit = 0 supermarket works normally
 */
volatile sig_atomic_t quit;

/**
 * if closing = 1 no more clients enters 
 * and will be served remainig clients inside supermarket
 * else if closing = 0 supermarket works normally
 */
volatile sig_atomic_t closing;

void *cashier(void *arg);
void cashier_thread_init(int n_cashiers, int n_clients);