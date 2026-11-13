#include "GhostTransformerG.hpp"
#include "GhostMatrixG.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
    std::cout << "=== Teste C++ GhostTransformerG (IA Propria V5) ===" << std::endl;

    // 1. Instancia o TransformerG com dimensao compativel com self-test
    GhostTransformerG model(32, 16, 2, 2, 2, 42);
    std::cout << "[V5] Modelo instanciado com sucesso. Contexto: " << model.context_size() << std::endl;
    assert(model.context_size() == 32);

    // 2. Executa auto-teste de kernels de atencao quantica e GEMM de Qubits
    std::cout << "[V5] Executando self_test() de atencao e propagacao quantica..." << std::endl;
    model.self_test();

    // 3. Testa geracao de texto controlada (generate_checked)
    std::cout << "[V5] Testando geracao com pesos quânticos..." << std::endl;
    auto res = model.generate_checked("GhostLang", 16);
    std::cout << "[V5] Geracao executada: bytes=" << res.generated_bytes 
              << " motivo=" << res.reason << std::endl;

    // 4. Testa algebra direta de GhostMatrixG (Qubits POD: r, i, theta)
    GhostMatrixG m1(4, 4);
    GhostMatrixG m2(4, 4);
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            m1.at(r, c) = { static_cast<float>(r + 1), static_cast<float>(c + 1), 0.5f };
            m2.at(r, c) = { 1.0f, 0.0f, 0.1f };
        }
    }
    assert(m1.row_count() == 4 && m1.col_count() == 4);
    assert(m1.at(0, 0).real == 1.0f);
    assert(m1.at(0, 0).imag == 1.0f);
    assert(m1.at(0, 0).theta == 0.5f);

    std::cout << "=== Todos os testes do GhostTransformerG passaram com sucesso! ===" << std::endl;
    return 0;
}
