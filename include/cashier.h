#include "tsqueue.h"
#include "config.h"
#include "customer.h"
#define NEXT "next"
#define CLOSING "closing"

/**
 * index of array correspond to cashier id
 * for each cell:
 *  1 stands for cash opened
 *  0 stands for cash closed
 */
extern int state[K];

extern pthread_mutex_t state_lock;

extern int_fifo_tsqueue_t cash_q[K];
extern customer_data buff[1];

void cashier(void *arg);
void analytics(void *arg);