# 📝 libtslog - Biblioteca de Logging Thread-Safe

## 🏗️ Arquitetura e Padrões

### **Componentes Principais**

- **Estrutura `logger_t`**: Armazena o handle para o arquivo de log (`FILE*`) e um mutex (`pthread_mutex_t`) para serializar acesso concorrente
- **Função `log_init`**: Atua como construtor do Singleton - inicializa a estrutura, abre arquivo em modo "append" e inicializa o mutex
- **Função `log_escrever`**: Método principal - adquire lock, escreve mensagem com timestamp, garante flush e libera lock
- **Função `log_destruir`**: Destrutor - libera todos os recursos alocados (mutex, arquivo, memória)

### **Fluxo de Dados**

1. Aplicação cliente chama `log_init()` para obter instância do logger
2. Threads chamam `log_escrever()` passando mensagens
3. Internamente, `log_escrever()` serializa acesso ao arquivo
4. No final da aplicação, `log_destruir()` libera recursos

## 🚀 Sistema de Chat com Logging Integrado

### **Funcionalidades**
- ✅ **Servidor TCP/IP** multi-cliente na porta 18080
- ✅ **Cliente de chat** com interface intuitiva
- ✅ **Logging thread-safe** com timestamps
- ✅ **Scripts de teste** para múltiplos clientes
- ✅ **Makefile completo** com automação

## 📁 Estrutura do Projeto

```
.
├── src/
│   ├── libtslog.c          # Biblioteca de logging
│   ├── servidor.c          # Servidor de chat
│   └── cliente.c           # Cliente de chat
├── include/
│   └── libtslog.h          # Headers da biblioteca
├── test/
│   ├── log_teste.c         # Testes unitários
│   └── testar_cliente.sh   # Script de teste múltiplos clientes
├── build/                  # Binários compilados
└── Makefile               # Sistema de build
```

## 🔧 Compilação

### **Pré-requisitos**
- GCC
- Make
- Biblioteca pthread

### **Compilação Automática (Recomendada)**
```bash
# Compilar tudo
make all

# Apenas compilar (sem executar)
make compile

# Limpar e recompilar tudo
make rebuild
```

### **Compilação Manual**
```bash
# Criar pasta build
mkdir -p build

# Compilar a biblioteca
gcc -Wall -Wextra -pedantic -g -I./include -c src/libtslog.c -o build/libtslog.o

# Compilar e linkar o teste
gcc -Wall -Wextra -pedantic -g -I./include test/log_teste.c build/libtslog.o -o build/log_teste -lpthread

# Compilar servidor
gcc -Wall -Wextra -pedantic -g -I./include -c src/servidor.c -o build/servidor.o
gcc build/servidor.o build/libtslog.o -o build/servidor -lpthread

# Compilar cliente
gcc -Wall -Wextra -pedantic -g -I./include -c src/cliente.c -o build/cliente.o
gcc build/cliente.o build/libtslog.o -o build/cliente -lpthread
```

## 🎯 Uso

### **Executar Testes Unitários**
```bash
make run
```

### **Executar Servidor**
```bash
make run-server
```

### **Executar Cliente**
```bash
make run-client
```

### **Testar com Múltiplos Clientes**
```bash
make test-clients
```

## 📊 Comandos do Makefile

```bash
make all           # Compila todos os componentes
make servidor      # Compila apenas o servidor
make cliente       # Compila apenas o cliente
make log_teste     # Compila apenas os testes
make clean         # Remove arquivos compilados e logs
make status        # Mostra status da compilação
make help          # Mostra todos os comandos disponíveis
```


## 🛠️ API da Biblioteca

### **Funções Disponíveis**
- `logger_t* log_init(const char *nomeArquivo)`
- `void log_escrever(logger_t *log, const char *mensagem)`
- `void log_escrever_verbose(logger_t *log, const char *mensagem)`
- `void log_set_verbose(logger_t *log, int verbose)`
- `void log_destruir(logger_t *log)`

## 📝 Características Técnicas

- **Porta**: 8080
- **Máx. Clientes**: 10 simultâneos
- **Threads**: pthreads para concorrência
- **Sincronização**: mutex para acesso seguro
- **Protocolo**: TCP/IP
- **Logs**: Timestamps no formato `DD-MM-YYYY HH:MM:SS`

## 🐛 Solução de Problemas

### **Porta já em uso**
```bash
# Liberar porta 8080
make clean-port
```

### **Problemas de compilação**
```bash
# Limpar e recompilar tudo
make rebuild
```
