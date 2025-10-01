#include "../include/libtslog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

static logger_t *log = NULL;

void *receive_messages(void *arg) {
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[read_size] = '\0';
        
        // Exibir mensagem recebida de forma destacada
        printf("\n📨 MENSAGEM DO SERVIDOR: %s\n", buffer);
        printf("> ");
        fflush(stdout);
        
        // Log da mensagem recebida
        char log_msg[BUFFER_SIZE + 50];
        sprintf(log_msg, "Mensagem recebida do servidor: %s", buffer);
        log_escrever_verbose(log, log_msg);
    }

    // Servidor desconectou
    printf("\n❌ Servidor desconectou\n");
    log_escrever_verbose(log, "Servidor desconectou");
    
    return NULL;
}

int main(int argc, char const *argv[]) {
    log = log_init("cliente.log");
    if (log == NULL) {
        fprintf(stderr, "Erro ao inicializar logger do cliente.\n");
        return 1;
    }
    
    // Ativar modo verbose para exibir no terminal
    log_set_verbose(log, 1);

    int sock = 0;
    struct sockaddr_in serv_addr;
    pthread_t recv_thread;
    char server_ip[100] = "127.0.0.1"; // IP padrão local

    // Permitir especificar IP do servidor por argumento
    if (argc > 1) {
        strncpy(server_ip, argv[1], sizeof(server_ip) - 1);
    }

    printf("=== CLIENTE DE CHAT ===\n");
    printf("Conectando ao servidor %s:%d...\n", server_ip, PORT);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n❌ Erro na criação do socket\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converter endereço IP
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\n❌ Endereço IP inválido: %s\n", server_ip);
        return -1;
    }


    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n❌ Conexão falhou com %s:%d\n", server_ip, PORT);
        printf("   Certifique-se que o servidor está rodando\n");
        log_escrever_verbose(log, "ERRO: Falha na conexão com o servidor");
        return -1;
    }


    // Thread para receber mensagens
    pthread_create(&recv_thread, NULL, receive_messages, (void *)&sock);
    pthread_detach(recv_thread);

    // Enviar mensagens
    char message[BUFFER_SIZE];
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        message[strcspn(message, "\n")] = 0; // Remover newline

        if (strlen(message) == 0) continue;

        // Enviar mensagem para o servidor
        int bytes_sent = send(sock, message, strlen(message), 0);
        if (bytes_sent < 0) {
            printf("❌ Erro ao enviar mensagem\n");
            break;
        }
        
        // Log da mensagem enviada
        char log_msg[BUFFER_SIZE + 50];
        sprintf(log_msg, "Mensagem enviada para servidor: %s", message);
        log_escrever_verbose(log, log_msg);

        if (strcmp(message, "sair") == 0) {
            printf("Desconectando...\n");
            break;
        }
    }

    close(sock);
    log_escrever_verbose(log, "Cliente desconectado");
    log_destruir(log);
    
    printf("Cliente finalizado.\n");
    return 0;
}