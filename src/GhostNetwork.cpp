// GhostNetwork (rede clássica: camadas gate >> gate no programa compilado) delega o treino e a
// API ao motor GhostHybridNetwork, o mesmo que o REPL usa para toda network. Assim compilador e
// REPL fazem a mesma conta, e a rede clássica ganha loss, accuracy, predict, classify, verbose
// e os otimizadores Adam, SGD, momentum, RMSprop e GD. O motor treina as mesmas matrizes no
// lugar, então quem usa as camadas diretamente (ia_propria, rede * vetor) vê os pesos novos.
#include "GhostMatrix.hpp"
#include "GhostHybrid.hpp"

GhostHybridNetwork& GhostNetwork::motor() const {
    if (!motor_) {
        std::vector<GhostLayerRef> refs;
        for (GhostMatrix* m : layers) refs.emplace_back(m);
        motor_ = std::make_shared<GhostHybridNetwork>(name, refs);
    }
    return *motor_;
}

void GhostNetwork::optimize(const GhostMatrix& input, const GhostMatrix& target, int epochs, double lr, const std::string& opt) {
    motor().optimize(input, target, epochs, lr, opt);
}
double GhostNetwork::loss(const GhostMatrix& X, const GhostMatrix& Y) const { return motor().loss(X, Y); }
double GhostNetwork::accuracy(const GhostMatrix& X, const GhostMatrix& Y) const { return motor().accuracy(X, Y); }
double GhostNetwork::predict_vec(const std::vector<double>& x) const { return motor().forward_vec(x).at(0); }
int GhostNetwork::classify_vec(const std::vector<double>& x) const { return motor().classify_vec(x); }
void GhostNetwork::set_verbose(int every_epochs) { motor().set_verbose(every_epochs); }
std::string GhostNetwork::summary() const { return motor().summary(); }
void GhostNetwork::display() const { motor().display(); }
