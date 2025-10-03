#include "../include/libtslog.h"
#include "../include/fila_threadsafe.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

static logger_t *log = NULL;
int client_sockets[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Fila global de mensagens
ThreadSafeQueue msg_queue;

// Variáveis globais para controle de shutdown
static volatile sig_atomic_t shutdown_requested = 0;
static int server_fd_global = -1;

/**
 * Handler para sinais de shutdown (Ctrl+C, etc)
 */
void graceful_shutdown(int sig) {
    printf("\n🛑 Recebido sinal %d. Finalizando servidor graciosamente...\n", sig);
    shutdown_requested = 1;
    
    // Fechar socket do servidor para parar accept()
    if (server_fd_global != -1) {
        close(server_fd_global);
    }
}

/**
 * Registrar handlers de sinal
 */
void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = graceful_shutdown;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, NULL);  // Ctrl+C
    sigaction(SIGTERM, &sa, NULL); // kill command
}

/**
 * Marcar socket para remoção da lista
 */
void mark_socket_for_removal(int bad_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == bad_socket) {
            client_sockets[i] = 0; // Marcar como disponível
            close(bad_socket); // Fechar o socket problemático
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    char remove_log[100];
    sprintf(remove_log, "Socket %d removido por erro de comunicação", bad_socket);
    tsqueue_push(&msg_queue, remove_log);
}

/**
 * Broadcast: envia mensagem para todos os clientes conectados
 * Versão segura contra race conditions
 */
void broadcast_message(const char *msg, int exclude_fd) {
    int socket_copy[MAX_CLIENTS];
    int client_count = 0;
    
    // Fazer cópia protegida da lista de sockets
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0) {
            socket_copy[client_count++] = client_sockets[i];
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    int sent_count = 0;
    int failed_count = 0;
    
    // Enviar para a cópia (sem precisar de lock)
    for (int i = 0; i < client_count; i++) {
        if (socket_copy[i] != exclude_fd) {
            int bytes_sent = send(socket_copy[i], msg, strlen(msg), MSG_DONTWAIT);
            if (bytes_sent > 0) {
                sent_count++;
            } else {
                // Verificar se é erro não-blocking ou socket fechado
                if (bytes_sent < 0 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
                    failed_count++;
                    // Marcar socket para remoção posterior
                    mark_socket_for_removal(socket_copy[i]);
                }
            }
        }
    }
    
    // Log do broadcast
    char broadcast_log[150];
    sprintf(broadcast_log, "Broadcast: '%s' enviado para %d/%d clientes (%d falhas)", 
            msg, sent_count, client_count, failed_count);
    tsqueue_push(&msg_queue, broadcast_log);
}

/**
 * Thread que consome mensagens da fila e grava no log centralizado
 */
void *logger_thread(void *arg) {
    char msg[MSG_SIZE];
    while (!shutdown_requested) {
        tsqueue_pop(&msg_queue, msg); // espera até ter msg
        log_escrever_verbose(log, msg);
        printf("📜 [LoggerThread] %s\n", msg);
    }
    return NULL;
}

/**
 * Contador de clientes conectados
 */
int count_connected_clients() {
    pthread_mutex_lock(&clients_mutex);
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0) {
            count++;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return count;
}

/**
 * Thread para atender um cliente 
 */
void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    int read_size;
    char client_ip[INET_ADDRSTRLEN];
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // Obter informações do cliente
    getpeername(client_fd, (struct sockaddr*)&addr, &addr_len);
    inet_ntop(AF_INET, &addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(addr.sin_port);


    char conn_msg[150];
    sprintf(conn_msg, "Cliente conectado: FD=%d, IP=%s:%d", 
            client_fd, client_ip, client_port);
    tsqueue_push(&msg_queue, conn_msg);
    
    // Notificar outros clientes sobre nova conexão
    char welcome_msg[BUFFER_SIZE];
    sprintf(welcome_msg, "🟢 Novo usuário conectado: %s:%d", client_ip, client_port);
    broadcast_message(welcome_msg, client_fd);
    
    // Mensagem de boas-vindas para o novo cliente
    char personal_welcome[100];
    sprintf(personal_welcome, "Bem-vindo ao chat! Você está conectado como %s:%d", client_ip, client_port);
    if (send(client_fd, personal_welcome, strlen(personal_welcome), MSG_DONTWAIT) < 0) {
        // Erro ao enviar - cliente provavelmente desconectou
        goto cleanup;
    }

    printf("✅ Novo cliente conectado: %s:%d\n", client_ip, client_port);

    while (!shutdown_requested && (read_size = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[read_size] = '\0';
        
        // Verificar se é comando de saída
        if (strcmp(buffer, "sair") == 0 || strcmp(buffer, "/quit") == 0) {
            break;
        }
        
        // Ignorar mensagens vazias
        if (strlen(buffer) == 0) continue;
        
        // Exibir mensagem recebida no servidor
        printf("\n📨 [%s:%d]: %s\n", client_ip, client_port, buffer);
        
        // Formatar mensagem para broadcast
        char formatted_msg[BUFFER_SIZE + 100];
        sprintf(formatted_msg, "[%s:%d]: %s", client_ip, client_port, buffer);
        
        // Enviar para TODOS os clientes (broadcast)
        broadcast_message(formatted_msg, client_fd);
        
        // Enviar para o logger thread-safe
        char log_msg[BUFFER_SIZE + 100];
        sprintf(log_msg, "Mensagem do cliente [%s:%d]: %s", 
                client_ip, client_port, buffer);
        tsqueue_push(&msg_queue, log_msg);
        
        printf("> Mensagem broadcast enviada para outros clientes...\n");
    }

cleanup:
    // Cliente desconectado
    char disc_msg[100];
    sprintf(disc_msg, "Cliente desconectado: %s:%d", client_ip, client_port);
    tsqueue_push(&msg_queue, disc_msg);
    
    // Notificar outros clientes sobre a desconexão (se não for shutdown)
    if (!shutdown_requested) {
        char leave_msg[BUFFER_SIZE];
        sprintf(leave_msg, "🔴 Usuário saiu: %s:%d", client_ip, client_port);
        broadcast_message(leave_msg, client_fd);
    }
    
    printf("❌ Cliente desconectado: %s:%d\n", client_ip, client_port);

    // Remover cliente da lista
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_fd) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(client_fd);
    return NULL;
}

/**
 * Função principal - Versão com shutdown graceful
 */
int main() {
    // Configurar handlers de sinal primeiro
    setup_signal_handlers();
    
    log = log_init("servidor.log");
    if (log == NULL) {
        fprintf(stderr, "Erro ao inicializar logger do servidor.\n");
        return 1;
    }
    log_set_verbose(log, 1);

    // Inicializar fila de mensagens
    tsqueue_init(&msg_queue);

    // Criar thread para consumir mensagens da fila e registrar logs
    pthread_t log_tid;
    if (pthread_create(&log_tid, NULL, logger_thread, NULL) != 0) {
        log_erro(log, "criação da thread de logger", errno);
        return 1;
    }
    pthread_detach(log_tid);

    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Inicializar lista de clientes
    memset(client_sockets, 0, sizeof(client_sockets));

    // Criar socket
    if ((server_fd_global = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        log_erro(log, "criação do socket", errno);
        exit(EXIT_FAILURE);
    }

    // Configurar socket para reutilizar porta
    if (setsockopt(server_fd_global, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        log_erro(log, "configuração do socket", errno);
        close(server_fd_global);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd_global, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_erro(log, "bind da porta", errno);
        close(server_fd_global);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd_global, 3) < 0) {
        log_erro(log, "listen", errno);
        close(server_fd_global);
        exit(EXIT_FAILURE);
    }

    char startup_msg[100];
    sprintf(startup_msg, "=== Servidor de Chat Iniciado (Porta: %d) ===", PORT);
    tsqueue_push(&msg_queue, startup_msg);
    
    printf("🚀 Servidor de Chat iniciado na porta %d\n", PORT);
    printf("📡 Aguardando conexões de clientes...\n");
    printf("💡 Pressione Ctrl+C para finalizar graciosamente\n");

    // Loop principal com verificação de shutdown
    while (!shutdown_requested) {
        // Accept com timeout para verificar shutdown
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd_global, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(server_fd_global + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR) {
            log_erro(log, "select", errno);
            break;
        }
        
        if (shutdown_requested) {
            break;
        }
        
        if (activity > 0 && FD_ISSET(server_fd_global, &readfds)) {
            int client_fd = accept(server_fd_global, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (client_fd < 0) {
                if (errno != EINTR) {
                    log_erro(log, "accept", errno);
                }
                continue;
            }

            // Verificar se há slots disponíveis
            if (count_connected_clients() >= MAX_CLIENTS) {
                char reject_msg[] = "Servidor cheio. Tente novamente mais tarde.";
                send(client_fd, reject_msg, strlen(reject_msg), MSG_DONTWAIT);
                close(client_fd);
                
                char full_msg[100];
                sprintf(full_msg, "Cliente rejeitado - Limite máximo (%d) atingido", MAX_CLIENTS);
                tsqueue_push(&msg_queue, full_msg);
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
                    
                    if (pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) != 0) {
                        log_erro(log, "criação da thread do cliente", errno);
                        free(new_sock);
                        client_sockets[i] = 0;
                        close(client_fd);
                    } else {
                        pthread_detach(thread_id);
                    }
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            
            if (!added) {
                tsqueue_push(&msg_queue, "ERRO: Número máximo de clientes atingido");
                close(client_fd);
            } else {
                printf("👥 Clientes conectados: %d/%d\n", count_connected_clients(), MAX_CLIENTS);
            }
        }
    }

    // SHUTDOWN GRACEFUL
    printf("\n🧹 Finalizando servidor suavemente...\n");
    
    // Fechar todos os sockets dos clientes
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0) {
            close(client_sockets[i]);
            client_sockets[i] = 0;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    // Fechar socket do servidor
    if (server_fd_global != -1) {
        close(server_fd_global);
    }
    
    // Log final
    tsqueue_push(&msg_queue, "Servidor finalizado suavemente");
    
    // Dar tempo para a thread de logger processar mensagens pendentes
    sleep(2);
    
    // Cleanup
    log_destruir(log);
    tsqueue_destroy(&msg_queue);
    
    printf("✅ Servidor finalizado com sucesso.\n");
    return 0;
}