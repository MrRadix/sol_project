#include <pthread.h>
//#define ISFULL(queue) queue.tail+1 == queue.dim
//#define ISEMPTY(queue) queue.tail == queue.head
#define ISEMPTY(queue) queue.head == NULL

struct analytics_args {
    int intervall;
    int id;
};

typedef struct _ts_queue_el {
    void *el;
    struct _ts_queue_el *next;
} ts_queue_el;

typedef struct _fifo_tsqueue_t {
    ts_queue_el *head;
    ts_queue_el *tail;
    int n_elements;
    pthread_mutex_t mutex;
    pthread_cond_t empty;
} fifo_tsqueue_t;


/**
 * params:
 *  - queue:    queue to initialize
 *  - n:        queue dimention
 */
int fifo_tsqueue_init(fifo_tsqueue_t *queue);

/**
 * effect: doubles the queue size
 */
int fifo_tsqueue_realloc(fifo_tsqueue_t *queue);

/**
 * effects: frees queue allocation in heap
 */
int fifo_tsqueue_free(fifo_tsqueue_t *queue);

/**
 * returns 1 if queue is full 0 otherwise
 */
int fifo_tsqueue_isfull(fifo_tsqueue_t queue);

/**
 * returns 1 if queue is empty 0 otherwise
 */
int fifo_tsqueue_isempty(fifo_tsqueue_t queue);

/**
 * params:
 *  - queue: queue
 *  - str: element to push in queue
 * 
 * effects: adds element to queue
 */
int fifo_tsqueue_push(fifo_tsqueue_t *queue, void *el, size_t size);

/**
 * effects: remove element pointed by head
 */
void *fifo_tsqueue_pop(fifo_tsqueue_t *queue);

/**
 * effects: returns number of element in queue
 */
int fifo_tsqueue_n_items(fifo_tsqueue_t *queue);