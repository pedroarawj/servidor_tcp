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
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

static logger_t *log = NULL;
int client_sockets[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int read_size;
    char client_ip[INET_ADDRSTRLEN];
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // Obter informações do cliente
    getpeername(client_fd, (struct sockaddr*)&addr, &addr_len);
    inet_ntop(AF_INET, &addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    char conn_msg[150];
    sprintf(conn_msg, "Cliente conectado: FD=%d, IP=%s:%d", client_fd, client_ip, ntohs(addr.sin_port));
    log_escrever_verbose(log, conn_msg);
    
    printf("✅ Novo cliente conectado: %s:%d\n", client_ip, ntohs(addr.sin_port));

    while ((read_size = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[read_size] = '\0';
        
        // Exibir mensagem do cliente de forma destacada
        printf("\n📨 MENSAGEM DO CLIENTE [%s:%d]: %s\n", 
               client_ip, ntohs(addr.sin_port), buffer);
        printf("> Aguardando próxima mensagem...\n");
        
        // Log da mensagem recebida
        char log_msg[BUFFER_SIZE + 100];
        sprintf(log_msg, "Mensagem do cliente [%s:%d]: %s", 
                client_ip, ntohs(addr.sin_port), buffer);
        log_escrever_verbose(log, log_msg);
    }

    // Cliente desconectado
    char disc_msg[100];
    sprintf(disc_msg, "Cliente desconectado: %s:%d", client_ip, ntohs(addr.sin_port));
    log_escrever_verbose(log, disc_msg);
    printf("❌ Cliente desconectado: %s:%d\n", client_ip, ntohs(addr.sin_port));

    // Remover cliente
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_fd) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(client_fd);
    free(arg);
    return NULL;
}

int main() {
    log = log_init("servidor.log");
    if (log == NULL) {
        fprintf(stderr, "Erro ao inicializar logger do servidor.\n");
        return 1;
    }
    
    // Ativar modo verbose para exibir no terminal
    log_set_verbose(log, 1);

    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Inicializar array de clientes
    memset(client_sockets, 0, sizeof(client_sockets));

    // Criar socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configurar socket para reutilizar porta
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind - associar socket à porta
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        log_escrever_verbose(log, "ERRO: Falha no bind da porta 8080");
        exit(EXIT_FAILURE);
    }

    // Listen - escutar por conexões
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        log_escrever_verbose(log, "ERRO: Falha no listen");
        exit(EXIT_FAILURE);
    }

    log_escrever_verbose(log, "=== Servidor de Recepção Iniciado ===");

    while (1) {
        // Accept - aceitar conexões de clientes
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        // Adicionar cliente à lista
        pthread_mutex_lock(&clients_mutex);
        int added = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = client_fd;
                added = 1;
                
                // Criar thread para o cliente
                pthread_t thread_id;
                int *new_sock = malloc(sizeof(int));
                *new_sock = client_fd;
                pthread_create(&thread_id, NULL, handle_client, (void*)new_sock);
                pthread_detach(thread_id);
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
        
        if (!added) {
            log_escrever_verbose(log, "ERRO: Número máximo de clientes atingido");
            close(client_fd);
        }
    }

    close(server_fd);
    log_escrever_verbose(log, "Servidor finalizado");
    log_destruir(log);
    return 0;
}