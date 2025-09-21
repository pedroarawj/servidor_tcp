Descrição
A biblioteca libtslog segue um padrão para garantir uma única instância global do logger, combinado com um Monitor (através da mutex) para garantir thread-safety.

Componentes Principais:

Estrutura logger_t: Armazena o handle para o arquivo de log (FILE*) e um mutex (pthread_mutex_t) para serializar o acesso concorrente.

Função log_init: Atua como construtor do Singleton. Inicializa a estrutura logger_t, abre o arquivo no modo "append" e inicializa o mutex.

Função log_escrever: É o método principal. Adquire o lock da mutex, escreve a mensagem no arquivo, garante que ela é flushed e libera o lock. Este é o ponto de serialização que impede que mensagens de threads diferentes se intercalem.

Função log_destruir: Destrutor. Libera todos os recursos alocados (mutex, handle do arquivo, memória da estrutura).

Fluxo de Dados:

A aplicação cliente (e.g., log_teste.c) chama log_init para obter a instância do logger.

As threads da aplicação chamam log_escrever, passando a mensagem.

Internamente, log_escrever serializa o acesso ao arquivo e escreve a mensagem.

No final da aplicação, log_destruir é chamado para fechar o arquivo e limpar os recursos.

Compilação 

Pré-requisitos
GCC 
Make
Biblioteca pthread

Clone ou baixe o projeto
git clone <url-do-repositorio>
cd libtslog

Compile o projeto
make

Limpar arquivos compilados
make clean

Limpar e recompilar tudo
make rebuild

Compilação manual sem make

Criar pasta build
mkdir -p build

Compilar a biblioteca
gcc -Wall -Wextra -pedantic -g -I./include -c src/libtslog.c -o build/libtslog.o

Compilar e linkar o teste
gcc -Wall -Wextra -pedantic -g -I./include test/log_teste.c build/libtslog.o -o build/log_teste -lpthread

Executar
./build/log_teste