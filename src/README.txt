PASTA: src/
FINALIDADE: Código-fonte em C++17 do projeto.

ARQUIVOS QUE FICARÃO AQUI DENTRO:
- main.cpp:
  Ponto de entrada do programa. Orquestra a fase de aquecimento (warmup),
  as baterias de testes sequencial e concorrente (com 1, 2, 4, 8... threads)
  e imprime a tabela comparativa final de resultados.

- mandelbrot.hpp:
  Cabeçalho compartilhado contendo as constantes geométricas e de resolução
  (WIDTH = 1920, HEIGHT = 1080, MAX_ITER = 2000, limites Re e Im) e as assinaturas
  das funções.

- mandelbrot_serial.cpp:
  Implementação de referência estritamente sequencial (single-thread), processando
  as linhas da imagem de forma linear.

- mandelbrot_parallel.cpp:
  Implementação concorrente utilizando std::thread com distribuição dinâmica de
  linhas através de um contador atômico (std::atomic<int> com fetch_add), evitando
  condições de corrida sem necessidade de mutex.

- utils.hpp:
  Funções auxiliares para medição de tempo de alta precisão (std::chrono::steady_clock)
  e cálculo do hash de integridade FNV-1a de 64 bits (checksum) para validar que
  o resultado paralelo é idêntico ao sequencial.
