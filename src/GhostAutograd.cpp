#include "../include/GhostAutograd.hpp"

#include <set>
#include <map>
#include <mutex>
#include <algorithm>

namespace GhostAI {

void GhostVar::backward() {
    // 1. Ordenação topológica do grafo computacional reverso
    std::vector<VarPtr> topo;
    std::set<GhostVar*> visited;

    std::function<void(const VarPtr&)> build_topo = [&](const VarPtr& node) {
        if (!node || visited.count(node.get())) return;
        visited.insert(node.get());
        for (const auto& parent : node->parents) {
            build_topo(parent);
        }
        topo.push_back(node);
    };

    build_topo(shared_from_this());

    // 2. O gradiente de partida na perda escalar é 1.0
    this->grad = 1.0;

    // 3. Execução das derivadas em ordem reversa
    for (auto it = topo.rbegin(); it != topo.rend(); ++it) {
        if ((*it)->backward_fn) {
            (*it)->backward_fn();
        }
    }
}

void GhostVar::zero_grad() {
    this->grad = 0.0;
}

VarPtr make_var(double val, bool req_grad) {
    return std::make_shared<GhostVar>(val, req_grad, "leaf");
}

VarPtr var_add(const VarPtr& a, const VarPtr& b) {
    auto res = std::make_shared<GhostVar>(a->val + b->val, a->requires_grad || b->requires_grad, "add");
    res->parents = { a, b };
    res->backward_fn = [a, b, res]() {
        if (a->requires_grad) a->grad += res->grad;
        if (b->requires_grad) b->grad += res->grad;
    };
    return res;
}

VarPtr var_sub(const VarPtr& a, const VarPtr& b) {
    auto res = std::make_shared<GhostVar>(a->val - b->val, a->requires_grad || b->requires_grad, "sub");
    res->parents = { a, b };
    res->backward_fn = [a, b, res]() {
        if (a->requires_grad) a->grad += res->grad;
        if (b->requires_grad) b->grad -= res->grad;
    };
    return res;
}

VarPtr var_mul(const VarPtr& a, const VarPtr& b) {
    auto res = std::make_shared<GhostVar>(a->val * b->val, a->requires_grad || b->requires_grad, "mul");
    res->parents = { a, b };
    res->backward_fn = [a, b, res]() {
        if (a->requires_grad) a->grad += res->grad * b->val;
        if (b->requires_grad) b->grad += res->grad * a->val;
    };
    return res;
}

VarPtr var_relu(const VarPtr& a) {
    double v = (a->val > 0.0) ? a->val : 0.0;
    auto res = std::make_shared<GhostVar>(v, a->requires_grad, "relu");
    res->parents = { a };
    res->backward_fn = [a, res]() {
        if (a->requires_grad) {
            a->grad += (a->val > 0.0) ? res->grad : 0.0;
        }
    };
    return res;
}

VarPtr var_sigmoid(const VarPtr& a) {
    double s = 1.0 / (1.0 + std::exp(-a->val));
    auto res = std::make_shared<GhostVar>(s, a->requires_grad, "sigmoid");
    res->parents = { a };
    res->backward_fn = [a, s, res]() {
        if (a->requires_grad) {
            a->grad += res->grad * s * (1.0 - s);
        }
    };
    return res;
}

VarPtr var_tanh(const VarPtr& a) {
    double t = std::tanh(a->val);
    auto res = std::make_shared<GhostVar>(t, a->requires_grad, "tanh");
    res->parents = { a };
    res->backward_fn = [a, t, res]() {
        if (a->requires_grad) {
            a->grad += res->grad * (1.0 - t * t);
        }
    };
    return res;
}

// -------------------------------------------------------------
// IMPLEMENTAÇÃO DE GHOST TENSOR
// -------------------------------------------------------------

GhostTensor::GhostTensor(size_t r, size_t c, bool req_grad, double init_val)
    : rows(r), cols(c) {
    data.reserve(r * c);
    for (size_t i = 0; i < r * c; ++i) {
        data.push_back(make_var(init_val, req_grad));
    }
}

GhostTensor::GhostTensor(size_t r, size_t c, const std::vector<double>& vals, bool req_grad)
    : rows(r), cols(c) {
    data.reserve(r * c);
    for (size_t i = 0; i < r * c; ++i) {
        double v = (i < vals.size()) ? vals[i] : 0.0;
        data.push_back(make_var(v, req_grad));
    }
}

void GhostTensor::zero_grad() {
    for (auto& v : data) {
        v->zero_grad();
    }
}

void GhostTensor::step(double lr) {
    for (auto& v : data) {
        if (v->requires_grad) {
            v->val -= lr * v->grad;
        }
    }
}

std::shared_ptr<GhostTensor> GhostTensor::add(const std::shared_ptr<GhostTensor>& a, const std::shared_ptr<GhostTensor>& b) {
    if (a->rows != b->rows || a->cols != b->cols) {
        throw std::runtime_error("Dimensoes incompativeis para adicao de tensores");
    }
    auto out = std::make_shared<GhostTensor>();
    out->rows = a->rows;
    out->cols = a->cols;
    out->data.reserve(a->data.size());
    for (size_t i = 0; i < a->data.size(); ++i) {
        out->data.push_back(var_add(a->data[i], b->data[i]));
    }
    return out;
}

std::shared_ptr<GhostTensor> GhostTensor::sub(const std::shared_ptr<GhostTensor>& a, const std::shared_ptr<GhostTensor>& b) {
    if (a->rows != b->rows || a->cols != b->cols) {
        throw std::runtime_error("Dimensoes incompativeis para subtracao de tensores");
    }
    auto out = std::make_shared<GhostTensor>();
    out->rows = a->rows;
    out->cols = a->cols;
    out->data.reserve(a->data.size());
    for (size_t i = 0; i < a->data.size(); ++i) {
        out->data.push_back(var_sub(a->data[i], b->data[i]));
    }
    return out;
}

std::shared_ptr<GhostTensor> GhostTensor::matmul(const std::shared_ptr<GhostTensor>& a, const std::shared_ptr<GhostTensor>& b) {
    if (a->cols != b->rows) {
        throw std::runtime_error("Dimensoes incompativeis para multiplicacao matricial");
    }
    auto out = std::make_shared<GhostTensor>();
    out->rows = a->rows;
    out->cols = b->cols;
    out->data.resize(a->rows * b->cols);

    for (size_t i = 0; i < a->rows; ++i) {
        for (size_t j = 0; j < b->cols; ++j) {
            VarPtr sum = var_mul(a->at(i, 0), b->at(0, j));
            for (size_t k = 1; k < a->cols; ++k) {
                sum = var_add(sum, var_mul(a->at(i, k), b->at(k, j)));
            }
            out->data[i * b->cols + j] = sum;
        }
    }
    return out;
}

std::shared_ptr<GhostTensor> GhostTensor::relu(const std::shared_ptr<GhostTensor>& a) {
    auto out = std::make_shared<GhostTensor>();
    out->rows = a->rows;
    out->cols = a->cols;
    out->data.reserve(a->data.size());
    for (const auto& item : a->data) {
        out->data.push_back(var_relu(item));
    }
    return out;
}

std::shared_ptr<GhostTensor> GhostTensor::sigmoid(const std::shared_ptr<GhostTensor>& a) {
    auto out = std::make_shared<GhostTensor>();
    out->rows = a->rows;
    out->cols = a->cols;
    out->data.reserve(a->data.size());
    for (const auto& item : a->data) {
        out->data.push_back(var_sigmoid(item));
    }
    return out;
}

std::shared_ptr<GhostTensor> GhostTensor::mse_loss(const std::shared_ptr<GhostTensor>& pred, const std::shared_ptr<GhostTensor>& target) {
    if (pred->rows != target->rows || pred->cols != target->cols) {
        throw std::runtime_error("Dimensoes incompativeis para calculo de MSE Loss");
    }
    size_t n = pred->data.size();
    if (n == 0) return std::make_shared<GhostTensor>(1, 1, false, 0.0);

    VarPtr diff0 = var_sub(pred->data[0], target->data[0]);
    VarPtr total = var_mul(diff0, diff0);

    for (size_t i = 1; i < n; ++i) {
        VarPtr diff = var_sub(pred->data[i], target->data[i]);
        VarPtr sq = var_mul(diff, diff);
        total = var_add(total, sq);
    }

    VarPtr scale = make_var(1.0 / (double)n, false);
    VarPtr mse = var_mul(total, scale);

    auto out = std::make_shared<GhostTensor>();
    out->rows = 1;
    out->cols = 1;
    out->data.push_back(mse);
    return out;
}

// -------------------------------------------------------------
// TABELA GLOBAL DE HANDLES PARA INTERPRETADOR E C++
// -------------------------------------------------------------

static std::mutex g_autograd_mtx;
static std::map<int, TensorPtr> g_tensors;
static int g_tensor_counter = 1;

int ghost_autograd_create(int rows, int cols, bool req_grad, double init_val) {
    std::lock_guard<std::mutex> lock(g_autograd_mtx);
    int id = g_tensor_counter++;
    g_tensors[id] = std::make_shared<GhostTensor>(rows, cols, req_grad, init_val);
    return id;
}

void ghost_autograd_set(int handle, int r, int c, double val) {
    std::lock_guard<std::mutex> lock(g_autograd_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        it->second->set_val(r, c, val);
    }
}

double ghost_autograd_get(int handle, int r, int c) {
    std::lock_guard<std::mutex> lock(g_autograd_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        return it->second->get_val(r, c);
    }
    return 0.0;
}

double ghost_autograd_grad(int handle, int r, int c) {
    std::lock_guard<std::mutex> lock(g_autograd_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        return it->second->get_grad(r, c);
    }
    return 0.0;
}

int ghost_autograd_matmul(int hA, int hB) {
    std::lock_guard<std::mutex> lock(g_autograd_mtx);
    auto itA = g_tensors.find(hA);
    auto itB = g_tensors.find(hB);
    if (itA == g_tensors.end() || itB == g_tensors.end()) return 0;
    auto res = GhostTensor::matmul(itA->second, itB->second);
    int id = g_tensor_counter++;
    g_tensors[id] = res;
    return id;
}

int ghost_autograd_add(int hA, int hB) {
    std::lock_guard<std::mutex> lock(g_autograd_mtx);
    auto itA = g_tensors.find(hA);
    auto itB = g_tensors.find(hB);
    if (itA == g_tensors.end() || itB == g_tensors.end()) return 0;
    auto res = GhostTensor::add(itA->second, itB->second);
    int id = g_tensor_counter++;
    g_tensors[id] = res;
    return id;
}

int ghost_autograd_relu(int hA) {
    std::lock_guard<std::mutex> lock(g_autograd_mtx);
    auto itA = g_tensors.find(hA);
    if (itA == g_tensors.end()) return 0;
    auto res = GhostTensor::relu(itA->second);
    int id = g_tensor_counter++;
    g_tensors[id] = res;
    return id;
}

int ghost_autograd_mse(int hPred, int hTarget) {
    std::lock_guard<std::mutex> lock(g_autograd_mtx);
    auto itP = g_tensors.find(hPred);
    auto itT = g_tensors.find(hTarget);
    if (itP == g_tensors.end() || itT == g_tensors.end()) return 0;
    auto res = GhostTensor::mse_loss(itP->second, itT->second);
    int id = g_tensor_counter++;
    g_tensors[id] = res;
    return id;
}

void ghost_autograd_backward(int hLoss) {
    std::lock_guard<std::mutex> lock(g_autograd_mtx);
    auto it = g_tensors.find(hLoss);
    if (it != g_tensors.end() && !it->second->data.empty()) {
        it->second->data[0]->backward();
    }
}

void ghost_autograd_step(int handle, double lr) {
    std::lock_guard<std::mutex> lock(g_autograd_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        it->second->step(lr);
    }
}

void ghost_autograd_zero_grad(int handle) {
    std::lock_guard<std::mutex> lock(g_autograd_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        it->second->zero_grad();
    }
}

} // namespace GhostAI
