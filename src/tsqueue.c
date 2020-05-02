#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#include "tsqueue.h"

void int_fifo_tsqueue_init(int_fifo_tsqueue_t *queue, int n) {
    queue->buff = (int*)malloc(n*sizeof(int));

    queue->dim = n;
    queue->head = 0;
    queue->tail = 0;
    pthread_mutex_init(queue->mutex, NULL);
    pthread_cond_init(queue->empty, NULL);
}

void int_fifo_tsqueue_realloc(int_fifo_tsqueue_t *queue) {
    
    pthread_mutex_lock(queue->mutex);

    int *p = realloc(queue->buff, queue->dim*2*sizeof(int));

    if (!p) {
        perror("error during realloc");
        pthread_mutex_unlock(queue->mutex);
        
        return;
    } else {
        queue->buff = p;
    }

    queue->dim = queue->dim*2;

    pthread_mutex_unlock(queue->mutex);
}

void int_fifo_tsqueue_free(int_fifo_tsqueue_t *queue) {
    pthread_mutex_lock(queue->mutex);

    free(queue->buff);

    pthread_mutex_unlock(queue->mutex);

    pthread_mutex_destroy(queue->mutex);
}

int int_fifo_tsqueue_isfull(int_fifo_tsqueue_t queue) {
    
    pthread_mutex_lock(queue.mutex);
    int ret = (queue.tail+1) % queue.dim == queue.head;
    pthread_mutex_unlock(queue.mutex);

    return ret;
}

int int_fifo_tsqueue_isempty(int_fifo_tsqueue_t queue) {

    pthread_mutex_lock(queue.mutex);
    int ret = queue.tail == queue.head;
    pthread_mutex_unlock(queue.mutex);

    return ret;
}

void int_fifo_tsqueue_push(int_fifo_tsqueue_t *queue, int el) {

    pthread_mutex_lock(queue->mutex);

    // checks if queue is full
    if (ISFULL((*queue))) {
        int_fifo_tsqueue_realloc(queue);
    }

    queue->buff[queue->tail] = el;

    queue->tail = (queue->tail + 1) % queue->dim;

    pthread_mutex_unlock(queue->mutex);
}

int int_fifo_tsqueue_pop(int_fifo_tsqueue_t *queue) {
    int ret;

    pthread_mutex_lock(queue->mutex);

    // checks if queue is empty
    if (ISEMPTY((*queue))) {
        pthread_mutex_unlock(queue->mutex);

        return -1;
    }

    ret = queue->buff[queue->head];
    queue->head = (queue->head + 1) % queue->dim;

    pthread_mutex_unlock(queue->mutex);

    return ret;
}