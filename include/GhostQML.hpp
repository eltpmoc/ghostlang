#pragma once
// GhostQML.hpp - Circuitos quânticos parametrizados e treináveis da GhostLang (F4).
//
// Um qcircuit guarda uma lista de portas: fixas, treináveis (θ) e de codificação de dados (x).
// Gradientes são exatos pela regra parameter-shift: dE/dθ = [E(θ+π/2) - E(θ-π/2)] / 2.
// O otimizador é Adam.
//
//   qcircuit c(2);
//   c.encode_all();          // RY(x_q) em cada qubit
//   c.ansatz(2);             // 2 camadas RY/RZ treináveis + CNOTs
//   c.add_sample(1, 0.3, 2.1);
//   c.fit(60, 0.1);          // classificador híbrido: saída = w * <O>(x) + b
//   int classe = c.predict(0.4, 1.9);
//
//   c.observable("ZZ", 0.5); // Hamiltoniano = soma de Paulis ponderados
//   double e = c.minimize(200, 0.05); // VQE
#include "GhostQuantum.hpp"

#include <random>
#include <tuple>
#include <string>
#include <utility>
#include <vector>

class GhostQCircuit {
public:
    explicit GhostQCircuit(int qubits = 1);

    int size() const { return n_; }
    int num_params() const { return static_cast<int>(theta_.size()); }
    int num_features() const { return features_; }
    int num_gates() const { return static_cast<int>(ops_.size()); }
    void seed(unsigned long long value);  // chame antes de criar parâmetros para inicialização reprodutível
    void init_params();                   // re-sorteia todos os θ em [-π, π]

    // ---- Portas fixas ----
    void h(int q);
    void x(int q);
    void y(int q);
    void z(int q);
    void s(int q);
    void t(int q);
    void cx(int control, int target);
    void cz(int control, int target);
    void swap(int a, int b);
    void rx(int q, double angle);
    void ry(int q, double angle);
    void rz(int q, double angle);

    // ---- Portas treináveis (retornam o índice do parâmetro criado) ----
    int rx_train(int q);
    int ry_train(int q);
    int rz_train(int q);
    void rx_param(int q, int index);  // reutiliza um parâmetro existente
    void ry_param(int q, int index);
    void rz_param(int q, int index);
    // Portas com parâmetro compartilhado e escala: ângulo = escala * θ[index]
    void rx_param_scaled(int q, int index, double scale);
    void rzz(int a, int b, double angle);                       // fixa: exp(-i ângulo/2 Z⊗Z)
    int rzz_train(int a, int b);
    void rzz_param(int a, int b, int index, double scale = 1.0);
    int add_param(double initial);                              // cria θ sem porta (para compartilhar)
    void ansatz(int layers);          // por camada: RY+RZ treináveis em cada qubit e CNOTs em cadeia

    // ---- Codificação de dados: ângulo = escala * x[feature] ----
    void encode_rx(int q, int feature, double scale = 1.0);
    void encode_ry(int q, int feature, double scale = 1.0);
    void encode_rz(int q, int feature, double scale = 1.0);
    void encode_all(double scale = 1.0); // RY(escala * x_q) no qubit q
    // RZZ(escala * x[fi] * x[fj]): termo de produto entre features (feature maps não lineares)
    void encode_rzz(int a, int b, int fi, int fj, double scale = 1.0);
    // Feature map "ZZ": por repetição, H em todos, RZ(2·escala·x_q) e RZZ(2·escala·x_i·x_j) em cada par
    void zz_feature_map(int reps = 1, double scale = 1.0);

    // ---- Kernel quântico: k(x, y) = |<φ(x)|φ(y)>|², com φ = este circuito aplicado a |0...0> ----
    double kernel_vec(const std::vector<double>& x, const std::vector<double>& y) const;
    template <typename... T> double kernel(T... xs) const { return kernel_split({static_cast<double>(xs)...}); }
    double kernel_fit(double lambda = 1e-3);   // kernel ridge: (K + λI)α = y; retorna a acurácia de treino
    template <typename... T> int kernel_predict(T... xs) const { return kernel_predict_vec({static_cast<double>(xs)...}); }
    template <typename... T> double kernel_score(T... xs) const { return kernel_score_vec({static_cast<double>(xs)...}); }
    double kernel_score_vec(const std::vector<double>& x) const;
    int kernel_predict_vec(const std::vector<double>& x) const;
    double kernel_accuracy() const;            // nas amostras de treino
    double kernel_test_accuracy() const;       // nas amostras de teste (add_test)
    double kernel_alignment() const;           // kernel-target alignment A(K, Y)
    std::string kernel_matrix(int max_rows = 8) const; // K das primeiras amostras, em texto

    // ---- Conjunto de teste (para medir generalização) ----
    template <typename... T> void add_test(double label, T... xs) { add_test_vec({static_cast<double>(xs)...}, label); }
    void add_test_vec(const std::vector<double>& x, double label);
    int test_samples() const { return static_cast<int>(test_x_.size()); }
    double test_accuracy() const;              // classificador variacional (fit) no conjunto de teste

    // ---- QAOA para MaxCut ----
    void edge(int a, int b, double weight = 1.0);
    void clear_edges() { edges_.clear(); }
    int num_edges() const { return static_cast<int>(edges_.size()); }
    void qaoa(int layers);                     // monta o circuito QAOA do zero e o observável de custo
    double expected_cut() const;               // E[corte] com os parâmetros atuais
    double cut_of(const std::string& bits) const; // valor do corte de uma solução, ex.: "0101" (q(n-1)...q0)
    double maxcut_exact() const;               // força bruta (até 24 vértices), para comparação
    std::string maxcut_solution(int shots);    // amostra o circuito e devolve o melhor corte encontrado

    // ---- Observável (Hamiltoniano) ----
    void observable(const std::string& pauli, double coef = 1.0);
    void clear_observable();
    std::string observable_string() const;

    // ---- Avaliação ----
    double expect_vec(const std::vector<double>& x) const;
    std::vector<double> gradient_vec(const std::vector<double>& x) const; // parameter-shift
    template <typename... T> double expect(T... xs) const { return expect_vec({static_cast<double>(xs)...}); }
    GhostQReg state_vec(const std::vector<double>& x) const;
    template <typename... T> std::string state(T... xs) const { return state_vec({static_cast<double>(xs)...}).ket(); }

    // ---- VQE ----
    double energy() const;                   // <H> com os parâmetros atuais (sem dados)
    double minimize(int steps, double lr);   // Adam + parameter-shift; retorna a energia final

    // ---- Classificador híbrido: saída = w * <O>(x) + b ----
    template <typename... T> void add_sample(double label, T... xs) { add_sample_vec({static_cast<double>(xs)...}, label); }
    void add_sample_vec(const std::vector<double>& x, double label);
    int load_csv(const std::string& path);   // colunas: x0, x1, ..., rótulo
    void clear_data();
    int samples() const { return static_cast<int>(data_x_.size()); }
    double fit(int epochs, double lr);       // retorna a perda (MSE) final
    template <typename... T> double score(T... xs) const { return score_vec({static_cast<double>(xs)...}); }
    template <typename... T> int predict(T... xs) const { return predict_vec({static_cast<double>(xs)...}); }
    double score_vec(const std::vector<double>& x) const;
    int predict_vec(const std::vector<double>& x) const;
    double accuracy() const;                 // fração de acertos nas amostras carregadas
    double loss() const;
    void set_verbose(int every_epochs) { verbose_ = every_epochs; }

    // ---- Parâmetros ----
    double param(int i) const;
    void set_param(int i, double value);
    double weight() const { return w_; }
    double bias() const { return b_; }
    std::string params() const;
    void save(const std::string& path) const;
    void load(const std::string& path);

    // ---- Visualização ----
    std::string draw() const;
    std::string to_string() const;
    void display() const;

    // ---- OpenQASM 3.0 Interoperabilidade ----
    std::string to_qasm() const;
    static GhostQCircuit from_qasm(const std::string& qasm);

    // ---- Inverso de sub-circuitos (circuit nome_dg, ver GhostQTape.hpp) ----
    // Cópia leve (portas, parâmetros e gerador aleatório, sem dados nem observável) para gravar um circuit.
    GhostQCircuit inverse_recorder() const;
    // Acrescenta o inverso das portas que `rec` ganhou a partir de op_start, em ordem reversa:
    // ângulos fixos e escalas (parâmetros e features) negados; S† = S³, T† = T⁷.
    // Parâmetros criados durante a gravação (índice >= param_start) viram parâmetros novos
    // deste circuito com o mesmo valor; os já existentes são compartilhados.
    void append_inverse(const GhostQCircuit& rec, int op_start, int param_start);

private:
    friend class GhostQLayer;
    enum class Kind { H, X, Y, Z, S, T, CX, CZ, SWAP, RX, RY, RZ, RZZ };
    enum class Source { Fixed, Param, Feature, Feature2 };
    struct Op {
        Kind kind;
        int a, b;        // qubits (b = -1 em portas de 1 qubit)
        Source source;
        int index;       // índice do parâmetro ou da feature
        double value;    // ângulo fixo, ou escala (Param: ângulo = escala*θ; Feature: escala*x)
        int index2 = -1; // Feature2: segunda feature (ângulo = escala * x[index] * x[index2])
    };
    std::vector<std::tuple<int, int, double>> edges_;
    std::vector<std::vector<double>> test_x_;
    std::vector<double> test_y_;
    std::vector<double> alpha_;                       // kernel ridge
    std::vector<std::vector<GhostQReg::amp>> kernel_states_;
    double kernel_split(const std::vector<double>& xy) const;
    double op_angle(const Op& op, const std::vector<double>& x) const;
    void add_op2(Kind k, int a, int b, Source src, int index, double value, int index2 = -1);

    int n_;
    std::vector<Op> ops_;
    std::vector<double> theta_;
    std::vector<std::pair<double, std::string>> obs_;
    int features_ = 0;
    double w_ = 1.0, b_ = 0.0;
    std::vector<std::vector<double>> data_x_;
    std::vector<double> data_y_;     // rótulos em {-1, +1}
    bool zero_one_labels_ = false;   // rótulos originais eram 0/1
    mutable std::mt19937_64 rng_;
    int verbose_ = 0;

    void check_qubit(int q) const;
    int new_param();
    void add_rot(Kind k, int q, Source src, int index, double value);
    void run_into(GhostQReg& reg, const std::vector<double>& x, int shift_op, double shift, bool labels) const;
    double measure_obs(const GhostQReg& reg) const;
    double eval(const std::vector<double>& x, int shift_op, double shift) const;
    std::vector<double> param_gradient(const std::vector<double>& x) const;
    std::vector<double> zeros_input() const { return std::vector<double>(features_, 0.0); }
};

std::ostream& operator<<(std::ostream& os, const GhostQCircuit& c);

// ---------------------------------------------------------------------------
// GhostQLayer - camada quântica para redes híbridas (network W1 >> Q >> W2).
//   entrada x (n valores) -> RY(x_q) em cada qubit -> ansatz treinável -> saída <Z_q> (n valores em [-1, 1])
// Gradientes em relação a θ e às entradas x são exatos (parameter-shift), o que permite
// backpropagation através da camada.
// ---------------------------------------------------------------------------
class GhostQLayer {
public:
    explicit GhostQLayer(int qubits = 1, int layers = 1);

    int inputs() const { return c_.size(); }
    int outputs() const { return c_.size(); }
    int num_params() const { return c_.num_params(); }
    void seed(unsigned long long value);   // re-sorteia os θ de forma reprodutível
    double* param_ptr(int i) { return &c_.theta_.at(static_cast<std::size_t>(i)); }
    GhostQCircuit& circuit() { return c_; }

    std::vector<double> forward(const std::vector<double>& x) const;
    // gout = dL/d(saída). Soma dL/dx em gin (tamanho inputs) e dL/dθ em gtheta (tamanho num_params).
    void backward(const std::vector<double>& x, const std::vector<double>& gout,
                  std::vector<double>& gin, std::vector<double>& gtheta) const;

    std::string draw() const { return c_.draw(); }
    std::string to_string() const;
    void display() const;

private:
    GhostQCircuit c_;
    std::vector<double> z_all(const std::vector<double>& x, int shift_op, double shift) const;
};

std::ostream& operator<<(std::ostream& os, const GhostQLayer& layer);
