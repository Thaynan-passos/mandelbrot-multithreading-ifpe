PASTA: results/
FINALIDADE: Armazenamento dos dados empíricos e resultados dos benchmarks.

ARQUIVOS QUE FICARÃO AQUI DENTRO:
- benchmark_output.txt:
  Captura textual bruta da saída do terminal após a execução do binário
  em modo Release na máquina de teste (tempos médios e confirmações de checksum).

- tabela_resultados.csv:
  Dados consolidados e tabulados em formato CSV contendo colunas:
  Threads, Tempo_ms, Speedup, Eficiencia, Valido.

- grafico_speedup.png:
  Gráfico gerado a partir do CSV comparando a curva de Speedup Real obtida
  em relação à reta do Speedup Ideal (S = N) e curva de eficiência.
