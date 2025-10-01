# =============================================
# CONFIGURAÇÕES DO COMPILADOR
# =============================================
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g -I./include
LDFLAGS = -lpthread

# =============================================
# DIRETÓRIOS DO PROJETO
# =============================================
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
TEST_DIR = test

# =============================================
# ARQUIVOS DO PROJETO
# =============================================
# Biblioteca
LIB_SRC = $(SRC_DIR)/libtslog.c
LIB_OBJ = $(BUILD_DIR)/libtslog.o
LIB_HEADER = $(INCLUDE_DIR)/libtslog.h

# Teste unitário
TEST_SRC = $(TEST_DIR)/log_teste.c
TEST_BIN = $(BUILD_DIR)/log_teste

# Servidor e Cliente
SERVER_SRC = $(SRC_DIR)/servidor.c
SERVER_OBJ = $(BUILD_DIR)/servidor.o
SERVER_BIN = $(BUILD_DIR)/servidor

CLIENT_SRC = $(SRC_DIR)/cliente.c
CLIENT_OBJ = $(BUILD_DIR)/cliente.o
CLIENT_BIN = $(BUILD_DIR)/cliente

# Script de teste
TEST_SCRIPT = $(TEST_DIR)/testar_cliente.sh

# =============================================
# REGRAS PRINCIPAIS
# =============================================

# Alvo padrão - compila tudo
all: libtslog log_teste servidor cliente
	@echo "=== Compilação concluída ==="
	@echo "Arquivos gerados em $(BUILD_DIR)/:"
	@echo "  - $(notdir $(TEST_BIN))    (teste unitário)"
	@echo "  - $(notdir $(SERVER_BIN))  (servidor)"
	@echo "  - $(notdir $(CLIENT_BIN))  (cliente)"

# Apenas compilar (sem executar) 
compile: all
	@echo "Compilação concluída."

# Compilar e executar teste unitário
run: $(TEST_BIN)
	@echo "Executando teste unitário..."
	./$(TEST_BIN)

# Compilar e executar servidor
run-server: $(SERVER_BIN)
	@echo "Executando servidor..."
	./$(SERVER_BIN)

# Compilar e executar cliente
run-client: $(CLIENT_BIN)
	@echo "Executando cliente..."
	./$(CLIENT_BIN)

# Testar com múltiplos clientes
test-clients: servidor cliente
	@echo "=== Teste com múltiplos clientes ==="
	@chmod +x $(TEST_SCRIPT)
	./$(TEST_SCRIPT)

# =============================================
# REGRAS DE COMPILAÇÃO
# =============================================

# Criar diretório build se não existir
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compilar a biblioteca
$(LIB_OBJ): $(LIB_SRC) $(LIB_HEADER) | $(BUILD_DIR)
	@echo "Compilando biblioteca libtslog..."
	$(CC) $(CFLAGS) -c $< -o $@

# Alvo para biblioteca
libtslog: $(LIB_OBJ)

# Compilar o teste unitário
$(TEST_BIN): $(TEST_SRC) $(LIB_OBJ) | $(BUILD_DIR)
	@echo "Compilando teste unitário..."
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Alvo para teste unitário
log_teste: $(TEST_BIN)

# Compilar servidor
$(SERVER_OBJ): $(SERVER_SRC) $(LIB_HEADER) | $(BUILD_DIR)
	@echo "Compilando servidor..."
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_BIN): $(SERVER_OBJ) $(LIB_OBJ) | $(BUILD_DIR)
	@echo "Linkando servidor..."
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Alvo para servidor
servidor: $(SERVER_BIN)

# Compilar cliente
$(CLIENT_OBJ): $(CLIENT_SRC) $(LIB_HEADER) | $(BUILD_DIR)
	@echo "Compilando cliente..."
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_BIN): $(CLIENT_OBJ) $(LIB_OBJ) | $(BUILD_DIR)
	@echo "Linkando cliente..."
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Alvo para cliente
cliente: $(CLIENT_BIN)

# =============================================
# REGRAS UTILITÁRIAS
# =============================================

# Limpar arquivos compilados
clean:
	@echo "Limpando arquivos compilados..."
	rm -rf $(BUILD_DIR)
	@echo "Limpando arquivos de log..."
	rm -f *.log

# Forçar recompilação completa
rebuild: clean all

# Executar testes rapidamente (apenas se já compilado)
quick-test: 
	@echo "=== Teste Rápido ==="
	@if [ -f "$(SERVER_BIN)" ] && [ -f "$(CLIENT_BIN)" ]; then \
		echo "Iniciando servidor em background..."; \
		./$(SERVER_BIN) & \
		SERVER_PID=$$!; \
		sleep 2; \
		echo "Executando cliente..."; \
		./$(CLIENT_BIN); \
		kill $$SERVER_PID 2>/dev/null || true; \
	else \
		echo "Erro: Execute 'make all' primeiro para compilar."; \
	fi

# Mostrar status da compilação
status:
	@echo "=== Status do Projeto ==="
	@echo "Arquivos na pasta $(BUILD_DIR)/:"
	@ls -la $(BUILD_DIR)/ 2>/dev/null || echo "Pasta $(BUILD_DIR) não existe."
	@echo ""
	@echo "Arquivos de log:"
	@ls -la *.log 2>/dev/null || echo "Nenhum arquivo de log encontrado."

# Ajuda
help:
	@echo "=== Makefile para Sistema de Chat com Logging ==="
	@echo "Comandos disponíveis:"
	@echo ""
	@echo "COMPILAÇÃO:"
	@echo "  make           - Compila todos os componentes"
	@echo "  make all       - Compila tudo (mesmo que make)"
	@echo "  make compile   - Apenas compila (sem executar)"
	@echo "  make rebuild   - Limpa e recompila tudo"
	@echo ""
	@echo "COMPONENTES INDIVIDUAIS:"
	@echo "  make libtslog  - Compila apenas a biblioteca"
	@echo "  make log_teste - Compila apenas o teste unitário"
	@echo "  make servidor  - Compila apenas o servidor"
	@echo "  make cliente   - Compila apenas o cliente"
	@echo ""
	@echo "EXECUÇÃO:"
	@echo "  make run           - Executa teste unitário"
	@echo "  make run-server    - Executa servidor"
	@echo "  make run-client    - Executa cliente"
	@echo "  make test-clients  - Teste com múltiplos clientes (script)"
	@echo "  make quick-test    - Teste rápido servidor+cliente"
	@echo ""
	@echo "UTILITÁRIOS:"
	@echo "  make clean   - Remove pasta build e arquivos de log"
	@echo "  make status  - Mostra status dos arquivos compilados"
	@echo "  make help    - Mostra esta ajuda"

# =============================================
# DECLARAÇÃO DE ALVOS FALSOS
# =============================================
.PHONY: all compile run run-server run-client test-clients quick-test \
        libtslog log_teste servidor cliente clean rebuild status help