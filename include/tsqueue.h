#include <pthread.h>
#define ISFULL(queue) queue.tail+1 == queue.dim
#define ISEMPTY(queue) queue.tail == queue.head

typedef struct _int_fifo_tsqueue_t {
    int *buff;
    int dim;
    int head;
    int tail;
    pthread_mutex_t mutex;
    pthread_cond_t empty;
} int_fifo_tsqueue_t;

/**
 * params:
 *  - queue:    queue to initialize
 *  - n:        queue dimention
 */
int int_fifo_tsqueue_init(int_fifo_tsqueue_t *queue, int n);

/**
 * effect: doubles the queue size
 */
int int_fifo_tsqueue_realloc(int_fifo_tsqueue_t *queue);

/**
 * effects: frees queue allocation in heap
 */
int int_fifo_tsqueue_free(int_fifo_tsqueue_t *queue);

/**
 * returns 1 if queue is full 0 otherwise
 */
int in_fifo_tsqueue_isfull(int_fifo_tsqueue_t queue);

/**
 * returns 1 if queue is empty 0 otherwise
 */
int int_fifo_tsqueue_isempty(int_fifo_tsqueue_t queue);

/**
 * params:
 *  - queue: queue
 *  - str: element to push in queue
 * 
 * effects: adds element to queue
 */
int int_fifo_tsqueue_push(int_fifo_tsqueue_t *queue, int el);

/**
 * effects: remove element pointed by head
 */
int int_fifo_tsqueue_pop(int_fifo_tsqueue_t *queue);