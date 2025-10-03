#include "../include/fila_threadsafe.h"
#include <string.h>
#include <stdio.h>

/**
 * Inicializa a fila thread-safe
 * @param q Ponteiro para a fila a ser inicializada
 */
void tsqueue_init(ThreadSafeQueue *q) {
    // Inicializa índices e tamanho
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    
    // Inicializa mutex para exclusão mútua
    pthread_mutex_init(&q->mutex, NULL);
    
    // Inicializa condition variables para sincronização
    pthread_cond_init(&q->not_empty, NULL);  // Sinaliza quando a fila não está vazia
    pthread_cond_init(&q->not_full, NULL);   // Sinaliza quando a fila não está cheia
}

/**
 * Destroi a fila thread-safe e libera recursos
 * @param q Ponteiro para a fila a ser destruída
 */
void tsqueue_destroy(ThreadSafeQueue *q) {
    // Destroi mutex e condition variables
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

/**
 * Insere uma mensagem na fila (operacao bloqueante se cheia)
 * @param q Ponteiro para a fila
 * @param msg Mensagem a ser inserida (string)
 */
void tsqueue_push(ThreadSafeQueue *q, const char *msg) {
    // Bloqueia o mutex para acesso exclusivo à fila
    pthread_mutex_lock(&q->mutex);
    
    // Espera enquanto a fila estiver cheia 
    while (q->size == QUEUE_CAPACITY) {
        // Libera temporariamente o mutex e espera pelo sinal de não-cheia
        pthread_cond_wait(&q->not_full, &q->mutex);
    }

    // Copia a mensagem para o buffer na posição tail
    strncpy(q->buffer[q->tail], msg, MSG_SIZE - 1);
    q->buffer[q->tail][MSG_SIZE - 1] = '\0'; // Garante null terminator para segurança
    
    // Atualiza cauda (fila circular)
    q->tail = (q->tail + 1) % QUEUE_CAPACITY;
    q->size++; // Incrementa contador de elementos

    // Sinaliza que a fila não está mais vazia (pode haver threads esperando)
    pthread_cond_signal(&q->not_empty);
    
    // Libera o mutex
    pthread_mutex_unlock(&q->mutex);
}

/**
 * Remove uma mensagem da fila (operacao bloqueante se vazia)
 * @param q Ponteiro para a fila
 * @param out Buffer onde a mensagem removida será copiada
 */
void tsqueue_pop(ThreadSafeQueue *q, char *out) {
    // Bloqueia o mutex para acesso exclusivo à fila
    pthread_mutex_lock(&q->mutex);
    
    // Espera enquanto a fila estiver vazia 
    while (q->size == 0) {
        // Libera temporariamente o mutex e espera pelo sinal de não-vazia
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }

    // Copia a mensagem da posição head para o buffer de saída
    strncpy(out, q->buffer[q->head], MSG_SIZE);
    
    // Atualiza cabeca (fila circular)
    q->head = (q->head + 1) % QUEUE_CAPACITY;
    q->size--; // Decrementa contador de elementos

    // Sinaliza que a fila não está mais cheia (pode haver threads esperando)
    pthread_cond_signal(&q->not_full);
    
    // Libera o mutex
    pthread_mutex_unlock(&q->mutex);
}