#include "GhostTransformer.hpp"
#include "GhostTransformerG.hpp"
#include "GhostDevice.hpp"
#include <iostream>
#include <chrono>
#include <cassert>
#include <iomanip>

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  Benchmark: Transformer Clássico (V4) vs TransformerG (V5)" << std::endl;
    std::cout << "==========================================================" << std::endl;

    const int ctx = 32;
    const int emb = 16;
    const int layers = 2;
    const int heads = 2;
    const int hidden = 32;

    // 1. Instanciação dos Modelos
    std::cout << "[1] Instanciando modelos V4 e V5..." << std::endl;
    GhostTransformer model_v4(ctx, emb, layers, heads, hidden, 42);
    GhostTransformerG model_v5(ctx, emb, layers, heads, hidden, 42);

    // 2. Benchmark de Auto-teste e Atencao
    std::cout << "[2] Benchmark de Attention Kernel & Self-Test..." << std::endl;
    
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        model_v4.self_test();
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double v4_us = std::chrono::duration<double, std::micro>(t1 - t0).count() / 5.0;

    auto t2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        model_v5.self_test();
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    double v5_us = std::chrono::duration<double, std::micro>(t3 - t2).count() / 5.0;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  -> Transformer V4 (Clássico/Float): " << v4_us << " us / step" << std::endl;
    std::cout << "  -> TransformerG V5 (Quântico/Qubit): " << v5_us << " us / step" << std::endl;

    // 3. Benchmark de Geração de Tokens
    std::cout << "[3] Benchmark de Geração de Sequência..." << std::endl;
    auto t4 = std::chrono::high_resolution_clock::now();
    auto res_v5 = model_v5.generate_checked("GhostLang IA", 8);
    auto t5 = std::chrono::high_resolution_clock::now();
    double gen_us = std::chrono::duration<double, std::micro>(t5 - t4).count();

    std::cout << "  -> TransformerG V5 gerou " << res_v5.generated_bytes 
              << " bytes em " << gen_us << " us (Status: " << res_v5.reason << ")" << std::endl;

    std::cout << "==========================================================" << std::endl;
    std::cout << "Benchmark concluído com sucesso!" << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
