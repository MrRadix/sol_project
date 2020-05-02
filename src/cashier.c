#include "cashier.h"

state_lock = PTHREAD_MUTEX_INITIALIZER;
cash_empty = PTHREAD_COND_INITIALIZER;

void analytics(void *arg) {}

void cashier(void *arg) {
    int id      = ((int*)arg)[0];
    int time    = ((int*)arg)[1];
    int open    = 1;
    int empty;
    int cpipe;

    /**
     * TODO: start analytics thread
     */

    /**
     * initializes cash queue
     */
    fifo_tsqueue_init(&cash_q[id], C);

    while (open) {

        /**
         * waits for a customer that puts himself in the queue
         */
        pthread_mutex_lock(cash_q[id].mutex);
        while (ISEMPTY(cash_q[id])) {
    
            pthread_mutex_lock(&state_lock);
            open = state[id];
            pthread_mutex_unlock(&state_lock);

            /**
             * checks if director decided to close cash
             */
            if (!open) {
                break;
            }

            pthread_cond_wait(cash_q[id].empty, cash_q[id].mutex);
        }
        pthread_mutex_unlock(cash_q[id].mutex);

        if (!open) {  
            break;
        }

        cpipe = int_fifo_tsqueue_pop(&cash_q[id]);
        write(cpipe, NEXT, 5);

        /**
         * TODO: comunicating with customer
         */

        pthread_mutex_lock(&state_lock);
        open = state[id];
        pthread_mutex_unlock(&state_lock);
    }

    /**
     * TODO: sending of close message to enqueued customers
     */
}