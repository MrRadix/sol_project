#include <pthread.h>
#define ISEMPTY(queue) queue.head == NULL

typedef struct _ts_queue_el {
    void *el;
    struct _ts_queue_el *next;
} ts_queue_el;

typedef struct _fifo_tsqueue_t {
    ts_queue_el *head;
    ts_queue_el *tail;
    int n_elements;
    pthread_mutex_t *mutex;
    pthread_cond_t *empty;
} fifo_tsqueue_t;

/**
 * initializes queue
 */
int fifo_tsqueue_init(fifo_tsqueue_t *queue);

/**
 * frees all allocated data and destroys mutex and condition variable
 */
int fifo_tsqueue_destroy(fifo_tsqueue_t *queue);

/**
 * returns 1 if queue is empty 0 otherwise
 */
int fifo_tsqueue_isempty(fifo_tsqueue_t queue);

/**
 * adds element of size in bytes size to queue
 */
int fifo_tsqueue_push(fifo_tsqueue_t *queue, void *el, size_t size);

/**
 * effects: remove element pointed by head
 */
void *fifo_tsqueue_pop(fifo_tsqueue_t *queue);

/**
 * effects: returns number of element in queue
 */
int fifo_tsqueue_n_items(fifo_tsqueue_t queue);
