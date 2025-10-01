#!/bin/bash

echo "=== Teste de Múltiplos Clientes ==="
echo "Iniciando servidor em background..."

# Iniciar servidor em background
./build/servidor &
SERVER_PID=$!

# Aguardar servidor inicializar
sleep 3

echo "Servidor iniciado (PID: $SERVER_PID)"
echo "Iniciando múltiplos clientes..."

# Função para executar cliente em terminal separado
start_client() {
    local client_id=$1
    gnome-terminal --title="Cliente $client_id" -- ./build/cliente 127.0.0.1
    sleep 1
}

# Iniciar múltiplos clientes
for i in {1..3}; do
    echo "Iniciando cliente $i..."
    start_client $i
done

echo ""
echo "✅ Todos os clientes foram iniciados"
echo "   - Servidor: localhost:8080"
echo "   - Clientes: 3 conectados"
echo ""
echo "Pressione Enter para finalizar o teste..."
read

# Finalizar servidor
echo "Finalizando servidor..."
kill $SERVER_PID 2>/dev/null

echo "Teste concluído. Verifique os arquivos de log:"
echo "   - servidor.log"
echo "   - cliente.log"