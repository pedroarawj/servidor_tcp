#include "../include/libtslog.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h> 
#include <stdatomic.h>

static atomic_int thread_counter = 0;

#define NUM_THREADS 3

// Função das threasd
void *thread_function(void *arg) {
    logger_t *log = (logger_t*) arg;
     int my_id = atomic_fetch_add(&thread_counter, 1) + 1;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        char message[100];
        sprintf(message, "Thread %d: Log message #%d", my_id, i + 1);
        log_escrever(log, message);
        usleep(1000); 
    }
    
    return NULL;
}

int main() {
    logger_t *log = log_init("app.log");
    if (log == NULL) {
        fprintf(stderr, "Erro ao inicializar o logger.\n");
        return 1;
    }
    
    pthread_t threads[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_function, (void*) log);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    log_destruir(log);
    
    printf("Logs gerados em 'app.log'.\n");
    
    return 0;
}