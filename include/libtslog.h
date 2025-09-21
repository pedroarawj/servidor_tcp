#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    FILE *arquivo;
} logger_t;

logger_t* log_init(const char *nomeArquivo);
void log_escrever(logger_t *log, const char *mensagem);
void log_destruir(logger_t *log);