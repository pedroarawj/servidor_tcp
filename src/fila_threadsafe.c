#include "../include/fila_threadsafe.h"
#include <string.h>
#include <stdio.h>

void tsqueue_init(ThreadSafeQueue *q) {
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

void tsqueue_destroy(ThreadSafeQueue *q) {
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

void tsqueue_push(ThreadSafeQueue *q, const char *msg) {
    pthread_mutex_lock(&q->mutex);
    while (q->size == QUEUE_CAPACITY) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }

    strncpy(q->buffer[q->tail], msg, MSG_SIZE - 1);
    q->buffer[q->tail][MSG_SIZE - 1] = '\0'; // garantir null terminator
    q->tail = (q->tail + 1) % QUEUE_CAPACITY;
    q->size++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

void tsqueue_pop(ThreadSafeQueue *q, char *out) {
    pthread_mutex_lock(&q->mutex);
    while (q->size == 0) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }

    strncpy(out, q->buffer[q->head], MSG_SIZE);
    q->head = (q->head + 1) % QUEUE_CAPACITY;
    q->size--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
}
