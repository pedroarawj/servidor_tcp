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

// Variável global para o logger do cliente
static logger_t *log = NULL;

/**
 * Thread para receber mensagens do servidor
 * @param arg Ponteiro para o socket do cliente
 * @return NULL
 */
void *receive_messages(void *arg) {
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int read_size;

    // Loop principal de recebimento de mensagens
    while ((read_size = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[read_size] = '\0'; // Adiciona null terminator
        
        // Exibir mensagem recebida de forma destacada no terminal
        printf("\n📨 MENSAGEM DO SERVIDOR: %s\n", buffer);
        printf("> ");
        fflush(stdout); // Força exibição imediata
        
        // Log da mensagem recebida no arquivo de log
        char log_msg[BUFFER_SIZE + 50];
        sprintf(log_msg, "Mensagem recebida do servidor: %s", buffer);
        log_escrever_verbose(log, log_msg);
    }

    // Servidor desconectou (recv retornou 0 ou erro)
    printf("\n❌ Servidor desconectou\n");
    log_escrever_verbose(log, "Servidor desconectou");
    
    return NULL;
}

/**
 * Função principal do cliente
 * @param argc Número de argumentos
 * @param argv Argumentos: [0] nome programa, [1] IP do servidor (opcional)
 * @return 0 em sucesso, -1 em erro
 */
int main(int argc, char const *argv[]) {
    // Inicializa o sistema de logging
    log = log_init("cliente.log");
    if (log == NULL) {
        fprintf(stderr, "Erro ao inicializar logger do cliente.\n");
        return 1;
    }
    
    // Ativar modo verbose para exibir logs no terminal
    log_set_verbose(log, 1);

    int sock = 0;
    struct sockaddr_in serv_addr;
    pthread_t recv_thread;
    char server_ip[100] = "127.0.0.1"; // IP padrão local (localhost)

    // Verifica se foi especificado IP do servidor por argumento
    if (argc > 1) {
        strncpy(server_ip, argv[1], sizeof(server_ip) - 1);
    }

    // Interface inicial do usuário
    printf("=== CLIENTE DE CHAT ===\n");
    printf("Conectando ao servidor %s:%d...\n", server_ip, PORT);

    // Cria socket TCP
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n❌ Erro na criação do socket\n");
        return -1;
    }

    // Configura estrutura de endereço do servidor
    serv_addr.sin_family = AF_INET;          // Família IPv4
    serv_addr.sin_port = htons(PORT);        // Porta (converte para network byte order)

    // Converte endereço IP de string para binário
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\n❌ Endereço IP inválido: %s\n", server_ip);
        return -1;
    }

    // Tenta conectar ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n❌ Conexão falhou com %s:%d\n", server_ip, PORT);
        printf("   Certifique-se que o servidor está rodando\n");
        log_escrever_verbose(log, "ERRO: Falha na conexão com o servidor");
        return -1;
    }

    // Cria thread para receber mensagens do servidor
    pthread_create(&recv_thread, NULL, receive_messages, (void *)&sock);
    pthread_detach(recv_thread); // A thread se auto-liberará ao terminar

    // Loop principal para envio de mensagens
    char message[BUFFER_SIZE];
    while (1) {
        printf("> ");
        fflush(stdout); // Força exibição do prompt
        
        // Lê mensagem do usuário
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            break; // EOF ou erro de leitura
        }
        
        // Remove newline do final da mensagem
        message[strcspn(message, "\n")] = 0;

        // Ignora mensagens vazias
        if (strlen(message) == 0) continue;

        // Envia mensagem para o servidor
        int bytes_sent = send(sock, message, strlen(message), 0);
        if (bytes_sent < 0) {
            printf("❌ Erro ao enviar mensagem\n");
            break;
        }
        
        // Log da mensagem enviada
        char log_msg[BUFFER_SIZE + 50];
        sprintf(log_msg, "Mensagem enviada para servidor: %s", message);
        log_escrever_verbose(log, log_msg);

        // Verifica se é comando de saída
        if (strcmp(message, "sair") == 0) {
            printf("Desconectando...\n");
            break;
        }
    }

    // Cleanup: fecha socket e destrói logger
    close(sock);
    log_escrever_verbose(log, "Cliente desconectado");
    log_destruir(log);
    
    printf("Cliente finalizado.\n");
    return 0;
}