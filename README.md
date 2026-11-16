# ANÁLISE COMPARATIVA DE DESEMPENHO ENTRE EXECUÇÃO SEQUENCIAL E MULTITHREADING NO CÁLCULO DO CONJUNTO DE MANDELBROT UTILIZANDO C++

**Instituição:** Instituto Federal de Educação, Ciência e Tecnologia de Pernambuco (IFPE) – Campus Paulista  
**Curso:** Bacharelado em Análise e Desenvolvimento de Sistemas (ADS 2026.2)  
**Disciplina:** Sistemas Operacionais  
**Docente:** Prof. Mércio Antônio O. de Andrade  
**Autores:** Thaynan D'avila dos Passos; Gabriel Verrisimo; Matheus de Morais  

---

## RESUMO
O conjunto de Mandelbrot é um problema clássico de computação intensiva em que cada ponto do plano complexo pode ser avaliado de forma independente, o que o torna adequado para o estudo de paralelismo. Este trabalho implementa em C++17 duas soluções para o cálculo de uma imagem de 1920×1080 pontos: uma versão estritamente sequencial e uma versão concorrente baseada em múltiplas threads (`std::thread`), com distribuição dinâmica de linhas por meio de um contador atômico. As duas versões são comparadas quanto ao tempo de execução, ao ganho de velocidade (*speedup*) e à eficiência, variando o número de threads, e a correção da versão paralela é verificada por meio de um checksum da imagem gerada. Os resultados são analisados à luz da Lei de Amdahl e de fatores como o desbalanceamento de carga, a criação de threads e a diferença entre núcleos físicos e lógicos.

**Palavras-chave:** Programação concorrente; Multithreading; Conjunto de Mandelbrot; Speedup; C++.

---

## 1. INTRODUÇÃO
Processadores modernos possuem vários núcleos, mas um programa escrito de forma sequencial utiliza apenas um deles por vez. Para aproveitar o hardware disponível, é necessário decompor o problema em partes que possam ser executadas simultaneamente, o que exige compreender como o sistema operacional escalona threads e como o hardware impõe limites ao ganho obtido.

O cálculo do conjunto de Mandelbrot é um bom candidato para esse estudo. Para cada ponto $c$ do plano complexo, aplica-se repetidamente a iteração $z_{n+1} = z_n^2 + c$, a partir de $z_0 = 0$, e conta-se quantas iterações são necessárias até que o módulo de $z$ ultrapasse 2. Como o resultado de um ponto não depende dos demais, o problema é classificado como "embaraçosamente paralelo". Ao mesmo tempo, o custo por ponto varia muito: pontos que pertencem ao conjunto exigem o número máximo de iterações, enquanto pontos distantes escapam rapidamente. Essa irregularidade permite discutir também o balanceamento de carga entre threads.

O objetivo deste trabalho é implementar uma solução sequencial e uma concorrente para o mesmo problema, medir o tempo de execução de ambas e avaliar o *speedup* e a eficiência obtidos, discutindo os fatores que impedem um ganho linear.

---

## 2. REFERENCIAL TEÓRICO

### 2.1 O Conjunto de Mandelbrot
O conjunto de Mandelbrot, estudado por Benoît Mandelbrot na década de 1980, é formado pelos números complexos $c$ para os quais a sequência $z_0 = 0, z_{n+1} = z_n^2 + c$ permanece limitada. Na prática, considera-se que a sequência diverge quando $|z_n| > 2$, e limita-se o cálculo a um número máximo de iterações (`MAX_ITER`). Pontos que atingem esse limite sem divergir são tratados como pertencentes ao conjunto (Mandelbrot, 1980; Peitgen; Jürgens; Saupe, 1992).

A imagem é gerada associando cada pixel a um ponto do plano complexo e colorindo-o conforme o número de iterações necessárias. O custo computacional total depende da resolução, da região observada e de `MAX_ITER`.

### 2.2 Programação Concorrente e Threads
Uma thread é a menor unidade de execução escalonável pelo sistema operacional. Threads de um mesmo processo compartilham o espaço de endereçamento, o que facilita a divisão de dados, mas exige cuidado para evitar condições de corrida. A partir do C++11, a biblioteca padrão oferece `<thread>` e `<atomic>`, que permitem criar threads e realizar operações atômicas de forma portável (Williams, 2019). A função `std::thread::hardware_concurrency()` informa o número de threads de hardware (núcleos lógicos) disponíveis.

### 2.3 Balanceamento de Carga
Uma estratégia simples de paralelização é a divisão estática: o trabalho é repartido em blocos contíguos de tamanho igual, um por thread. Quando o custo por unidade de trabalho é irregular, como no Mandelbrot, algumas threads terminam antes de outras e ficam ociosas, o que reduz o ganho. Uma alternativa é a divisão dinâmica, na qual as threads retiram pequenas unidades de trabalho (por exemplo, uma linha da imagem) de uma fila compartilhada à medida que ficam livres. Neste trabalho, a fila é implementada por um contador atômico incrementado com `fetch_add`, sem uso de mutex.

### 2.4 Métricas de Desempenho e Lei de Amdahl
O *speedup* ($S$) é a razão entre o tempo da versão sequencial ($T_{serial}$) e o tempo da versão paralela com $N$ threads ($T_{paralelo}$):
$$S = \frac{T_{serial}}{T_{paralelo}}$$

A eficiência ($E$) indica o quanto cada unidade de processamento é efetivamente aproveitada:
$$E = \frac{S}{N}$$

A Lei de Amdahl (Amdahl, 1967) estabelece que, se $f$ é a fração do programa que pode ser paralelizada, o *speedup* máximo com $N$ processadores é limitado por:
$$S_{max}(N) = \frac{1}{(1 - f) + \frac{f}{N}}$$

Assim, mesmo uma pequena fração sequencial limita o ganho total. A Lei de Gustafson (Gustafson, 1988) complementa essa visão ao observar que, na prática, o tamanho do problema costuma crescer junto com o número de processadores.

---

## 3. METODOLOGIA E DESENVOLVIMENTO

### 3.1 Descrição do Problema
O problema consiste em calcular, para uma imagem de 1920×1080 pixels, o número de iterações de cada ponto da região retangular do plano complexo definida por $\text{Re}(c) \in [-2.2; 1.0]$ e $\text{Im}(c) \in [-1.0; 1.0]$, com `MAX_ITER = 2000`. O resultado é armazenado em um vetor de inteiros de 16 bits, com um valor por pixel.

### 3.2 Função Comum de Cálculo
As duas versões utilizam exatamente as mesmas funções de cálculo, de modo que a única diferença entre elas é a forma de percorrer as linhas da imagem.

```cpp
constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;
constexpr int MAX_ITER = 2000;
constexpr double X_MIN = -2.2, X_MAX = 1.0;
constexpr double Y_MIN = -1.0, Y_MAX = 1.0;

using Image = std::vector<uint16_t>;

inline uint16_t mandelbrotPoint(double cr, double ci) {
    double zr = 0.0, zi = 0.0;
    int n = 0;
    while (n < MAX_ITER && zr * zr + zi * zi <= 4.0) {
        double tmp = zr * zr - zi * zi + cr;
        zi = 2.0 * zr * zi + ci;
        zr = tmp;
        ++n;
    }
    return static_cast<uint16_t>(n);
}

inline void computeRow(int y, Image& img) {
    double ci = Y_MIN + (Y_MAX - Y_MIN) * y / (HEIGHT - 1);
    for (int x = 0; x < WIDTH; ++x) {
        double cr = X_MIN + (X_MAX - X_MIN) * x / (WIDTH - 1);
        img[static_cast<size_t>(y) * WIDTH + x] = mandelbrotPoint(cr, ci);
    }
}
```

### 3.3 Implementação Sequencial
A versão sequencial percorre todas as linhas da imagem, uma após a outra, em uma única thread.

```cpp
void mandelbrotSerial(Image& img) {
    for (int y = 0; y < HEIGHT; ++y)
        computeRow(y, img);
}
```

### 3.4 Implementação Concorrente
Na versão concorrente, um conjunto de $N$ threads é criado. Cada thread solicita a próxima linha ainda não calculada por meio de um contador atômico compartilhado (`nextRow`). Como cada linha é escrita por uma única thread e em posições distintas do vetor, não há condição de corrida sobre a imagem, e não é necessário utilizar mutex. A distribuição dinâmica compensa o fato de que linhas próximas ao centro da imagem custam bem mais que as das bordas.

```cpp
void mandelbrotParallel(Image& img, unsigned numThreads) {
    std::atomic<int> nextRow{0};

    auto worker = [&]() {
        for (;;) {
            int y = nextRow.fetch_add(1, std::memory_order_relaxed);
            if (y >= HEIGHT) break;
            computeRow(y, img);
        }
    };

    std::vector<std::thread> pool;
    for (unsigned t = 0; t < numThreads; ++t)
        pool.emplace_back(worker);
    for (auto& th : pool)
        th.join();
}
```

### 3.5 Medição e Verificação
O tempo é medido com `std::chrono::steady_clock`, restrito ao cálculo da imagem (sem incluir gravação em arquivo). Antes das medições, a versão sequencial é executada uma vez para aquecimento de cache e do escalonador. Cada configuração é executada várias vezes (por padrão, 5), e o tempo reportado é a média. Para verificar a correção, calcula-se um checksum (FNV-1a de 64 bits) sobre a imagem da versão sequencial e compara-se com o checksum da imagem produzida pela versão concorrente em cada execução; qualquer divergência é sinalizada no relatório.

```cpp
template <typename F>
double timeMs(F&& f) {
    auto t0 = std::chrono::steady_clock::now();
    f();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

uint64_t checksum(const Image& img) {
    uint64_t s = 1469598103934665603ULL;
    for (uint16_t v : img) { s ^= v; s *= 1099511628211ULL; }
    return s;
}
```

#### Compilação
- **GCC / Clang:** `g++ -O2 -std=c++17 -pthread src/main.cpp -o mandelbrot`
- **MSVC (Visual Studio):** `cl /O2 /std:c++17 /EHsc src/main.cpp`

### 3.6 Ambiente Experimental
**Tabela 1:** Especificações do ambiente de teste

| Componente | Especificação |
| :--- | :--- |
| **Processador (CPU)** | Intel Core i7-12700H (14 núcleos / 20 threads) |
| **Memória RAM** | 32 GB DDR5 |
| **Sistema Operacional** | Microsoft Windows 11 Pro 64 bits |
| **Compilador** | MSVC (Visual Studio 2022), modo Release, /O2 |
| **Padrão da Linguagem** | C++17 |

---

## 4. RESULTADOS E DISCUSSÃO

> **Atenção:** Os valores da Tabela 2 devem ser preenchidos com os dados reais de benchmark obtidos na máquina em modo de alto desempenho.

**Tempo médio da versão sequencial (1920×1080, MAX_ITER = 2000):** `[___]` ms.

**Tabela 2:** Desempenho da versão concorrente em função do número de threads

| Threads | Tempo médio (ms) | Speedup ($S$) | Eficiência ($E$) | Resultado correto |
| :---: | :---: | :---: | :---: | :---: |
| 1 | [___] | [___] | [___] | [sim/não] |
| 2 | [___] | [___] | [___] | [sim/não] |
| 4 | [___] | [___] | [___] | [sim/não] |
| 8 | [___] | [___] | [___] | [sim/não] |
| 16 | [___] | [___] | [___] | [sim/não] |
| 20 | [___] | [___] | [___] | [sim/não] |

### 4.1 Análise de Desempenho e Speedup
*(Descrever, com base na Tabela 2, como o tempo diminui à medida que o número de threads aumenta, qual foi o maior speedup obtido e com quantas threads. Comparar o resultado com o speedup ideal).*

### 4.2 Fatores que Limitam o Ganho
- **Fração sequencial e Lei de Amdahl:** A criação e a junção (*join*) das threads, o aquecimento e a verificação são partes não paralelizáveis.
- **Núcleos físicos versus lógicos:** Em processadores com Hyper-Threading, as threads lógicas compartilham unidades de execução. O ganho ao transitar de núcleos físicos para lógicos tende a saturar.
- **Processadores híbridos:** O processador combina núcleos de desempenho (P-cores) e de eficiência (E-cores). A distribuição dinâmica com atômicos mitiga o descompasso de velocidades.
- **Balanceamento de carga:** A fila dinâmica evita que threads alocadas nas bordas da imagem fiquem ociosas aguardando threads responsáveis pelo centro.
- **Frequência e temperatura:** Oscilações de *thermal throttling* em notebooks justificam a realização de múltiplas baterias de testes com extração de médias.
- **Concorrência do SO:** Interferência do escalonador e processos concorrentes em segundo plano.

---

## 5. CONSIDERAÇÕES FINAIS
Este trabalho apresentou duas soluções em C++ para o cálculo do conjunto de Mandelbrot, uma sequencial e outra concorrente com distribuição dinâmica de linhas, garantindo a exatidão dos resultados por meio de checksum. A abordagem ilustra empiricamente o impacto da programação paralela e as fronteiras impostas pelas Leis de Amdahl e características de arquitetura de processadores modernos.

---

## REFERÊNCIAS
- **AMDAHL, Gene M.** Validity of the single processor approach to achieving large scale computing capabilities. In: AFIPS '67 (Spring): Proceedings of the April 18-20, 1967, spring joint computer conference, Atlantic City, New Jersey, p. 483–485, 1967. DOI: 10.1145/1465482.1465560.
- **GUSTAFSON, John L.** Reevaluating Amdahl's law. Communications of the ACM, v. 31, n. 5, p. 532–533, 1988.
- **MANDELBROT, Benoît B.** Fractal aspects of the iteration of $z \to \lambda z(1-z)$ for complex $\lambda$ and $z$. Annals of the New York Academy of Sciences, v. 357, p. 249–259, 1980.
- **PEITGEN, Heinz-Otto; JÜRGENS, Hartmut; SAUPE, Dietmar.** Chaos and Fractals: New Frontiers of Science. New York: Springer-Verlag, 1992.
- **SILBERSCHATZ, Abraham; GALVIN, Peter B.; GAGNE, Greg.** Operating System Concepts. 10. ed. Hoboken: John Wiley & Sons, 2018.
- **WILLIAMS, Anthony.** C++ Concurrency in Action. 2. ed. Shelter Island: Manning Publications, 2019.
