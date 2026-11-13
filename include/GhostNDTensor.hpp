#pragma once

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <numeric>
#include <functional>

#include "GhostBLAS.hpp"

namespace GhostAlgebra {

class GhostNDTensor {
private:
    std::vector<int> m_shape;
    std::vector<int> m_strides;
    std::vector<double> m_data;

    void compute_strides() {
        m_strides.resize(m_shape.size());
        if (m_shape.empty()) return;
        int stride = 1;
        for (int i = (int)m_shape.size() - 1; i >= 0; --i) {
            m_strides[i] = stride;
            stride *= m_shape[i];
        }
    }

public:
    GhostNDTensor() {
        m_shape = {0};
        compute_strides();
    }

    explicit GhostNDTensor(const std::vector<int>& shape, double init_val = 0.0)
        : m_shape(shape) {
        compute_strides();
        int total = size();
        m_data.assign(total, init_val);
    }

    GhostNDTensor(const std::vector<int>& shape, const std::vector<double>& data)
        : m_shape(shape), m_data(data) {
        compute_strides();
        if ((int)m_data.size() != size()) {
            m_data.resize(size(), 0.0);
        }
    }

    const std::vector<int>& shape() const { return m_shape; }
    int ndim() const { return (int)m_shape.size(); }
    int size() const {
        if (m_shape.empty()) return 0;
        int total = 1;
        for (int s : m_shape) total *= s;
        return total;
    }
    const std::vector<double>& data() const { return m_data; }
    std::vector<double>& data() { return m_data; }

    int flat_index(const std::vector<int>& indices) const {
        if (indices.size() != m_shape.size()) {
            throw std::runtime_error("Numero de indices incompativel com ndim do tensor");
        }
        int idx = 0;
        for (size_t i = 0; i < indices.size(); ++i) {
            if (indices[i] < 0 || indices[i] >= m_shape[i]) {
                throw std::out_of_range("Indice de tensor fora dos limites");
            }
            idx += indices[i] * m_strides[i];
        }
        return idx;
    }

    double get(const std::vector<int>& indices) const {
        return m_data[flat_index(indices)];
    }

    void set(const std::vector<int>& indices, double val) {
        m_data[flat_index(indices)] = val;
    }

    double get_flat(int i) const {
        if (i < 0 || i >= (int)m_data.size()) throw std::out_of_range("Indice linear fora dos limites");
        return m_data[i];
    }

    void set_flat(int i, double val) {
        if (i < 0 || i >= (int)m_data.size()) throw std::out_of_range("Indice linear fora dos limites");
        m_data[i] = val;
    }

    // Reshape
    GhostNDTensor reshape(const std::vector<int>& new_shape) const {
        int new_total = 1;
        for (int s : new_shape) new_total *= s;
        if (new_total != size()) {
            throw std::runtime_error("Tamanho incompativel para reshape do tensor");
        }
        GhostNDTensor res = *this;
        res.m_shape = new_shape;
        res.compute_strides();
        return res;
    }

    // Transpose para 2D e N-D geral
    GhostNDTensor transpose(const std::vector<int>& perm) const {
        if ((int)perm.size() != ndim()) {
            throw std::runtime_error("Permutacao de eixos incompativel com ndim");
        }
        std::vector<int> new_shape(ndim());
        for (int i = 0; i < ndim(); ++i) {
            new_shape[i] = m_shape[perm[i]];
        }
        GhostNDTensor res(new_shape);
        std::vector<int> current_idx(ndim(), 0);
        int total = size();
        for (int i = 0; i < total; ++i) {
            // linear para multi-index de origem
            int rem = i;
            for (int d = 0; d < ndim(); ++d) {
                current_idx[d] = rem / m_strides[d];
                rem %= m_strides[d];
            }
            // mapeia para multi-index de destino
            std::vector<int> new_idx(ndim());
            for (int d = 0; d < ndim(); ++d) {
                new_idx[d] = current_idx[perm[d]];
            }
            res.set(new_idx, m_data[i]);
        }
        return res;
    }

    // Fatiamento (Slice) ao longo do primeiro eixo
    GhostNDTensor slice(int start, int end) const {
        if (ndim() == 0 || start < 0 || end > m_shape[0] || start >= end) {
            throw std::runtime_error("Intervalo de fatiamento invalido");
        }
        std::vector<int> new_shape = m_shape;
        new_shape[0] = end - start;
        GhostNDTensor res(new_shape);

        int slice_size = m_strides[0] * (end - start);
        int offset = start * m_strides[0];
        std::copy(m_data.begin() + offset, m_data.begin() + offset + slice_size, res.m_data.begin());
        return res;
    }

    // Regra de Broadcasting NumPy: alinhar formas da direita para a esquerda
    static bool can_broadcast(const std::vector<int>& a, const std::vector<int>& b, std::vector<int>& out_shape) {
        int n_a = (int)a.size();
        int n_b = (int)b.size();
        int max_dim = std::max(n_a, n_b);
        out_shape.assign(max_dim, 1);

        for (int i = 0; i < max_dim; ++i) {
            int dim_a = (i < n_a) ? a[n_a - 1 - i] : 1;
            int dim_b = (i < n_b) ? b[n_b - 1 - i] : 1;

            if (dim_a != dim_b && dim_a != 1 && dim_b != 1) {
                return false;
            }
            out_shape[max_dim - 1 - i] = std::max(dim_a, dim_b);
        }
        return true;
    }

    GhostNDTensor broadcast_to(const std::vector<int>& target_shape) const {
        int target_total = 1;
        for (int s : target_shape) target_total *= s;
        GhostNDTensor res(target_shape);

        int t_ndim = (int)target_shape.size();
        int s_ndim = (int)m_shape.size();
        std::vector<int> t_idx(t_ndim, 0);

        for (int i = 0; i < target_total; ++i) {
            // decompoe indice linear no shape alvo
            int rem = i;
            for (int d = 0; d < t_ndim; ++d) {
                t_idx[d] = rem / res.m_strides[d];
                rem %= res.m_strides[d];
            }
            // mapeia para indice no shape original usando broadcast
            std::vector<int> s_idx(s_ndim, 0);
            for (int d = 0; d < s_ndim; ++d) {
                int target_d = t_ndim - s_ndim + d;
                int dim_val = m_shape[d];
                s_idx[d] = (dim_val == 1) ? 0 : t_idx[target_d];
            }
            res.m_data[i] = get(s_idx);
        }
        return res;
    }

    // Operações element-wise com broadcasting
    GhostNDTensor elementwise_op(const GhostNDTensor& other, std::function<double(double, double)> op) const {
        std::vector<int> out_shape;
        if (!can_broadcast(m_shape, other.m_shape, out_shape)) {
            throw std::runtime_error("Shapes incompativeis para broadcasting de tensores");
        }
        GhostNDTensor b_a = broadcast_to(out_shape);
        GhostNDTensor b_b = other.broadcast_to(out_shape);

        GhostNDTensor res(out_shape);
        int n = res.size();
        for (int i = 0; i < n; ++i) {
            res.m_data[i] = op(b_a.m_data[i], b_b.m_data[i]);
        }
        return res;
    }

    GhostNDTensor add(const GhostNDTensor& other) const {
        return elementwise_op(other, [](double a, double b) { return a + b; });
    }

    GhostNDTensor sub(const GhostNDTensor& other) const {
        return elementwise_op(other, [](double a, double b) { return a - b; });
    }

    GhostNDTensor mul(const GhostNDTensor& other) const {
        return elementwise_op(other, [](double a, double b) { return a * b; });
    }

    GhostNDTensor div(const GhostNDTensor& other) const {
        return elementwise_op(other, [](double a, double b) {
            if (b == 0.0) return 0.0;
            return a / b;
        });
    }

    // Multiplicação de matrizes 2D / Batched MatMul com backend BLAS acelerado
    GhostNDTensor matmul(const GhostNDTensor& other) const {
        if (ndim() == 2 && other.ndim() == 2) {
            int m = m_shape[0];
            int k = m_shape[1];
            int k2 = other.m_shape[0];
            int n = other.m_shape[1];
            if (k != k2) {
                throw std::runtime_error("Dimensoes internas incompativeis para matmul");
            }
            GhostNDTensor res({m, n});
            GhostBLAS::dgemm(m, n, k, 1.0, m_data.data(), k, other.m_data.data(), n, 0.0, res.m_data.data(), n);
            return res;
        }
        throw std::runtime_error("Matmul suportado para 2D");
    }

    double sum() const {
        return std::accumulate(m_data.begin(), m_data.end(), 0.0);
    }

    double mean() const {
        if (m_data.empty()) return 0.0;
        return sum() / m_data.size();
    }

    std::string to_string() const {
        std::ostringstream ss;
        ss << "Tensor[shape=(";
        for (size_t i = 0; i < m_shape.size(); ++i) {
            ss << m_shape[i] << (i + 1 < m_shape.size() ? ", " : "");
        }
        ss << "), size=" << size() << "]";
        return ss.str();
    }
};

} // namespace GhostAlgebra

// C API bindings
extern "C" {
int ghost_tensor_create(const int* shape, int ndim, double init_val);
int ghost_tensor_from_data(const int* shape, int ndim, const double* data, int data_len);
void ghost_tensor_free(int handle);
int ghost_tensor_ndim(int handle);
int ghost_tensor_dim(int handle, int axis);
int ghost_tensor_size(int handle);
double ghost_tensor_get(int handle, const int* indices, int idx_len);
void ghost_tensor_set(int handle, const int* indices, int idx_len, double val);
double ghost_tensor_get_flat(int handle, int i);
void ghost_tensor_set_flat(int handle, int i, double val);
int ghost_tensor_reshape(int handle, const int* new_shape, int new_ndim);
int ghost_tensor_transpose(int handle, const int* perm, int perm_len);
int ghost_tensor_slice(int handle, int start, int end);
int ghost_tensor_add(int hA, int hB);
int ghost_tensor_sub(int hA, int hB);
int ghost_tensor_mul(int hA, int hB);
int ghost_tensor_div(int hA, int hB);
int ghost_tensor_matmul(int hA, int hB);
double ghost_tensor_sum(int handle);
double ghost_tensor_mean(int handle);
const char* ghost_tensor_to_string(int handle);
}

// Sobrecargas C++ para tipos nativos do compilador GhostLang
template <typename T, typename = std::enable_if_t<!std::is_pointer_v<std::decay_t<T>>>>
inline int ghost_tensor_create(const T& shape, int ndim, double init_val) {
    std::vector<int> shp;
    for (size_t i = 0; i < shape.size(); ++i) {
        shp.push_back(static_cast<int>(shape[i]));
    }
    return ghost_tensor_create(shp.data(), ndim, init_val);
}

template <typename T, typename = std::enable_if_t<!std::is_pointer_v<std::decay_t<T>>>>
inline double ghost_tensor_get(int handle, const T& indices, int idx_len) {
    std::vector<int> idx;
    for (size_t i = 0; i < indices.size(); ++i) {
        idx.push_back(static_cast<int>(indices[i]));
    }
    return ghost_tensor_get(handle, idx.data(), idx_len);
}

template <typename T, typename = std::enable_if_t<!std::is_pointer_v<std::decay_t<T>>>>
inline void ghost_tensor_set(int handle, const T& indices, int idx_len, double val) {
    std::vector<int> idx;
    for (size_t i = 0; i < indices.size(); ++i) {
        idx.push_back(static_cast<int>(indices[i]));
    }
    ghost_tensor_set(handle, idx.data(), idx_len, val);
}

template <typename T, typename = std::enable_if_t<!std::is_pointer_v<std::decay_t<T>>>>
inline int ghost_tensor_reshape(int handle, const T& new_shape, int new_ndim) {
    std::vector<int> shp;
    for (size_t i = 0; i < new_shape.size(); ++i) {
        shp.push_back(static_cast<int>(new_shape[i]));
    }
    return ghost_tensor_reshape(handle, shp.data(), new_ndim);
}

template <typename T, typename = std::enable_if_t<!std::is_pointer_v<std::decay_t<T>>>>
inline int ghost_tensor_transpose(int handle, const T& perm, int perm_len) {
    std::vector<int> p;
    for (size_t i = 0; i < perm.size(); ++i) {
        p.push_back(static_cast<int>(perm[i]));
    }
    return ghost_tensor_transpose(handle, p.data(), perm_len);
}

