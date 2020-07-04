#include "tsqueue.h"
#include "linkedlist.h"
#include <time.h>
#include <signal.h>
#define NEXT 1
#define CLOSING 0

struct analytics_data {
    int n_clients;
    time_t timestamp;
    int id;
};

struct analytics_args {
    int id;
    int msec;
};

struct cashier_args {
    int id;
    int time_per_client;
    int analytics_time;
    int prod_time;
};


/**
 * cashier info to write in log file
 */
struct cashier_info {
    int n_clients;              // number of served clients
    int n_products;
    int n_closings;             // number of closings
    int_list_node *time_per_operiod;   // time for all opening periods
    int_list_node *time_per_client;    // time of service of every client served
};

/**
 * client info to write in log file
 */
typedef struct _client_info {
    int id;
    int n_products;     // number of products purchased
    long sm_time;     // time spent inside supermarket in milliseconds
    long q_time;      // time spent in queue in milliseconds
    int q_viewed;       // number of queue viewed
} client_info;

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
client_info *buff;

int *buff_is_empty;
pthread_mutex_t *buff_lock;
pthread_cond_t *buff_empty;

/**
 * for each cashier: queue where customers waits
 */
fifo_tsqueue_t *cash_q;


/**
 * queue where cashiers put analytics data
 * (n_clients waiting) for director
 */
fifo_tsqueue_t analytics_q;


/**
 * queue where clients info are stored by cashiers, or by clients with 
 * 0 products, for director
 */
fifo_tsqueue_t clients_info_q;

/**
 * array of cashiers info 
 */
struct cashier_info *cashiers_info;


/**
 * mutex initialized by director
 */
pthread_mutex_t clients_inside_lock;
pthread_cond_t max_clients_inside;
int clients_inside;


/**
 * count total products purchased
 */
int n_total_products;
pthread_mutex_t n_prod_lock;

int n_total_clients;
pthread_mutex_t n_clients_lock;


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

void mask_signals(void);

void *cashier(void *arg);
void cashier_thread_init(int n_cashiers, int n_clients);
void cashier_thread_clear(int n_cashiers, int n_clients);