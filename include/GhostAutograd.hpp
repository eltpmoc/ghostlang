#ifndef GHOST_AUTOGRAD_HPP
#define GHOST_AUTOGRAD_HPP

#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <cmath>
#include <stdexcept>
#include <iostream>

namespace GhostAI {

struct GhostVar : public std::enable_shared_from_this<GhostVar> {
    double val = 0.0;
    double grad = 0.0;
    bool requires_grad = false;
    std::string op = "leaf";
    std::vector<std::shared_ptr<GhostVar>> parents;
    std::function<void()> backward_fn;

    GhostVar(double v = 0.0, bool req = false, const std::string& operation = "leaf")
        : val(v), grad(0.0), requires_grad(req), op(operation) {}

    void backward();
    void zero_grad();
};

using VarPtr = std::shared_ptr<GhostVar>;

VarPtr make_var(double val, bool req_grad = false);
VarPtr var_add(const VarPtr& a, const VarPtr& b);
VarPtr var_sub(const VarPtr& a, const VarPtr& b);
VarPtr var_mul(const VarPtr& a, const VarPtr& b);
VarPtr var_relu(const VarPtr& a);
VarPtr var_sigmoid(const VarPtr& a);
VarPtr var_tanh(const VarPtr& a);

class GhostTensor {
public:
    size_t rows = 0;
    size_t cols = 0;
    std::vector<VarPtr> data;

    GhostTensor() = default;
    GhostTensor(size_t r, size_t c, bool req_grad = false, double init_val = 0.0);
    GhostTensor(size_t r, size_t c, const std::vector<double>& vals, bool req_grad = false);

    VarPtr at(size_t r, size_t c) const {
        if (r >= rows || c >= cols) throw std::out_of_range("Indice de tensor fora dos limites");
        return data[r * cols + c];
    }

    double get_val(size_t r, size_t c) const { return at(r, c)->val; }
    double get_grad(size_t r, size_t c) const { return at(r, c)->grad; }
    void set_val(size_t r, size_t c, double v) { at(r, c)->val = v; }

    void zero_grad();
    void step(double lr);

    static std::shared_ptr<GhostTensor> add(const std::shared_ptr<GhostTensor>& a, const std::shared_ptr<GhostTensor>& b);
    static std::shared_ptr<GhostTensor> sub(const std::shared_ptr<GhostTensor>& a, const std::shared_ptr<GhostTensor>& b);
    static std::shared_ptr<GhostTensor> matmul(const std::shared_ptr<GhostTensor>& a, const std::shared_ptr<GhostTensor>& b);
    static std::shared_ptr<GhostTensor> relu(const std::shared_ptr<GhostTensor>& a);
    static std::shared_ptr<GhostTensor> sigmoid(const std::shared_ptr<GhostTensor>& a);
    static std::shared_ptr<GhostTensor> mse_loss(const std::shared_ptr<GhostTensor>& pred, const std::shared_ptr<GhostTensor>& target);
};

using TensorPtr = std::shared_ptr<GhostTensor>;

// Gerenciador de Handles de Tensores para Bindings
int ghost_autograd_create(int rows, int cols, bool req_grad, double init_val);
void ghost_autograd_set(int handle, int r, int c, double val);
double ghost_autograd_get(int handle, int r, int c);
double ghost_autograd_grad(int handle, int r, int c);
int ghost_autograd_matmul(int hA, int hB);
int ghost_autograd_add(int hA, int hB);
int ghost_autograd_relu(int hA);
int ghost_autograd_mse(int hPred, int hTarget);
void ghost_autograd_backward(int hLoss);
void ghost_autograd_step(int handle, double lr);
void ghost_autograd_zero_grad(int handle);

} // namespace GhostAI

#endif // GHOST_AUTOGRAD_HPP
