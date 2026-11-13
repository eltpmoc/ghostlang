#include "GhostMatrixG.hpp"
#include <stdexcept>

GhostMatrixG::GhostMatrixG(size_t r, size_t c) : rows(r), cols(c) {
    data.resize(r * c, {0.0f, 0.0f, 1.0f}); // real, imag, theta inicializado em 1.0
}

GhostMatrixG::GhostMatrixG(const std::vector<std::vector<GhostNumberG>>& legacy_matrix) {
    rows = legacy_matrix.size();
    cols = rows > 0 ? legacy_matrix[0].size() : 0;
    data.resize(rows * cols);
    for (size_t r = 0; r < rows; ++r) {
        if (legacy_matrix[r].size() != cols) throw std::invalid_argument("Matriz legacy irregular");
        for (size_t c = 0; c < cols; ++c) {
            set_ghost_number(r, c, legacy_matrix[r][c]);
        }
    }
}

GhostQubit& GhostMatrixG::at(size_t r, size_t c) {
    return data[r * cols + c];
}

const GhostQubit& GhostMatrixG::at(size_t r, size_t c) const {
    return data[r * cols + c];
}

GhostNumberG GhostMatrixG::get_ghost_number(size_t r, size_t c) const {
    const GhostQubit& q = at(r, c);
    GhostNumberG gn(q.real, q.imag, 0.0, q.theta); // Usando construtor (x, y, p, theta2)
    return gn;
}

void GhostMatrixG::set_ghost_number(size_t r, size_t c, const GhostNumberG& gn) {
    GhostQubit& q = at(r, c);
    q.real = (float)gn.get_real();
    q.imag = (float)gn.get_imag();
    q.theta = (float)gn.get_theta2();
}

GhostMatrixG GhostMatrixG::multiply(const GhostMatrixG& other) const {
    if (cols != other.rows) throw std::invalid_argument("Dimensoes incompativeis para Multiplicacao Quantica");
    
    GhostMatrixG result(rows, other.cols);
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < other.cols; ++j) {
            float sum_real = 0.0f;
            float sum_imag = 0.0f;
            float max_theta = 0.0f; // A incerteza se propaga pegando a maior divergência ou uma média
            
            for (size_t k = 0; k < cols; ++k) {
                const GhostQubit& a = at(i, k);
                const GhostQubit& b = other.at(k, j);
                
                // Multiplicação de Complexos Padrão
                float r = a.real * b.real - a.imag * b.imag;
                float im = a.real * b.imag + a.imag * b.real;
                
                sum_real += r;
                sum_imag += im;
                
                // Propagação de Theta (Interferência de Incerteza)
                float inter_theta = a.theta * b.theta;
                if (inter_theta > max_theta) max_theta = inter_theta;
            }
            
            GhostQubit& res = result.at(i, j);
            res.real = sum_real;
            res.imag = sum_imag;
            res.theta = max_theta == 0.0f ? 1.0f : max_theta;
        }
    }
    return result;
}
