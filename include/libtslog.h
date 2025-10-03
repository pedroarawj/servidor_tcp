#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    FILE *arquivo;
    int verbose;  // controle de exibição
} logger_t;

logger_t* log_init(const char *nomeArquivo);
void log_escrever(logger_t *log, const char *mensagem);
void log_escrever_verbose(logger_t *log, const char *mensagem);  
void log_set_verbose(logger_t *log, int verbose);
void log_erro(logger_t *log, const char *operacao, int error_code); 
void log_destruir(logger_t *log);