#pragma once
// GhostHybrid.hpp - Redes híbridas clássico-quânticas da GhostLang (F4).
//
//   gate W1(2, 2);          // densa: 2 entradas -> 2 saídas
//   qlayer Q(2, 2);         // quântica: 2 qubits, 2 camadas de ansatz
//   gate W2(1, 2);          // densa: 2 -> 1
//   network rede = W1 >> Q >> W2;
//   rede => optimize(X, Y, epochs=100, lr=0.05, opt="adam");   // colunas de X/Y = amostras
//
// Ativações: ReLU entre duas camadas densas; nenhuma antes de uma qlayer (a saída densa vira
// ângulo RY); a saída <Z> da qlayer entra direto na camada seguinte; a última camada é linear.
// Backpropagation: densas pela regra da cadeia; qlayer pelo parameter-shift (θ e entradas).
// Só a parte real (x) dos pesos GhostNumberG é treinada, como na GhostNetwork clássica.
#include "GhostMatrix.hpp"
#include "GhostQML.hpp"

#include <ostream>
#include <string>
#include <vector>

struct GhostLayerRef {
    GhostMatrix* dense = nullptr;
    GhostQLayer* quantum = nullptr;
    GhostLayerRef(GhostMatrix* m) : dense(m) {}
    GhostLayerRef(GhostQLayer* q) : quantum(q) {}
    int in() const { return dense ? static_cast<int>(dense->getCols()) : quantum->inputs(); }
    int out() const { return dense ? static_cast<int>(dense->getRows()) : quantum->outputs(); }
};

class GhostHybridNetwork {
public:
    GhostHybridNetwork() = default;
    GhostHybridNetwork(const std::string& name, std::vector<GhostLayerRef> layers);

    int inputs() const;
    int outputs() const;
    int num_params() const;

    std::vector<double> forward_vec(const std::vector<double>& x) const;
    GhostMatrix forward(const GhostMatrix& X) const;  // X: entradas x amostras
    template <typename... T> double predict(T... xs) const { return forward_vec({static_cast<double>(xs)...}).at(0); }
    template <typename... T> int classify(T... xs) const { return classify_vec({static_cast<double>(xs)...}); }
    int classify_vec(const std::vector<double>& x) const;

    void optimize(const GhostMatrix& X, const GhostMatrix& Y, int epochs, double lr, const std::string& opt = "adam");
    double loss(const GhostMatrix& X, const GhostMatrix& Y) const;      // MSE médio por amostra
    double accuracy(const GhostMatrix& X, const GhostMatrix& Y) const;  // 1 saída: limiar 0,5 (0/1) ou 0 (±1); várias: argmax
    // Perda e gradiente achatado (mesma ordem de parameters()); usado pelos testes.
    double loss_and_grad(const GhostMatrix& X, const GhostMatrix& Y, std::vector<double>* grad) const;
    std::vector<double*> parameters();

    void set_verbose(int every_epochs) { verbose_ = every_epochs; }
    std::string summary() const;
    void display() const;

private:
    std::string name_;
    std::vector<GhostLayerRef> layers_;
    int verbose_ = 0;
    std::vector<double> adam_m_, adam_v_;
    int adam_t_ = 0;
    bool zero_one_ = true; // alvos 0/1 (limiar 0,5) - definido no optimize

    bool relu_after(std::size_t i) const;
    void check_data(const GhostMatrix& X, const GhostMatrix& Y) const;
    double sample_backward(const std::vector<double>& x, const std::vector<double>& y,
                           std::vector<double>& grad, double scale) const;
    static std::vector<double> column(const GhostMatrix& M, std::size_t c);
};

GhostMatrix operator*(const GhostHybridNetwork& net, const GhostMatrix& input);
std::ostream& operator<<(std::ostream& os, const GhostHybridNetwork& net);
