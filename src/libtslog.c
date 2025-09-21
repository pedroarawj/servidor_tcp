#include "../include/libtslog.h"
#include <string.h>

static logger_t* log_global = NULL;

logger_t* log_init(const char *nomeArquivo) {
    if (log_global != NULL) {
        return log_global;
    }

    logger_t *log = (logger_t*)malloc(sizeof(logger_t));
    if (log == NULL) {
        return NULL;
    }
    log->arquivo = fopen(nomeArquivo, "a");
    if (log->arquivo == NULL) {
        free(log);
        return NULL;
    }

    if (pthread_mutex_init(&log->mutex, NULL) != 0) {
        fclose(log->arquivo);
        free(log);
        return NULL;
    }

    log_global = log;
    return log_global;
}

void log_escrever(logger_t *log, const char *mensagem) {
    if (log == NULL || mensagem == NULL) {
        return;
    }

    pthread_mutex_lock(&log->mutex);
    fprintf(log->arquivo, "%s\n", mensagem);
    fflush(log->arquivo);
    pthread_mutex_unlock(&log->mutex);
}

void log_destruir(logger_t *log) {
    if (log == NULL) {
        return;
    }

    pthread_mutex_destroy(&log->mutex);
    fclose(log->arquivo);
    free(log);
    log_global = NULL;
}