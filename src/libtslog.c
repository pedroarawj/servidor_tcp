#include "../include/libtslog.h"
#include <string.h>
#include <time.h>

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

    log->verbose = 0;  // Padrão: não exibe no terminal
    log_global = log;
    return log_global;
}

void log_set_verbose(logger_t *log, int verbose) {
    if (log != NULL) {
        log->verbose = verbose;
    }
}

void log_escrever(logger_t *log, const char *mensagem) {
    if (log == NULL || mensagem == NULL) {
        return;
    }

    pthread_mutex_lock(&log->mutex);
    
    // Obter timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d-%m-%Y %H:%M:%S", t);
    
    // Escrever no arquivo
    fprintf(log->arquivo, "[%s] %s\n", timestamp, mensagem);
    fflush(log->arquivo);
    
    // Exibir no terminal se verbose estiver ativado
    if (log->verbose) {
        printf("[%s] %s\n", timestamp, mensagem);
        fflush(stdout);
    }
    
    pthread_mutex_unlock(&log->mutex);
}

void log_escrever_verbose(logger_t *log, const char *mensagem) {
    if (log == NULL || mensagem == NULL) {
        return;
    }

    pthread_mutex_lock(&log->mutex);
    
    // Obter timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d-%m-%Y %H:%M:%S", t);
    
    // Sempre escrever no arquivo
    fprintf(log->arquivo, "[%s] %s\n", timestamp, mensagem);
    fflush(log->arquivo);
    
    printf("[%s] %s\n", timestamp, mensagem);
    fflush(stdout);
    
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