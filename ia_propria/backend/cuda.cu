// Backend opcional e especifico da IA 168 -> 32 -> 42. Nao modifica GhostLang.
#include <cuda_runtime.h>
#include <cmath>
#include <cstdio>
#include <string>

namespace {
constexpr int NI = 168, NH = 32, NO = 42;
constexpr int B1 = NI * NH, W2 = B1 + NH, B2 = W2 + NH * NO, NP = B2 + NO;
struct Storage {
    double *weights = nullptr, *moment = nullptr, *variance = nullptr;
    double *sample = nullptr, *result = nullptr;
    int step = 0;
    ~Storage() {
        cudaFree(weights); cudaFree(moment); cudaFree(variance);
        cudaFree(sample); cudaFree(result);
    }
} storage;
std::string error_message;
bool check(cudaError_t error) {
    if (error == cudaSuccess) return true;
    error_message = cudaGetErrorString(error);
    return false;
}
__global__ void probe(double* result) { result[0] = 2.0 + 3.0; }

__global__ void step_kernel(double* w, double* m, double* v,
                            const double* sample, double* result,
                            bool training, double rate, double correction1, double correction2) {
    __shared__ double hidden[NH], output[NO], delta[NO], hidden_delta[NH];
    int t = threadIdx.x;
    if (t < NH) {
        double value = 0.0;
        for (int c = 0; c < NI; ++c) value += w[t * NI + c] * sample[c];
        value += w[B1 + t];
        hidden[t] = value > 0.0 ? value : 0.0;
    }
    __syncthreads();
    if (t < NO) {
        double value = 0.0;
        for (int c = 0; c < NH; ++c) value += w[W2 + t * NH + c] * hidden[c];
        value += w[B2 + t];
        output[t] = value;
        result[t] = value;
        if (training) delta[t] = value - sample[NI + t];
    }
    __syncthreads();
    if (!training) return; // Mesmo caminho para todas as threads do bloco.
    if (t < NH) {
        double value = 0.0;
        for (int r = 0; r < NO; ++r) value += w[W2 + r * NH + t] * delta[r];
        hidden_delta[t] = hidden[t] > 0.0 ? value : 0.0;
    }
    // Todo delta usa os pesos antigos, antes de qualquer atualizacao.
    __syncthreads();
    for (int i = t; i < NP; i += blockDim.x) {
        double gradient;
        if (i < B1) gradient = hidden_delta[i / NI] * sample[i % NI];
        else if (i < W2) gradient = hidden_delta[i - B1];
        else if (i < B2) gradient = delta[(i - W2) / NH] * hidden[(i - W2) % NH];
        else gradient = delta[i - B2];
        m[i] = 0.9 * m[i] + (1.0 - 0.9) * gradient;
        v[i] = 0.999 * v[i] + (1.0 - 0.999) * gradient * gradient;
        double corrected_m = m[i] / correction1;
        double corrected_v = v[i] / correction2;
        w[i] -= rate * corrected_m / (sqrt(corrected_v) + 1e-8);
    }
}
}

extern "C" const char* ia_cuda_error() { return error_message.c_str(); }

extern "C" int ia_cuda_init(char* description, int capacity) {
    int count = 0;
    if (!check(cudaGetDeviceCount(&count))) return 0;
    if (count == 0) { error_message = "Nenhuma GPU CUDA visivel"; return 0; }
    if (!check(cudaSetDevice(0))) return 0;
    cudaDeviceProp properties{};
    if (!check(cudaGetDeviceProperties(&properties, 0))) return 0;
    if (!check(cudaMalloc(&storage.weights, NP * sizeof(double))) ||
        !check(cudaMalloc(&storage.moment, NP * sizeof(double))) ||
        !check(cudaMalloc(&storage.variance, NP * sizeof(double))) ||
        !check(cudaMalloc(&storage.sample, (NI + NO) * sizeof(double))) ||
        !check(cudaMalloc(&storage.result, NO * sizeof(double)))) return 0;
    probe<<<1, 1>>>(storage.result);
    double observed = 0.0;
    if (!check(cudaGetLastError()) ||
        !check(cudaMemcpy(&observed, storage.result, sizeof(double), cudaMemcpyDeviceToHost))) return 0;
    if (observed != 5.0) { error_message = "Autoteste CUDA retornou resultado incorreto"; return 0; }
    std::snprintf(description, capacity, "%s (%zu MiB)", properties.name,
                  properties.totalGlobalMem / (1024 * 1024));
    return 1;
}

extern "C" int ia_cuda_upload(const double* weights) {
    storage.step = 0;
    return check(cudaMemcpy(storage.weights, weights, NP * sizeof(double), cudaMemcpyHostToDevice)) &&
           check(cudaMemset(storage.moment, 0, NP * sizeof(double))) &&
           check(cudaMemset(storage.variance, 0, NP * sizeof(double)));
}

extern "C" int ia_cuda_download(double* weights) {
    return check(cudaMemcpy(weights, storage.weights, NP * sizeof(double), cudaMemcpyDeviceToHost));
}

extern "C" int ia_cuda_predict(const double* input, double* output) {
    if (!check(cudaMemcpy(storage.sample, input, NI * sizeof(double), cudaMemcpyHostToDevice))) return 0;
    step_kernel<<<1, 256>>>(storage.weights, storage.moment, storage.variance,
                           storage.sample, storage.result, false, 0.0, 1.0, 1.0);
    return check(cudaGetLastError()) &&
           check(cudaMemcpy(output, storage.result, NO * sizeof(double), cudaMemcpyDeviceToHost));
}

extern "C" int ia_cuda_train(const double* sample, double rate) {
    if (!check(cudaMemcpy(storage.sample, sample, (NI + NO) * sizeof(double), cudaMemcpyHostToDevice))) return 0;
    ++storage.step;
    step_kernel<<<1, 256>>>(storage.weights, storage.moment, storage.variance,
                           storage.sample, storage.result, true, rate,
                           1.0 - std::pow(0.9, storage.step), 1.0 - std::pow(0.999, storage.step));
    return check(cudaGetLastError()) && check(cudaDeviceSynchronize());
}
