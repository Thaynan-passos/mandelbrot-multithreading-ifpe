#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <algorithm>

// ============================================================================
// 3.2 Parâmetros e Funções Comuns de Cálculo
// ============================================================================
constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;
constexpr int MAX_ITER = 2000;
constexpr double X_MIN = -2.2, X_MAX = 1.0;
constexpr double Y_MIN = -1.0, Y_MAX = 1.0;
constexpr int NUM_RUNS = 5; // Número de repetições para cálculo da média

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

// ============================================================================
// 3.3 Implementação Sequencial
// ============================================================================
void mandelbrotSerial(Image& img) {
    for (int y = 0; y < HEIGHT; ++y) {
        computeRow(y, img);
    }
}

// ============================================================================
// 3.4 Implementação Concorrente (Distribuição Dinâmica via Contador Atômico)
// ============================================================================
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
    pool.reserve(numThreads);
    for (unsigned t = 0; t < numThreads; ++t) {
        pool.emplace_back(worker);
    }
    for (auto& th : pool) {
        th.join();
    }
}

// ============================================================================
// 3.5 Funções de Medição de Tempo e Verificação (Checksum FNV-1a 64 bits)
// ============================================================================
template <typename F>
double timeMs(F&& f) {
    auto t0 = std::chrono::steady_clock::now();
    f();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

uint64_t checksum(const Image& img) {
    uint64_t s = 1469598103934665603ULL;
    for (uint16_t v : img) {
        s ^= v;
        s *= 1099511628211ULL;
    }
    return s;
}

// ============================================================================
// Função Principal (Orquestração dos Testes e Geração da Tabela)
// ============================================================================
int main() {
    const size_t totalPixels = static_cast<size_t>(WIDTH) * HEIGHT;
    Image imgSerial(totalPixels, 0);
    Image imgParallel(totalPixels, 0);

    unsigned maxThreads = std::thread::hardware_concurrency();
    if (maxThreads == 0) maxThreads = 8; // Fallback caso o SO não informe

    std::cout << "=================================================================\n";
    std::cout << "BENCHMARK CONJUNTO DE MANDELBROT (C++17)\n";
    std::cout << "Resolucao: " << WIDTH << "x" << HEIGHT << " | MAX_ITER: " << MAX_ITER << "\n";
    std::cout << "Threads logicas detectadas (hardware_concurrency): " << maxThreads << "\n";
    std::cout << "Execucoes por configuracao: " << NUM_RUNS << "\n";
    std::cout << "=================================================================\n\n";

    // 1. Aquecimento (Warmup) para estabilizar cache e clock da CPU
    std::cout << "Executando aquecimento (warmup)...\n";
    mandelbrotSerial(imgSerial);
    uint64_t expectedChecksum = checksum(imgSerial);

    // 2. Medição da Versão Sequencial
    double totalSerialMs = 0.0;
    for (int run = 0; run < NUM_RUNS; ++run) {
        std::fill(imgSerial.begin(), imgSerial.end(), 0);
        double elapsed = timeMs([&]() {
            mandelbrotSerial(imgSerial);
        });
        totalSerialMs += elapsed;
    }
    double avgSerialMs = totalSerialMs / NUM_RUNS;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nTempo medio da versao sequencial: " << avgSerialMs << " ms\n";
    std::cout << "Checksum de referencia (FNV-1a): " << expectedChecksum << "\n\n";

    // 3. Definição da lista de threads a testar (1, 2, 4, 8... até maxThreads)
    std::vector<unsigned> threadCounts;
    for (unsigned t = 1; t < maxThreads; t *= 2) {
        threadCounts.push_back(t);
    }
    if (threadCounts.empty() || threadCounts.back() != maxThreads) {
        threadCounts.push_back(maxThreads);
    }

    // Cabeçalho da Tabela 2
    std::cout << "-----------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(10) << "Threads"
              << std::setw(18) << "Tempo medio (ms)"
              << std::setw(15) << "Speedup (S)"
              << std::setw(16) << "Eficiencia (E)"
              << "Correto\n";
    std::cout << "-----------------------------------------------------------------\n";

    // 4. Medição da Versão Concorrente
    for (unsigned numThreads : threadCounts) {
        double totalParallelMs = 0.0;
        bool allCorrect = true;

        for (int run = 0; run < NUM_RUNS; ++run) {
            std::fill(imgParallel.begin(), imgParallel.end(), 0);
            double elapsed = timeMs([&]() {
                mandelbrotParallel(imgParallel, numThreads);
            });
            totalParallelMs += elapsed;

            // Verifica integridade a cada execução
            if (checksum(imgParallel) != expectedChecksum) {
                allCorrect = false;
            }
        }

        double avgParallelMs = totalParallelMs / NUM_RUNS;
        double speedup = avgSerialMs / avgParallelMs;
        double efficiency = speedup / static_cast<double>(numThreads);

        std::cout << std::left
                  << std::setw(10) << numThreads
                  << std::setw(18) << avgParallelMs
                  << std::setw(15) << speedup
                  << std::setw(16) << efficiency
                  << (allCorrect ? "sim" : "NAO") << "\n";
    }
    std::cout << "-----------------------------------------------------------------\n";

    return 0;
}
