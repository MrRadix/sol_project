#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "tsqueue.h"

int int_fifo_tsqueue_init(int_fifo_tsqueue_t *queue, int n) {

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

    return EXIT_SUCCESS;
}

int int_fifo_tsqueue_isempty(int_fifo_tsqueue_t queue) {
    
    if (pthread_mutex_lock(&queue.mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }
    
    int ret = ISEMPTY(queue);
    
    if (pthread_mutex_unlock(&queue.mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return -1;
    }

    return ret;
}

int int_fifo_tsqueue_push(int_fifo_tsqueue_t *queue, int el) {

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }

    if (ISEMPTY((*queue))) {
        fprintf(stderr, "inside push function with queue empty\n");
        queue->head = (ts_queue_el *)malloc(sizeof(ts_queue_el));
        queue->tail = (ts_queue_el *)malloc(sizeof(ts_queue_el));

        queue->head->el = el;
        queue->head->next = NULL;

        queue->tail = queue->head;

    } else {
        fprintf(stderr, "inside push function with queue not empty\n");

        queue->tail->next = (ts_queue_el *)malloc(sizeof(ts_queue_el));
        queue->tail = queue->tail->next;
        queue->tail->el = el;
        queue->tail->next = NULL;
    }

    if (pthread_cond_signal(&queue->empty) != 0) {
        perror("tsqueue error during signaling");
        return -1;
    }

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return -1;
    }

    return EXIT_SUCCESS;
}

int int_fifo_tsqueue_pop(int_fifo_tsqueue_t *queue) {
    int ret;
    ts_queue_el *tmp;

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }

    // checks if queue is empty
    if (ISEMPTY((*queue))) {
        if (pthread_mutex_unlock(&queue->mutex) != 0) {
            perror("tsqueue error during mutex unlocking");
            return -1;
        }

        return -1;
    }

    ret = queue->head->el;
    tmp = queue->head;
    queue->head = queue->head->next;

    free(tmp);

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return -1;
    }

    return ret;
}

/**
int int_fifo_tsqueue_init(int_fifo_tsqueue_t *queue, int n) {

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        perror("tsqueue error during mutex initialization");
        return -1;
    }

    if (pthread_cond_init(&queue->empty, NULL) != 0) {
        perror("tsqueue error during condition variable initialization");
        return -1;
    }

    queue->buff = (int*)malloc(n*sizeof(int));

    queue->dim = n;
    queue->head = 0;
    queue->tail = 0;

    return EXIT_SUCCESS;
}

int int_fifo_tsqueue_realloc(int_fifo_tsqueue_t *queue) {
    
    if (pthread_mutex_lock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }

    int *p = realloc(queue->buff, queue->dim*2*sizeof(int));

    if (!p) {
        perror("tsqueue error during realloc");
        pthread_mutex_unlock(&queue->mutex);
        
        return -1;
    } else {
        queue->buff = p;
    }

    queue->dim = queue->dim*2;

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return -1;
    }

    return EXIT_SUCCESS;
}

int int_fifo_tsqueue_free(int_fifo_tsqueue_t *queue) {
    
    if (pthread_mutex_lock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }

    free(queue->buff);

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return -1;
    }

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->empty);

    return EXIT_SUCCESS;
}

int int_fifo_tsqueue_isfull(int_fifo_tsqueue_t queue) {
    
    if (pthread_mutex_lock(&queue.mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }
    
    int ret = (queue.tail+1) % queue.dim == queue.head;
    
    if (pthread_mutex_unlock(&queue.mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return -1;
    }

    return ret;
}

int int_fifo_tsqueue_isempty(int_fifo_tsqueue_t queue) {

    if (pthread_mutex_lock(&queue.mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }

    int ret = queue.tail == queue.head;
    
    if (pthread_mutex_unlock(&queue.mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return -1;
    }

    return ret;
}

int int_fifo_tsqueue_push(int_fifo_tsqueue_t *queue, int el) {
    int isfull;

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }

    isfull = ISFULL((*queue));

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return -1;
    }

    // checks if queue is full
    if (isfull) {
        if (int_fifo_tsqueue_realloc(queue) != 0) {
            perror("tsqueue error during realloc operation");
            return -1;
        }
    }

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }

    queue->buff[queue->tail] = el;

    queue->tail = queue->tail + 1;

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

int int_fifo_tsqueue_pop(int_fifo_tsqueue_t *queue) {
    int ret;

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex locking");
        return -1;
    }

    // checks if queue is empty
    if (ISEMPTY((*queue))) {
        if (pthread_mutex_unlock(&queue->mutex) != 0) {
            perror("tsqueue error during mutex unlocking");
            return -1;
        }

        return -1;
    }

    ret = queue->buff[queue->head];
    queue->head = (queue->head + 1) % queue->dim;

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        perror("tsqueue error during mutex unlocking");
        return -1;
    }

    return ret;
}
*/