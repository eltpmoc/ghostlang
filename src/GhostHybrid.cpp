// GhostHybrid.cpp - Backpropagation através de camadas densas e quânticas (F4).
#include "GhostHybrid.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
std::string fixed(double v, int d) { std::ostringstream os; os << std::fixed << std::setprecision(d) << v; return os.str(); }
}

GhostHybridNetwork::GhostHybridNetwork(const std::string& name, std::vector<GhostLayerRef> layers)
    : name_(name), layers_(std::move(layers)) {
    if (layers_.empty()) throw std::invalid_argument("network " + name_ + ": sem camadas");
    for (std::size_t i = 0; i + 1 < layers_.size(); ++i) {
        if (layers_[i].out() != layers_[i + 1].in()) {
            throw std::invalid_argument("network " + name_ + ": a camada " + std::to_string(i + 2) + " espera " +
                                        std::to_string(layers_[i + 1].in()) + " entradas, mas a camada " + std::to_string(i + 1) +
                                        " produz " + std::to_string(layers_[i].out()));
        }
    }
}

int GhostHybridNetwork::inputs() const { return layers_.empty() ? 0 : layers_.front().in(); }
int GhostHybridNetwork::outputs() const { return layers_.empty() ? 0 : layers_.back().out(); }

int GhostHybridNetwork::num_params() const {
    int n = 0;
    for (const auto& l : layers_) n += l.dense ? l.out() * l.in() + l.out() : l.quantum->num_params();
    return n;
}

bool GhostHybridNetwork::relu_after(std::size_t i) const {
    return layers_[i].dense && i + 1 < layers_.size() && layers_[i + 1].dense;
}

std::vector<double> GhostHybridNetwork::column(const GhostMatrix& M, std::size_t c) {
    std::vector<double> v(M.getRows());
    for (std::size_t r = 0; r < M.getRows(); ++r) v[r] = M(r, c).x;
    return v;
}

std::vector<double> GhostHybridNetwork::forward_vec(const std::vector<double>& x) const {
    if (static_cast<int>(x.size()) != inputs()) {
        throw std::invalid_argument("network " + name_ + ": espera " + std::to_string(inputs()) + " entradas, recebeu " + std::to_string(x.size()));
    }
    std::vector<double> a = x;
    for (std::size_t i = 0; i < layers_.size(); ++i) {
        const auto& L = layers_[i];
        if (L.quantum) { a = L.quantum->forward(a); continue; }
        const GhostMatrix& W = *L.dense;
        std::vector<double> z(W.getRows());
        for (std::size_t r = 0; r < W.getRows(); ++r) {
            double s = W.bias[r].x;
            for (std::size_t c = 0; c < W.getCols(); ++c) s += W(r, c).x * a[c];
            z[r] = relu_after(i) ? std::max(0.0, s) : s;
        }
        a = std::move(z);
    }
    return a;
}

GhostMatrix GhostHybridNetwork::forward(const GhostMatrix& X) const {
    GhostMatrix out(outputs(), X.getCols());
    for (std::size_t c = 0; c < X.getCols(); ++c) {
        const auto y = forward_vec(column(X, c));
        for (std::size_t r = 0; r < y.size(); ++r) out(r, c) = GhostNumberG(y[r], 0.0, 0.0);
    }
    return out;
}

int GhostHybridNetwork::classify_vec(const std::vector<double>& x) const {
    const auto y = forward_vec(x);
    if (y.size() == 1) return y[0] >= (zero_one_ ? 0.5 : 0.0) ? 1 : (zero_one_ ? 0 : -1);
    return static_cast<int>(std::max_element(y.begin(), y.end()) - y.begin());
}

void GhostHybridNetwork::check_data(const GhostMatrix& X, const GhostMatrix& Y) const {
    if (static_cast<int>(X.getRows()) != inputs()) throw std::invalid_argument("network " + name_ + ": X precisa ter " + std::to_string(inputs()) + " linhas (uma por entrada)");
    if (static_cast<int>(Y.getRows()) != outputs()) throw std::invalid_argument("network " + name_ + ": Y precisa ter " + std::to_string(outputs()) + " linhas (uma por saida)");
    if (X.getCols() != Y.getCols() || X.getCols() == 0) throw std::invalid_argument("network " + name_ + ": X e Y precisam ter o mesmo numero de colunas (amostras)");
}

// Forward guardando ativações, depois backward. Soma scale * dL/dθ em grad; retorna o erro quadrático da amostra.
double GhostHybridNetwork::sample_backward(const std::vector<double>& x, const std::vector<double>& y,
                                           std::vector<double>& grad, double scale) const {
    const std::size_t L = layers_.size();
    std::vector<std::vector<double>> in(L), pre(L);
    std::vector<double> a = x;
    for (std::size_t i = 0; i < L; ++i) {
        in[i] = a;
        const auto& layer = layers_[i];
        if (layer.quantum) { a = layer.quantum->forward(a); continue; }
        const GhostMatrix& W = *layer.dense;
        std::vector<double> z(W.getRows());
        for (std::size_t r = 0; r < W.getRows(); ++r) {
            double s = W.bias[r].x;
            for (std::size_t c = 0; c < W.getCols(); ++c) s += W(r, c).x * a[c];
            z[r] = s;
        }
        pre[i] = z;
        if (relu_after(i)) for (auto& v : z) v = std::max(0.0, v);
        a = std::move(z);
    }
    double sq = 0.0;
    std::vector<double> delta(a.size());
    for (std::size_t o = 0; o < a.size(); ++o) { const double e = a[o] - y[o]; sq += e * e; delta[o] = 2.0 * e * scale; }

    // offsets de cada camada no vetor achatado de parâmetros
    std::vector<std::size_t> off(L);
    std::size_t acc = 0;
    for (std::size_t i = 0; i < L; ++i) { off[i] = acc; acc += layers_[i].dense ? layers_[i].out() * layers_[i].in() + layers_[i].out() : layers_[i].quantum->num_params(); }

    for (std::size_t ii = L; ii-- > 0;) {
        const auto& layer = layers_[ii];
        std::vector<double> prev(layer.in(), 0.0);
        if (layer.quantum) {
            std::vector<double> gq(layer.quantum->num_params(), 0.0);
            layer.quantum->backward(in[ii], delta, prev, gq);
            for (std::size_t p = 0; p < gq.size(); ++p) grad[off[ii] + p] += gq[p];
        } else {
            const GhostMatrix& W = *layer.dense;
            const std::size_t R = W.getRows(), C = W.getCols();
            if (relu_after(ii)) for (std::size_t r = 0; r < R; ++r) if (pre[ii][r] <= 0.0) delta[r] = 0.0;
            for (std::size_t r = 0; r < R; ++r) {
                for (std::size_t c = 0; c < C; ++c) {
                    grad[off[ii] + r * C + c] += delta[r] * in[ii][c];
                    prev[c] += W(r, c).x * delta[r];
                }
                grad[off[ii] + R * C + r] += delta[r];
            }
        }
        delta = std::move(prev);
    }
    return sq;
}

double GhostHybridNetwork::loss_and_grad(const GhostMatrix& X, const GhostMatrix& Y, std::vector<double>* grad) const {
    check_data(X, Y);
    const long long N = static_cast<long long>(X.getCols());
    const std::size_t P = static_cast<std::size_t>(num_params());
    std::vector<double> total(P, 0.0);
    double loss = 0.0;
    #pragma omp parallel
    {
        std::vector<double> local(P, 0.0);
        double local_loss = 0.0;
        #pragma omp for
        for (long long s = 0; s < N; ++s) local_loss += sample_backward(column(X, s), column(Y, s), local, 1.0 / N) / N;
        #pragma omp critical
        {
            for (std::size_t p = 0; p < P; ++p) total[p] += local[p];
            loss += local_loss;
        }
    }
    if (grad) *grad = std::move(total);
    return loss;
}

std::vector<double*> GhostHybridNetwork::parameters() {
    std::vector<double*> ptrs;
    for (auto& l : layers_) {
        if (l.quantum) { for (int p = 0; p < l.quantum->num_params(); ++p) ptrs.push_back(l.quantum->param_ptr(p)); continue; }
        GhostMatrix& W = *l.dense;
        for (std::size_t r = 0; r < W.getRows(); ++r) for (std::size_t c = 0; c < W.getCols(); ++c) ptrs.push_back(&W(r, c).x);
        for (std::size_t r = 0; r < W.getRows(); ++r) ptrs.push_back(&W.bias[r].x);
    }
    return ptrs;
}

double GhostHybridNetwork::loss(const GhostMatrix& X, const GhostMatrix& Y) const { return loss_and_grad(X, Y, nullptr); }

double GhostHybridNetwork::accuracy(const GhostMatrix& X, const GhostMatrix& Y) const {
    check_data(X, Y);
    bool zero_one = true;
    for (std::size_t c = 0; c < Y.getCols(); ++c) for (std::size_t r = 0; r < Y.getRows(); ++r) if (Y(r, c).x < 0) zero_one = false;
    int hits = 0;
    for (std::size_t c = 0; c < X.getCols(); ++c) {
        const auto y = forward_vec(column(X, c));
        const auto t = column(Y, c);
        bool ok;
        if (y.size() == 1) { const double th = zero_one ? 0.5 : 0.0; ok = (y[0] >= th) == (t[0] >= th); }
        else ok = (std::max_element(y.begin(), y.end()) - y.begin()) == (std::max_element(t.begin(), t.end()) - t.begin());
        hits += ok ? 1 : 0;
    }
    return double(hits) / X.getCols();
}

void GhostHybridNetwork::optimize(const GhostMatrix& X, const GhostMatrix& Y, int epochs, double lr, const std::string& opt) {
    check_data(X, Y);
    zero_one_ = true;
    for (std::size_t c = 0; c < Y.getCols(); ++c) for (std::size_t r = 0; r < Y.getRows(); ++r) if (Y(r, c).x < 0) zero_one_ = false;
    std::vector<double*> ptrs = parameters();
    if (adam_m_.size() != ptrs.size()) { adam_m_.assign(ptrs.size(), 0.0); adam_v_.assign(ptrs.size(), 0.0); adam_t_ = 0; }
    const bool adam = opt != "gd" && opt != "sgd";
    for (int epoch = 1; epoch <= epochs; ++epoch) {
        std::vector<double> g;
        const double l = loss_and_grad(X, Y, &g);
        if (verbose_ > 0 && (epoch == 1 || epoch % verbose_ == 0 || epoch == epochs)) {
            std::cout << "  epoca " << epoch << "/" << epochs << "  perda=" << fixed(l, 4) << "  acuracia=" << fixed(100.0 * accuracy(X, Y), 1) << "%" << std::endl;
        }
        if (adam) {
            ++adam_t_;
            const double b1 = 0.9, b2 = 0.999, eps = 1e-8;
            const double c1 = 1 - std::pow(b1, adam_t_), c2 = 1 - std::pow(b2, adam_t_);
            for (std::size_t p = 0; p < ptrs.size(); ++p) {
                adam_m_[p] = b1 * adam_m_[p] + (1 - b1) * g[p];
                adam_v_[p] = b2 * adam_v_[p] + (1 - b2) * g[p] * g[p];
                *ptrs[p] -= lr * (adam_m_[p] / c1) / (std::sqrt(adam_v_[p] / c2) + eps);
            }
        } else {
            for (std::size_t p = 0; p < ptrs.size(); ++p) *ptrs[p] -= lr * g[p];
        }
    }
}

std::string GhostHybridNetwork::summary() const {
    std::ostringstream os;
    os << "network " << name_ << " (hibrida): " << inputs();
    for (const auto& l : layers_) {
        if (l.dense) os << " -> [densa " << l.out() << "x" << l.in() << "]";
        else os << " -> [qlayer " << l.quantum->inputs() << " qubits, " << l.quantum->num_params() << " parametros]";
    }
    os << " -> " << outputs() << " saida(s); " << num_params() << " parametros treinaveis";
    return os.str();
}

void GhostHybridNetwork::display() const { std::cout << summary() << std::endl; }

GhostMatrix operator*(const GhostHybridNetwork& net, const GhostMatrix& input) { return net.forward(input); }
std::ostream& operator<<(std::ostream& os, const GhostHybridNetwork& net) { return os << net.summary(); }
