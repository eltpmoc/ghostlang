#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace ghost {
struct PackedG { double x, y, p; };
void set_device(const std::string& policy);
std::string device_info();
std::string backend();
bool cuda_available();
bool device_supports(const std::string& name);
std::string heterogeneous_devices();
bool accelerate_matrix(size_t rows, size_t inner, size_t cols);
void multiply_cuda(const PackedG* a, const PackedG* b, PackedG* out, size_t rows, size_t inner, size_t cols);
uint64_t cuda_matrix_calls();
}

// Funcoes publicas disponiveis diretamente em programas GhostLang.
inline void use_device(const std::string& policy) { ghost::set_device(policy); }
inline std::string device_info() { return ghost::device_info(); }
inline std::string device_backend() { return ghost::backend(); }
inline bool gpu_available() { return ghost::cuda_available(); }
inline bool device_supports(const std::string& name) { return ghost::device_supports(name); }
inline std::string heterogeneous_devices() { return ghost::heterogeneous_devices(); }
inline uint64_t gpu_matrix_calls() { return ghost::cuda_matrix_calls(); }
