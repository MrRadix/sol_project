#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "tsqueue.h"

int fifo_tsqueue_init(fifo_tsqueue_t *queue) {

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        perror("tsqueue error during mutex initialization");
        return -1;
    }

    if (pthread_cond_init(&queue->empty, NULL) != 0) {
        perror("tsqueue error during condition variable initialization");
        return -1;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->n_elements = 0;

    return EXIT_SUCCESS;
}

int fifo_tsqueue_isempty(fifo_tsqueue_t queue) {
    
    if (pthread_mutex_lock(&queue.mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }
    
    int ret = queue.head == NULL;
    
    if (pthread_mutex_unlock(&queue.mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return -1;
    }

    return ret;
}

int fifo_tsqueue_push(fifo_tsqueue_t *queue, void *el, size_t size) {

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }

    if (ISEMPTY((*queue))) {
        queue->head = (ts_queue_el *)malloc(sizeof(ts_queue_el));
        queue->tail = (ts_queue_el *)malloc(sizeof(ts_queue_el));

        queue->head->el = malloc(size);
        memcpy(queue->head->el, el, size);
        queue->head->next = NULL;

        queue->tail = queue->head;

    } else {
        queue->tail->next = (ts_queue_el *)malloc(sizeof(ts_queue_el));
        queue->tail = queue->tail->next;

        queue->tail->el = malloc(size);
        memcpy(queue->tail->el, el, size);
        queue->tail->next = NULL;
    }

    queue->n_elements += 1;

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return -1;
    }

    if (pthread_cond_signal(&queue->empty) != 0) {
        perror("tsqueue error during signaling");
        return -1;
    }

    return EXIT_SUCCESS;
}

void *fifo_tsqueue_pop(fifo_tsqueue_t *queue) {
    void *ret;
    ts_queue_el *tmp;

    fprintf(stderr, "inside pop function %d\n", (queue->head == NULL));
    // checks if queue is empty
    if (fifo_tsqueue_isempty(*queue)) {
        fprintf(stderr, "inside pop function if %d\n", (queue->head == NULL));

        return NULL;
    }

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return (void*)-1;
    }

    ret = queue->head->el;
    tmp = queue->head;
    queue->head = queue->head->next;

    queue->n_elements -= 1;

    free(tmp);

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return (void*)-1;
    }

    return ret;
}

int fifo_tsqueue_n_items(fifo_tsqueue_t queue) {
    int n_items;

    if (pthread_mutex_lock(&queue.mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }

    n_items = queue.n_elements;

    if (pthread_mutex_unlock(&queue.mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }

    return n_items;
}