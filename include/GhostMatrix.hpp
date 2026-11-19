#ifdef GHOST_NATIVE_RUNTIME
#include "GhostDevice.hpp"
#endif
#include <iostream>
#ifndef GHOST_MATRIX_HPP
#define GHOST_MATRIX_HPP

#include "GhostNumberG.hpp"
#include <vector>
#include <iostream>
#include <stdexcept>
#include <iomanip>
#include <fstream>  // Adicionado para File I/O (legado, mantido se necessario)
#include <sstream>  // Adicionado para parsing de strings
#include <string>
#include <memory>
#include <cmath>    // Adicionado para sqrt (Quantum Gates)
#include <cstdio>   // Adicionado nativo C-I/O para bypass de ABI

class GhostMatrix {
    friend class GhostNetwork;
private:
    size_t rows;
    size_t cols;
    std::vector<std::vector<GhostNumberG>> data;

public:
    // Otimizador Adam: estado por matriz
    int adam_t = 0;
    std::vector<std::vector<double>> adam_m;
    std::vector<std::vector<double>> adam_v;
    
    // Bias e estado Adam do Bias
    std::vector<GhostNumberG> bias;
    std::vector<double> adam_bias_m;
    std::vector<double> adam_bias_v;

    size_t getRows() const { return rows; }
    size_t getCols() const { return cols; }

    // -------------------------------------------------------------
    // FÁBRICAS QUÂNTICAS NATIVAS (Quantum Gates)
    // -------------------------------------------------------------
    static GhostMatrix Hadamard() {
        GhostMatrix h(2, 2);
        double inv_sqrt2 = 1.0 / std::sqrt(2.0);
        h(0, 0) = GhostNumberG(inv_sqrt2, 0.0, 0.0);
        h(0, 1) = GhostNumberG(inv_sqrt2, 0.0, 0.0);
        h(1, 0) = GhostNumberG(inv_sqrt2, 0.0, 0.0);
        h(1, 1) = GhostNumberG(-inv_sqrt2, 0.0, 0.0);
        return h;
    }

    static GhostMatrix PauliX() {
        GhostMatrix x(2, 2);
        x(0, 0) = GhostNumberG(0.0, 0.0, 0.0);
        x(0, 1) = GhostNumberG(1.0, 0.0, 0.0);
        x(1, 0) = GhostNumberG(1.0, 0.0, 0.0);
        x(1, 1) = GhostNumberG(0.0, 0.0, 0.0);
        return x;
    }

    static GhostMatrix PauliY() {
        GhostMatrix y(2, 2);
        y(0, 0) = GhostNumberG(0.0, 0.0, 0.0);
        y(0, 1) = GhostNumberG(0.0, 0.0, -1.0); // -i
        y(1, 0) = GhostNumberG(0.0, 0.0, 1.0);  // i
        y(1, 1) = GhostNumberG(0.0, 0.0, 0.0);
        return y;
    }

    static GhostMatrix PauliZ() {
        GhostMatrix z(2, 2);
        z(0, 0) = GhostNumberG(1.0, 0.0, 0.0);
        z(0, 1) = GhostNumberG(0.0, 0.0, 0.0);
        z(1, 0) = GhostNumberG(0.0, 0.0, 0.0);
        z(1, 1) = GhostNumberG(-1.0, 0.0, 0.0);
        return z;
    }

    // Construtor padrao necessario para uso no std::map
    GhostMatrix() : rows(0), cols(0), adam_t(0) {}

    GhostMatrix(size_t r, size_t c) : rows(r), cols(c), adam_t(0) {
        data.resize(rows, std::vector<GhostNumberG>(cols, GhostNumberG(0.0, 0.0, 0.0)));
        adam_m.resize(rows, std::vector<double>(cols, 0.0));
        adam_v.resize(rows, std::vector<double>(cols, 0.0));
        bias.resize(rows, GhostNumberG(0.0, 0.0, 0.0));
        adam_bias_m.resize(rows, 0.0);
        adam_bias_v.resize(rows, 0.0);
    }

    // NOVO: Método para carregar dados do mundo real para dentro do estado quântico
    void loadCSV(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Erro Critico: Nao foi possivel abrir o arquivo " + filename);
        }

        std::string line, val;
        size_t r = 0;
        
        while (std::getline(file, line) && r < rows) {
            std::stringstream ss(line);
            size_t c = 0;
            
            // Lê cada valor separado por vírgula
            while (std::getline(ss, val, ',') && c < cols) {
                // Carrega o dado real. A parte "fantasma" inicia zerada.
                data[r][c] = GhostNumberG(std::stod(val), 0.0, 0.0); 
                c++;
            }
            r++;
        }
        file.close();
    }

    void save(const std::string& filename) const {
        try {
            std::cout << "[Ghost OS Debug] Entrando no save para " << filename << " via C-I/O..." << std::endl;
            
            FILE* out = fopen(filename.c_str(), "wb");
            if (!out) {
                std::cerr << "[Ghost OS] Erro: Nao foi possivel abrir ou criar o arquivo " << filename << std::endl;
                return; // Ignora o erro silenciosamente
            }
            
            size_t r = rows;
            size_t c = cols;
            fwrite(&r, sizeof(size_t), 1, out);
            fwrite(&c, sizeof(size_t), 1, out);
            
            for (size_t i = 0; i < rows; ++i) {
                if (data[i].size() != cols) {
                    std::cerr << "[Ghost OS Debug] ERRO: Dimensao da linha corrompida!" << std::endl;
                    fclose(out);
                    return;
                }
                fwrite(data[i].data(), sizeof(GhostNumberG), cols, out);
            }
            
            // Salvar Bias
            size_t br = rows;
            fwrite(&br, sizeof(size_t), 1, out);
            fwrite(bias.data(), sizeof(GhostNumberG), rows, out);
            
            fclose(out);
            std::cout << "[Ghost OS Debug] Arquivo salvo com sucesso!" << std::endl;
        } catch(...) {
            // Ignora erro
        }
    }

    void load(const std::string& filename) {
        try {
            FILE* in = fopen(filename.c_str(), "rb");
            if (!in) {
                std::cerr << "[Ghost OS] Aviso: Checkpoint '" << filename << "' nao encontrado. Iniciando rede do zero." << std::endl;
                return;
            }
            
            size_t r = 0, c = 0;
            if (fread(&r, sizeof(size_t), 1, in) != 1) { r = 0; }
            if (fread(&c, sizeof(size_t), 1, in) != 1) { c = 0; }
            
            this->rows = r;
            this->cols = c;
            this->data.clear();
            this->adam_m.clear();
            this->adam_v.clear();
            
            this->data.resize(r, std::vector<GhostNumberG>(c, GhostNumberG(0.0, 0.0, 0.0)));
            this->adam_m.resize(r, std::vector<double>(c, 0.0));
            this->adam_v.resize(r, std::vector<double>(c, 0.0));
            
            for (size_t i = 0; i < rows; ++i) {
                if (fread(data[i].data(), sizeof(GhostNumberG), cols, in) != cols) {
                    break;
                }
            }
            
            // Carregar Bias se existir
            this->bias.resize(rows, GhostNumberG(0.0, 0.0, 0.0));
            this->adam_bias_m.resize(rows, 0.0);
            this->adam_bias_v.resize(rows, 0.0);
            size_t br = 0;
            if (fread(&br, sizeof(size_t), 1, in) == 1 && br == rows) {
                if (fread(bias.data(), sizeof(GhostNumberG), rows, in) != rows) {
                    // Ignora silently
                }
            }
            
            fclose(in);
        } catch(...) {
            // Ignora erro para não crashar no boot
        }
    }

    // Acesso aos elementos
    // Iteração elemento a elemento, linha por linha: for (var x in m) / foreach (x in m).
    // x é o Número G da posição (referência: alterar x altera a matriz).
    template <typename Mat, typename Elem>
    struct ElemIter {
        Mat* m;
        size_t r, c;
        Elem& operator*() const { return (*m)(r, c); }
        ElemIter& operator++() {
            if (++c >= m->getCols()) { c = 0; ++r; }
            return *this;
        }
        bool operator!=(const ElemIter& o) const { return r != o.r || c != o.c; }
    };
    using iterator = ElemIter<GhostMatrix, GhostNumberG>;
    using const_iterator = ElemIter<const GhostMatrix, const GhostNumberG>;
    iterator begin() { return getCols() == 0 ? end() : iterator{this, 0, 0}; }
    iterator end() { return iterator{this, getRows(), 0}; }
    const_iterator begin() const { return getCols() == 0 ? end() : const_iterator{this, 0, 0}; }
    const_iterator end() const { return const_iterator{this, getRows(), 0}; }

    GhostNumberG& operator()(size_t r, size_t c) {
        return data[r][c];
    }

    const GhostNumberG& operator()(size_t r, size_t c) const {
        return data[r][c];
    }

    // Multiplicação de Matrizes (O núcleo da evolução do estado VQC)
    GhostMatrix operator*(const GhostMatrix& other) const {
        if (this->cols != other.rows) {
            throw std::invalid_argument("Dimensoes incompativeis para multiplicacao matricial.");
        }

        GhostMatrix result(this->rows, other.cols);
#ifdef GHOST_NATIVE_RUNTIME
        if (ghost::accelerate_matrix(rows,cols,other.cols)) {
            std::vector<ghost::PackedG> left(rows*cols),right(other.rows*other.cols),out(rows*other.cols);
            for (size_t r=0;r<rows;++r) for(size_t c=0;c<cols;++c) {
                const auto& g=data[r][c]; left[r*cols+c]={g.x,g.y,g.p};
            }
            for (size_t r=0;r<other.rows;++r) for(size_t c=0;c<other.cols;++c) {
                const auto& g=other.data[r][c]; right[r*other.cols+c]={g.x,g.y,g.p};
            }
            ghost::multiply_cuda(left.data(),right.data(),out.data(),rows,cols,other.cols);
            for (size_t r=0;r<rows;++r) for(size_t c=0;c<other.cols;++c) {
                const auto& g=out[r*other.cols+c]; result(r,c)=GhostNumberG(g.x,g.y,g.p);
            }
            return result;
        }
#endif


        // Aceleração Massiva em Hardware (Multi-threading via OpenMP)
        #pragma omp parallel for collapse(2) if(rows*cols*other.cols >= 262144)
        for (int i = 0; i < (int)this->rows; ++i) {
            for (int j = 0; j < (int)other.cols; ++j) {
                GhostNumberG sum(0.0, 0.0);
                for (int k = 0; k < (int)this->cols; ++k) {
                    sum = sum + (this->data[i][k] * other.data[k][j]);
                }
                result(i, j) = sum;
            }
        }
        return result;
    }

    // Funcoes de Ativacao Nao-Lineares
    GhostMatrix relu() const {
        GhostMatrix result(this->rows, this->cols);
        for (size_t r = 0; r < this->rows; ++r) {
            for (size_t c = 0; c < this->cols; ++c) {
                double v = this->data[r][c].x;
                result(r, c) = GhostNumberG(v > 0 ? v : 0.0, 0.0, 0.0);
            }
        }
        return result;
    }

    GhostMatrix sigmoid() const {
        GhostMatrix result(this->rows, this->cols);
        for (size_t r = 0; r < this->rows; ++r) {
            for (size_t c = 0; c < this->cols; ++c) {
                double v = this->data[r][c].x;
                double sig = 1.0 / (1.0 + std::exp(-v));
                result(r, c) = GhostNumberG(sig, 0.0, 0.0);
            }
        }
        return result;
    }

    GhostMatrix tanh() const {
        GhostMatrix result(this->rows, this->cols);
        for (size_t r = 0; r < this->rows; ++r) {
            for (size_t c = 0; c < this->cols; ++c) {
                result(r, c) = this->data[r][c].tanh();
            }
        }
        return result;
    }

    GhostMatrix leaky_relu(double alpha = 0.01) const {
        GhostMatrix result(this->rows, this->cols);
        for (size_t r = 0; r < this->rows; ++r) {
            for (size_t c = 0; c < this->cols; ++c) {
                result(r, c) = this->data[r][c].leaky_relu(alpha);
            }
        }
        return result;
    }

    GhostMatrix gelu() const {
        GhostMatrix result(this->rows, this->cols);
        for (size_t r = 0; r < this->rows; ++r) {
            for (size_t c = 0; c < this->cols; ++c) {
                result(r, c) = this->data[r][c].gelu();
            }
        }
        return result;
    }

    GhostMatrix softmax() const {
        GhostMatrix result(this->rows, this->cols);
        for (size_t r = 0; r < this->rows; ++r) {
            double max_val = -1e30;
            for (size_t c = 0; c < this->cols; ++c) {
                if (this->data[r][c].x > max_val) max_val = this->data[r][c].x;
            }
            double sum = 0.0;
            for (size_t c = 0; c < this->cols; ++c) {
                sum += std::exp(this->data[r][c].x - max_val);
            }
            for (size_t c = 0; c < this->cols; ++c) {
                double sm = std::exp(this->data[r][c].x - max_val) / (sum > 0.0 ? sum : 1.0);
                result(r, c) = GhostNumberG(sm, 0.0, 0.0);
            }
        }
        return result;
    }

    // -------------------------------------------------------------
    // VISÃO COMPUTACIONAL QUÂNTICA
    // -------------------------------------------------------------
    // Aplica um filtro de convolução sobre a matriz (feature extraction)
    GhostMatrix conv2d(const GhostMatrix& kernel, int stride = 1) const {
        if (this->rows < kernel.rows || this->cols < kernel.cols) {
            throw std::invalid_argument("Kernel maior que a matriz original na Convolucao.");
        }
        
        int outRows = (this->rows - kernel.rows) / stride + 1;
        int outCols = (this->cols - kernel.cols) / stride + 1;
        GhostMatrix result(outRows, outCols);

#ifdef GHOST_GPU_ENABLED
        #pragma omp target teams distribute parallel for collapse(2) map(to: this->data, kernel.data) map(from: result.data)
#else
        #pragma omp parallel for collapse(2)
#endif
        for (int i = 0; i < outRows; ++i) {
            for (int j = 0; j < outCols; ++j) {
                GhostNumberG sum(0.0, 0.0);
                for (int ki = 0; ki < (int)kernel.rows; ++ki) {
                    for (int kj = 0; kj < (int)kernel.cols; ++kj) {
                        int rowIdx = i * stride + ki;
                        int colIdx = j * stride + kj;
                        sum = sum + (this->data[rowIdx][colIdx] * kernel.data[ki][kj]);
                    }
                }
                result(i, j) = sum;
            }
        }
        return result;
    }

    // Max Pooling 2D (visao computacional / convolucao)
    GhostMatrix max_pool2d(int pool_size = 2, int stride = 2) const {
        if ((int)this->rows < pool_size || (int)this->cols < pool_size) {
            throw std::invalid_argument("Janela de pooling maior que a matriz original.");
        }
        int outRows = ((int)this->rows - pool_size) / stride + 1;
        int outCols = ((int)this->cols - pool_size) / stride + 1;
        GhostMatrix result(outRows, outCols);

        for (int i = 0; i < outRows; ++i) {
            for (int j = 0; j < outCols; ++j) {
                GhostNumberG maxVal = this->data[i * stride][j * stride];
                for (int pi = 0; pi < pool_size; ++pi) {
                    for (int pj = 0; pj < pool_size; ++pj) {
                        const GhostNumberG& cur = this->data[i * stride + pi][j * stride + pj];
                        if (cur.norm_squared() > maxVal.norm_squared()) {
                            maxVal = cur;
                        }
                    }
                }
                result(i, j) = maxVal;
            }
        }
        return result;
    }

    // Average Pooling 2D
    GhostMatrix avg_pool2d(int pool_size = 2, int stride = 2) const {
        if ((int)this->rows < pool_size || (int)this->cols < pool_size) {
            throw std::invalid_argument("Janela de pooling maior que a matriz original.");
        }
        int outRows = ((int)this->rows - pool_size) / stride + 1;
        int outCols = ((int)this->cols - pool_size) / stride + 1;
        GhostMatrix result(outRows, outCols);
        double factor = 1.0 / (pool_size * pool_size);

        for (int i = 0; i < outRows; ++i) {
            for (int j = 0; j < outCols; ++j) {
                GhostNumberG sum(0.0, 0.0);
                for (int pi = 0; pi < pool_size; ++pi) {
                    for (int pj = 0; pj < pool_size; ++pj) {
                        sum = sum + this->data[i * stride + pi][j * stride + pj];
                    }
                }
                result(i, j) = sum * factor;
            }
        }
        return result;
    }

    // Backpropagation Convolucional: gradiente em relacao ao kernel
    GhostMatrix conv2d_grad_kernel(const GhostMatrix& input, const GhostMatrix& grad_output, int stride = 1) const {
        GhostMatrix grad_kernel(this->rows, this->cols);
        int outRows = (int)grad_output.getRows();
        int outCols = (int)grad_output.getCols();

        for (int ki = 0; ki < (int)this->rows; ++ki) {
            for (int kj = 0; kj < (int)this->cols; ++kj) {
                GhostNumberG sum(0.0, 0.0);
                for (int i = 0; i < outRows; ++i) {
                    for (int j = 0; j < outCols; ++j) {
                        int r = i * stride + ki;
                        int c = j * stride + kj;
                        if (r < (int)input.getRows() && c < (int)input.getCols()) {
                            sum = sum + (input(r, c) * grad_output(i, j));
                        }
                    }
                }
                grad_kernel(ki, kj) = sum;
            }
        }
        return grad_kernel;
    }

    // Backpropagation Convolucional: gradiente em relacao a entrada
    GhostMatrix conv2d_grad_input(const GhostMatrix& grad_output, const GhostMatrix& kernel, int stride = 1) const {
        GhostMatrix grad_input(this->rows, this->cols);
        int outRows = (int)grad_output.getRows();
        int outCols = (int)grad_output.getCols();

        for (int i = 0; i < outRows; ++i) {
            for (int j = 0; j < outCols; ++j) {
                const GhostNumberG& g = grad_output(i, j);
                for (int ki = 0; ki < (int)kernel.getRows(); ++ki) {
                    for (int kj = 0; kj < (int)kernel.getCols(); ++kj) {
                        int r = i * stride + ki;
                        int c = j * stride + kj;
                        if (r < (int)this->rows && c < (int)this->cols) {
                            grad_input(r, c) = grad_input(r, c) + (g * kernel(ki, kj));
                        }
                    }
                }
            }
        }
        return grad_input;
    }

    // Soma Matricial
    GhostMatrix operator+(const GhostMatrix& other) const {
        if (this->rows != other.rows || this->cols != other.cols) {
            throw std::invalid_argument("Dimensoes incompativeis para soma.");
        }
        GhostMatrix result(this->rows, this->cols);
        for (size_t i = 0; i < this->rows; ++i) {
            for (size_t j = 0; j < this->cols; ++j) {
                result(i, j) = this->data[i][j] + other.data[i][j];
            }
        }
        return result;
    }

    // Subtracao Matricial
    GhostMatrix operator-(const GhostMatrix& other) const {
        if (this->rows != other.rows || this->cols != other.cols) {
            throw std::invalid_argument("Dimensoes incompativeis para subtracao.");
        }
        GhostMatrix result(this->rows, this->cols);
        for (size_t i = 0; i < this->rows; ++i) {
            for (size_t j = 0; j < this->cols; ++j) {
                result(i, j) = this->data[i][j] - other.data[i][j];
            }
        }
        return result;
    }

    // Multiplicacao por Escalar Real
    GhostMatrix operator*(double scalar) const {
        GhostMatrix result(this->rows, this->cols);
        for (size_t i = 0; i < this->rows; ++i) {
            for (size_t j = 0; j < this->cols; ++j) {
                GhostNumberG val = this->data[i][j];
                result(i, j) = GhostNumberG(val.x * scalar, 0.0, val.p * scalar);
            }
        }
        return result;
    }

    // Divisao por Escalar Real
    GhostMatrix operator/(double scalar) const {
        if (scalar == 0.0) throw std::invalid_argument("Divisao por zero.");
        GhostMatrix result(this->rows, this->cols);
        for (size_t i = 0; i < this->rows; ++i) {
            for (size_t j = 0; j < this->cols; ++j) {
                GhostNumberG val = this->data[i][j];
                result(i, j) = GhostNumberG(val.x / scalar, 0.0, val.p / scalar);
            }
        }
        return result;
    }

    // Transposicao de Matriz
    GhostMatrix transpose() const {
        GhostMatrix result(this->cols, this->rows);
        for (size_t i = 0; i < this->rows; ++i) {
            for (size_t j = 0; j < this->cols; ++j) {
                result(j, i) = this->data[i][j];
            }
        }
        return result;
    }

    // Produto de Hadamard (Multiplicação elemento-a-elemento)
    GhostMatrix hadamard(const GhostMatrix& other) const {
        if (this->rows != other.rows || this->cols != other.cols) {
            throw std::invalid_argument("Dimensoes incompativeis para Produto de Hadamard.");
        }
        GhostMatrix result(this->rows, this->cols);
        for (size_t i = 0; i < this->rows; ++i) {
            for (size_t j = 0; j < this->cols; ++j) {
                result(i, j) = GhostNumberG(this->data[i][j].x * other.data[i][j].x, 0.0, 0.0);
            }
        }
        return result;
    }

    // Derivada da ReLU (para Backpropagation)
    GhostMatrix relu_derivative() const {
        GhostMatrix result(this->rows, this->cols);
        for (size_t i = 0; i < this->rows; ++i) {
            for (size_t j = 0; j < this->cols; ++j) {
                result(i, j) = GhostNumberG(this->data[i][j].x > 0 ? 1.0 : 0.0, 0.0, 0.0);
            }
        }
        return result;
    }

    // Otimizador Integrado (Backpropagation / Adam para única matriz)
    void optimize(const GhostMatrix& input, const GhostMatrix& target, int epochs, double lr, const std::string& opt) {
        if (opt == "adam") {
            double beta1 = 0.9, beta2 = 0.999, epsilon = 1e-8;
            for(int epoch = 1; epoch <= epochs; ++epoch) {
                this->adam_t++;
                GhostMatrix output = (*this) * input;
                for (size_t r = 0; r < this->rows; ++r) {
                    double error = output(r, 0).x - target(r, 0).x;
                    for (size_t c = 0; c < this->cols; ++c) {
                        double grad = error * input(c, 0).x; // Regra Delta
                        adam_m[r][c] = beta1 * adam_m[r][c] + (1 - beta1) * grad;
                        adam_v[r][c] = beta2 * adam_v[r][c] + (1 - beta2) * grad * grad;
                        double m_hat = adam_m[r][c] / (1 - std::pow(beta1, this->adam_t));
                        double v_hat = adam_v[r][c] / (1 - std::pow(beta2, this->adam_t));
                        this->data[r][c].x -= lr * m_hat / (std::sqrt(v_hat) + epsilon);
                    }
                }
            }
        } else {
            // Gradient Descent padrão
            for(int epoch = 1; epoch <= epochs; ++epoch) {
                GhostMatrix output = (*this) * input;
                for (size_t r = 0; r < this->rows; ++r) {
                    double error = output(r, 0).x - target(r, 0).x;
                    for (size_t c = 0; c < this->cols; ++c) {
                        double grad = error * input(c, 0).x;
                        this->data[r][c].x -= lr * grad;
                    }
                }
            }
        }
    }

    // Aplica a função ReLU a toda a matriz (Colapso Quântico)
    void activateReLU() {
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                data[i][j] = data[i][j].relu();
            }
        }
    }

    // Aplica a função Sigmoid a toda a matriz
    void activateSigmoid() {
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                data[i][j] = data[i][j].sigmoid();
            }
        }
    }

    void activateSin() {
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                data[i][j] = data[i][j].sin();
            }
        }
    }

    void activateCos() {
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                data[i][j] = data[i][j].cos();
            }
        }
    }

    void activateTan() {
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                data[i][j] = data[i][j].tan();
            }
        }
    }

    void activateTanh() {
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                data[i][j] = data[i][j].tanh();
            }
        }
    }

    void activateLeakyReLU(double alpha = 0.01) {
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                data[i][j] = data[i][j].leaky_relu(alpha);
            }
        }
    }

    void activateGELU() {
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                data[i][j] = data[i][j].gelu();
            }
        }
    }

    void activateSoftmax() {
        GhostMatrix sm = softmax();
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                data[i][j] = sm(i, j);
            }
        }
    }

    void display() const {
        for (size_t i = 0; i < rows; ++i) {
            std::cout << "[ ";
            for (size_t j = 0; j < cols; ++j) {
                std::cout << data[i][j].x << "+" << data[i][j].p << "g\t";
            }
            std::cout << "]" << std::endl;
        }
    }

    void set(size_t r, size_t c, const GhostNumberG& val) {
        if (r >= rows || c >= cols) {
            throw std::out_of_range("GhostMatrix::set: indice fora dos limites (" + std::to_string(r) + ", " + std::to_string(c) + ") para matriz " + std::to_string(rows) + "x" + std::to_string(cols));
        }
        data[r][c] = val;
    }

    void set(size_t r, size_t c, double val) {
        set(r, c, GhostNumberG(val, 0.0, 0.0));
    }

    GhostNumberG get(size_t r, size_t c) const {
        if (r >= rows || c >= cols) {
            throw std::out_of_range("GhostMatrix::get: indice fora dos limites (" + std::to_string(r) + ", " + std::to_string(c) + ") para matriz " + std::to_string(rows) + "x" + std::to_string(cols));
        }
        return data[r][c];
    }

    static GhostMatrix identity(size_t n) {
        GhostMatrix res(n, n);
        for (size_t i = 0; i < n; ++i) {
            res(i, i) = GhostNumberG(1.0, 0.0, 0.0);
        }
        return res;
    }

    double determinant() const {
        if (rows != cols) {
            throw std::invalid_argument("GhostMatrix::determinant: matriz deve ser quadrada (" + std::to_string(rows) + "x" + std::to_string(cols) + ")");
        }
        const size_t n = rows;
        if (n == 0) return 1.0;
        if (n == 1) return data[0][0].x;
        if (n == 2) return data[0][0].x * data[1][1].x - data[0][1].x * data[1][0].x;

        std::vector<std::vector<double>> mat(n, std::vector<double>(n));
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                mat[i][j] = data[i][j].x;
            }
        }

        double detVal = 1.0;
        for (size_t col = 0; col < n; ++col) {
            size_t pivot = col;
            double maxVal = std::abs(mat[col][col]);
            for (size_t r = col + 1; r < n; ++r) {
                if (std::abs(mat[r][col]) > maxVal) {
                    maxVal = std::abs(mat[r][col]);
                    pivot = r;
                }
            }
            if (maxVal < 1e-12) return 0.0;
            if (pivot != col) {
                std::swap(mat[col], mat[pivot]);
                detVal = -detVal;
            }
            detVal *= mat[col][col];
            for (size_t r = col + 1; r < n; ++r) {
                double factor = mat[r][col] / mat[col][col];
                for (size_t c = col; c < n; ++c) {
                    mat[r][c] -= factor * mat[col][c];
                }
            }
        }
        return detVal;
    }

    double det() const { return determinant(); }

    GhostMatrix inverse() const {
        if (rows != cols) {
            throw std::invalid_argument("GhostMatrix::inverse: matriz deve ser quadrada (" + std::to_string(rows) + "x" + std::to_string(cols) + ")");
        }
        const size_t n = rows;
        if (n == 0) return GhostMatrix(0, 0);

        std::vector<std::vector<double>> aug(n, std::vector<double>(2 * n, 0.0));
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                aug[i][j] = data[i][j].x;
            }
            aug[i][n + i] = 1.0;
        }

        for (size_t col = 0; col < n; ++col) {
            size_t pivot = col;
            double maxVal = std::abs(aug[col][col]);
            for (size_t r = col + 1; r < n; ++r) {
                if (std::abs(aug[r][col]) > maxVal) {
                    maxVal = std::abs(aug[r][col]);
                    pivot = r;
                }
            }
            if (maxVal < 1e-12) {
                throw std::runtime_error("GhostMatrix::inverse: matriz singular (determinante nulo), nao e invertivel.");
            }
            if (pivot != col) {
                std::swap(aug[col], aug[pivot]);
            }

            double diag = aug[col][col];
            for (size_t c = 0; c < 2 * n; ++c) {
                aug[col][c] /= diag;
            }

            for (size_t r = 0; r < n; ++r) {
                if (r != col) {
                    double factor = aug[r][col];
                    for (size_t c = 0; c < 2 * n; ++c) {
                        aug[r][c] -= factor * aug[col][c];
                    }
                }
            }
        }

        GhostMatrix invMat(n, n);
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                double val = aug[i][n + j];
                if (std::abs(val) < 1e-12) val = 0.0;
                invMat(i, j) = GhostNumberG(val, 0.0, 0.0);
            }
        }
        return invMat;
    }

    GhostMatrix inv() const { return inverse(); }

    void show() const {
        for (size_t i = 0; i < rows; ++i) {
            std::cout << "[ ";
            for (size_t j = 0; j < cols; ++j) {
                const auto& elem = data[i][j];
                if (std::abs(elem.y) < 1e-12 && std::abs(elem.p) < 1e-12 && std::abs(elem.theta2) < 1e-12) {
                    double v = std::abs(elem.x) < 1e-12 ? 0.0 : elem.x;
                    std::cout << std::fixed << std::setprecision(4) << v;
                } else {
                    std::cout << elem.to_math();
                }
                if (j + 1 < cols) std::cout << "  ";
            }
            std::cout << " ]\n";
        }
    }

    inline friend std::ostream& operator<<(std::ostream& os, const GhostMatrix& m) {
        for (size_t i = 0; i < m.rows; ++i) {
            os << "[ ";
            for (size_t j = 0; j < m.cols; ++j) {
                const auto& elem = m.data[i][j];
                if (std::abs(elem.y) < 1e-12 && std::abs(elem.p) < 1e-12 && std::abs(elem.theta2) < 1e-12) {
                    double v = std::abs(elem.x) < 1e-12 ? 0.0 : elem.x;
                    os << std::fixed << std::setprecision(4) << v;
                } else {
                    os << elem.to_math();
                }
                if (j + 1 < m.cols) os << "  ";
            }
            os << " ]";
            if (i + 1 < m.rows) os << "\n";
        }
        return os;
    }
};

// -------------------------------------------------------------
// MOTOR DE DEEP LEARNING (GhostNetwork)
// -------------------------------------------------------------
class GhostHybridNetwork; // GhostHybrid.hpp
// -------------------------------------------------------------
class GhostNetwork {
public:
    std::string name;
    std::vector<GhostMatrix*> layers;
    
    GhostNetwork(const std::string& n, const std::vector<GhostMatrix*>& l) : name(n), layers(l) {}
    
    GhostMatrix forward(const GhostMatrix& input, std::vector<GhostMatrix>& activations, std::vector<GhostMatrix>& z_values) const {
        GhostMatrix current = input;
        activations.push_back(current);
        
        for (size_t i = 0; i < layers.size(); ++i) {
            GhostMatrix z = (*layers[i]) * current;
            for (size_t r = 0; r < z.getRows(); ++r) {
                z(r, 0) = z(r, 0) + layers[i]->bias[r];
            }
            z_values.push_back(z);
            if (i < layers.size() - 1) {
                current = z.relu();
            } else {
                current = z; // Última camada é linear
            }
            activations.push_back(current);
        }
        return current;
    }
    
    // Treino e API iguais aos da rede hibrida (src/GhostNetwork.cpp): o motor GhostHybridNetwork
    // treina estas mesmas camadas no lugar (pesos e bias), com Adam, SGD, momentum, RMSprop ou GD,
    // e guarda o estado do otimizador entre as chamadas. Ate 13/11/2026 havia um laco proprio aqui,
    // cujo gradiente do bias usava so a primeira amostra do lote.
    void optimize(const GhostMatrix& input, const GhostMatrix& target, int epochs, double lr, const std::string& opt);
    double loss(const GhostMatrix& X, const GhostMatrix& Y) const;
    double accuracy(const GhostMatrix& X, const GhostMatrix& Y) const;
    double predict_vec(const std::vector<double>& x) const;
    int classify_vec(const std::vector<double>& x) const;
    template <typename... T> double predict(T... xs) const { return predict_vec({static_cast<double>(xs)...}); }
    template <typename... T> int classify(T... xs) const { return classify_vec({static_cast<double>(xs)...}); }
    void set_verbose(int every_epochs);
    std::string summary() const;
    void display() const;
private:
    mutable std::shared_ptr<GhostHybridNetwork> motor_;
    GhostHybridNetwork& motor() const;
};

// Sobrecargas globais para permitir o uso de 'rede * vetor' no GhostLang
inline GhostMatrix operator*(const GhostNetwork& net, const GhostMatrix& input) {
    std::vector<GhostMatrix> acts, zs;
    return net.forward(input, acts, zs);
}

inline GhostMatrix operator*(GhostNetwork* net, const GhostMatrix& input) {
    std::vector<GhostMatrix> acts, zs;
    return net->forward(input, acts, zs);
}

#endif