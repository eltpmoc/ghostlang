#pragma once
#include "GhostMatrix.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#ifdef IA_COM_CUDA
extern "C" int ia_cuda_init(char*, int);
extern "C" const char* ia_cuda_error();
extern "C" int ia_cuda_upload(const double*);
extern "C" int ia_cuda_download(double*);
extern "C" int ia_cuda_predict(const double*, double*);
extern "C" int ia_cuda_train(const double*, double);
#endif

inline bool ia_gpu_ativa = false;
inline bool ia_pesos_enviados = false;
inline void ia_backend_erro(const std::string& reason) {
    std::cerr << "[IA] Erro de backend: " << reason << '\n';
    std::exit(1);
}

inline int ia_iniciar() {
    const char* setting = std::getenv("IA_DEVICE");
    std::string requested = setting ? setting : "auto";
    if (requested != "auto" && requested != "cpu" && requested != "gpu")
        ia_backend_erro("IA_DEVICE deve ser auto, cpu ou gpu.");
    if (requested != "cpu") {
#ifdef IA_COM_CUDA
        char description[256]{};
        if (ia_cuda_init(description, sizeof(description))) {
            ia_gpu_ativa = true;
            std::cerr << "[IA] Dispositivo: GPU CUDA | " << description << " | autoteste OK\n";
            return 1;
        }
        if (requested == "gpu") ia_backend_erro(ia_cuda_error());
        std::cerr << "[IA] CUDA indisponivel: " << ia_cuda_error() << "; usando CPU.\n";
#else
        if (requested == "gpu") ia_backend_erro("Executavel sem backend CUDA. Recompile com CUDA.");
        std::cerr << "[IA] Backend CUDA nao compilado; usando CPU.\n";
#endif
    }
    std::cerr << "[IA] Dispositivo: CPU | GhostNetwork/OpenMP\n";
    return 0;
}

inline void ia_validar_rede(GhostNetwork* net) {
    if (net->layers.size() != 2 || net->layers[0]->getRows() != 32 ||
        net->layers[0]->getCols() != 168 || net->layers[1]->getRows() != 42 ||
        net->layers[1]->getCols() != 32)
        ia_backend_erro("CUDA suporta somente a arquitetura 168 -> 32 -> 42.");
}

inline void ia_enviar_pesos(GhostNetwork* net) {
#ifdef IA_COM_CUDA
    if (ia_pesos_enviados) return;
    ia_validar_rede(net);
    std::vector<double> weights;
    weights.reserve(6794);
    for (auto* layer : net->layers) {
        for (size_t r = 0; r < layer->getRows(); ++r)
            for (size_t c = 0; c < layer->getCols(); ++c) weights.push_back((*layer)(r,c).x);
        for (const auto& bias : layer->bias) weights.push_back(bias.x);
    }
    if (!ia_cuda_upload(weights.data())) ia_backend_erro(ia_cuda_error());
    ia_pesos_enviados = true;
#endif
}

inline GhostMatrix ia_prever(GhostNetwork* net, const GhostMatrix& input) {
    if (!ia_gpu_ativa) return net * input;
#ifdef IA_COM_CUDA
    ia_enviar_pesos(net);
    if (input.getRows() != 168 || input.getCols() != 1) ia_backend_erro("Entrada CUDA invalida.");
    double values[168], output[42];
    for (int i = 0; i < 168; ++i) values[i] = input(i,0).x;
    if (!ia_cuda_predict(values, output)) ia_backend_erro(ia_cuda_error());
    GhostMatrix result(42, 1);
    for (int i = 0; i < 42; ++i) result(i,0).x = output[i];
    return result;
#else
    ia_backend_erro("GPU indisponivel.");
    return GhostMatrix();
#endif
}

inline void ia_treinar_gpu(GhostNetwork* net, const GhostMatrix& input,
                           const GhostMatrix& target, double rate) {
#ifdef IA_COM_CUDA
    ia_enviar_pesos(net);
    if (input.getRows() != 168 || input.getCols() != 1 || target.getRows() != 42 || target.getCols() != 1)
        ia_backend_erro("Amostra CUDA invalida.");
    double sample[210];
    for (int i = 0; i < 168; ++i) sample[i] = input(i,0).x;
    for (int i = 0; i < 42; ++i) sample[168+i] = target(i,0).x;
    if (!ia_cuda_train(sample, rate)) ia_backend_erro(ia_cuda_error());
#else
    ia_backend_erro("GPU indisponivel.");
#endif
}

inline void ia_sincronizar(GhostNetwork* net) {
#ifdef IA_COM_CUDA
    if (!ia_gpu_ativa || !ia_pesos_enviados) return;
    double weights[6794];
    if (!ia_cuda_download(weights)) ia_backend_erro(ia_cuda_error());
    int i = 0;
    for (auto* layer : net->layers) {
        for (size_t r = 0; r < layer->getRows(); ++r)
            for (size_t c = 0; c < layer->getCols(); ++c) (*layer)(r,c).x = weights[i++];
        for (auto& bias : layer->bias) bias.x = weights[i++];
    }
#endif
}

inline void ia_invalidar() { ia_pesos_enviados = false; }
