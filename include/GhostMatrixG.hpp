#ifndef GHOST_MATRIX_G_HPP
#define GHOST_MATRIX_G_HPP

#include "GhostNumberG.hpp"
#include "GhostTransformerG.hpp" // Para acessar GhostQubit
#include <vector>
#include <string>
#include <iostream>

// Matriz de Estados Quânticos otimizada para o Transformer V5
class GhostMatrixG {
private:
    size_t rows;
    size_t cols;
    // Utilizamos um buffer contíguo 1D para máxima performance em CPU e fácil envio para CUDA (GPU)
    std::vector<GhostQubit> data;

public:
    GhostMatrixG(size_t r, size_t c);
    
    // Construtor a partir de vetores 2D antigos (compatibilidade com temps_phathon/GOPMatrix)
    GhostMatrixG(const std::vector<std::vector<GhostNumberG>>& legacy_matrix);

    size_t row_count() const { return rows; }
    size_t col_count() const { return cols; }
    
    // Acesso aos Qubits (usando indexação 1D: r * cols + c)
    GhostQubit& at(size_t r, size_t c);
    const GhostQubit& at(size_t r, size_t c) const;

    // Converte GhostQubit de volta para GhostNumberG caso o usuário da GhostLang queira imprimir
    GhostNumberG get_ghost_number(size_t r, size_t c) const;
    void set_ghost_number(size_t r, size_t c, const GhostNumberG& gn);

    // Aliases para o compilador GhostLang
    GhostNumberG get(size_t r, size_t c) const { return get_ghost_number(r, c); }
    void set(size_t r, size_t c, const GhostNumberG& gn) { set_ghost_number(r, c, gn); }

    // Multiplicação de Matriz Quântica: Combina real, imag e theta (Fase 4)
    GhostMatrixG multiply(const GhostMatrixG& other) const;
    
    // Para envio direto ao CUDA
    const GhostQubit* data_ptr() const { return data.data(); }
    GhostQubit* data_ptr() { return data.data(); }
    size_t size_bytes() const { return data.size() * sizeof(GhostQubit); }
};

inline GhostMatrixG matrix_g(int r, int c) {
    return GhostMatrixG(r, c);
}

#endif // GHOST_MATRIX_G_HPP
