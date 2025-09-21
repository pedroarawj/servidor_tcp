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

# Teste
TEST_SRC = $(TEST_DIR)/log_teste.c
TEST_BIN = $(BUILD_DIR)/log_teste.exe

# =============================================
# REGRAS PRINCIPAIS
# =============================================

# Alvo padrão - compila e executa automaticamente
all: $(TEST_BIN)
	@echo "Executando programa..."
	./$(TEST_BIN)

# Apenas compilar (sem executar) 
compile: $(TEST_BIN)
	@echo "Compilação concluída. Execute com: ./$(TEST_BIN)"

# Compilar e executar
run: $(TEST_BIN)
	@echo "Executando programa..."
	./$(TEST_BIN)

# =============================================
# REGRAS DE COMPILAÇÃO
# =============================================

# Criar diretório build se não existir
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compilar a biblioteca
$(LIB_OBJ): $(LIB_SRC) $(LIB_HEADER) | $(BUILD_DIR)
	@echo "Compilando biblioteca..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar o teste
$(TEST_BIN): $(TEST_SRC) $(LIB_OBJ) | $(BUILD_DIR)
	@echo "Compilando e linkando teste..."
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# =============================================
# REGRAS UTILITÁRIAS
# =============================================

# Limpar arquivos compilados
clean:
	@echo "Limpando arquivos compilados..."
	rm -rf $(BUILD_DIR)

# Forçar recompilação completa
rebuild: clean all

# Ajuda
help:
	@echo "=== Makefile para libtslog ==="
	@echo "Comandos disponíveis:"
	@echo "  make         - Compila e executa automaticamente"
	@echo "  make compile - Apenas compila (sem executar)"
	@echo "  make run     - Compila e executa"
	@echo "  make clean   - Remove pasta build completamente"
	@echo "  make rebuild - Limpa e recompila tudo"
	@echo "  make help    - Mostra esta ajuda"

# =============================================
# DECLARAÇÃO DE ALVOS FALSOS
# =============================================
.PHONY: all compile run clean rebuild help