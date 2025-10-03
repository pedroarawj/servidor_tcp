#include <pthread.h>

#define QUEUE_CAPACITY 50
#define MSG_SIZE 1024

typedef struct {
    char buffer[QUEUE_CAPACITY][MSG_SIZE]; // mensagens armazenadas
    int head;
    int tail;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} ThreadSafeQueue;

// Inicializa a fila
void tsqueue_init(ThreadSafeQueue *q);

// Destroi a fila
void tsqueue_destroy(ThreadSafeQueue *q);

// Insere mensagem (bloqueia se fila cheia)
void tsqueue_push(ThreadSafeQueue *q, const char *msg);

// Remove mensagem (bloqueia se fila vazia)
void tsqueue_pop(ThreadSafeQueue *q, char *out);
