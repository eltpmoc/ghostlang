#include "GhostDevice.hpp"
#include <atomic>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <stdexcept>

#ifdef GHOST_HAS_CUDA
extern "C" int ghost_cuda_probe(char*, int);
extern "C" int ghost_cuda_matmul(const ghost::PackedG*,const ghost::PackedG*,ghost::PackedG*,int,int,int,char*,int);
#endif
namespace {
std::mutex lock;
std::string policy,description,failure;
bool initialized=false, available=false;
std::atomic<uint64_t> matrix_calls{0};
void initialize() {
    if (initialized) return;
    if (policy.empty()) {
        const char* env=std::getenv("GHOST_DEVICE");
        policy=env?env:"auto";
        if (policy=="gpu") policy="cuda";
    }
    if (policy=="vulkan" || policy=="metal" || policy=="rocm")
        throw std::invalid_argument("GHOST_DEVICE/use_device: o backend " + policy + " ainda nao foi implementado (disponiveis: auto, cpu, cuda).");
    if (policy!="auto" && policy!="cpu" && policy!="cuda")
        throw std::invalid_argument("GHOST_DEVICE/use_device: use auto, cpu ou cuda.");
    available=false;
    failure="runtime compilado sem CUDA";
    if (policy=="cuda") {
#ifdef GHOST_HAS_CUDA
        char info[512]{};
        available=ghost_cuda_probe(info,sizeof(info));
        if (available) description=info; else failure=info;
#endif
        if (!available) throw std::runtime_error("CUDA obrigatorio indisponivel: "+failure);
    } else if (policy=="auto") {
#ifdef GHOST_HAS_CUDA
        char info[512]{};
        available=ghost_cuda_probe(info,sizeof(info));
        if (available) description=info; else failure=info;
#else
        available=false;
        description="CPU Host (OpenMP multi-threaded)";
#endif
    }
    initialized=true;
}
}
namespace ghost {
void set_device(const std::string& value) {
    std::lock_guard<std::mutex> guard(lock);
    const std::string next=value=="gpu"?"cuda":value;
    if (next=="vulkan" || next=="metal" || next=="rocm")
        throw std::invalid_argument("use_device: o backend " + next + " ainda nao foi implementado (disponiveis: auto, cpu, cuda).");
    if (next!="auto" && next!="cpu" && next!="cuda")
        throw std::invalid_argument("use_device: use auto, cpu ou cuda.");
    policy=next; initialized=false; initialize();
}
std::string backend() {
    std::lock_guard<std::mutex> guard(lock); initialize();
    return available?"cuda":"cpu";
}
std::string device_info() {
    std::lock_guard<std::mutex> guard(lock); initialize();
    if (available) return "CUDA: "+description+"; politica="+policy+(policy=="auto"?" (matrizes pequenas usam CPU)":"");
    return "CPU; politica="+policy+(policy=="auto"?"; CUDA indisponivel: "+failure:"");
}
bool cuda_available() { return backend()=="cuda"; }
bool device_supports(const std::string& name) {
    if (name=="cpu" || name=="auto") return true;
    if (name=="cuda") {
#ifdef GHOST_HAS_CUDA
        return true;
#else
        return false;
#endif
    }
    return false;   // vulkan, metal e rocm ainda não foram implementados
}
// Backends que este runtime realmente tem (cuda só quando compilado com CUDA)
std::string heterogeneous_devices() {
#ifdef GHOST_HAS_CUDA
    return "[\"cpu\", \"cuda\"]";
#else
    return "[\"cpu\"]";
#endif
}
bool accelerate_matrix(size_t r,size_t k,size_t c) {
    std::lock_guard<std::mutex> guard(lock); initialize();
    if (!available || !r || !k || !c) return false;
    // O empacotamento e as transferencias nao compensam para produtos pequenos.
    return policy=="cuda" || static_cast<long double>(r)*k*c>=262144;
}
void multiply_cuda(const PackedG* a,const PackedG* b,PackedG* out,size_t r,size_t k,size_t c) {
#ifdef GHOST_HAS_CUDA
    if (r>INT32_MAX || k>INT32_MAX || c>INT32_MAX) throw std::invalid_argument("Dimensoes CUDA excedem int32.");
    char error[512]{};
    if (!ghost_cuda_matmul(a,b,out,r,k,c,error,sizeof(error))) throw std::runtime_error(std::string("CUDA matmul: ")+error);
    ++matrix_calls;
#else
    throw std::runtime_error("Runtime sem CUDA.");
#endif
}
uint64_t cuda_matrix_calls() { return matrix_calls.load(); }
}
