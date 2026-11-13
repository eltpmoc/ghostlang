#include "../include/GhostNDTensor.hpp"
#include <map>
#include <mutex>

namespace GhostAlgebra {

static std::map<int, GhostNDTensor> g_tensors;
static int g_tensor_counter = 1;
static std::mutex g_tensor_mtx;
static std::string g_str_buffer;

} // namespace GhostAlgebra

using namespace GhostAlgebra;

extern "C" {

int ghost_tensor_create(const int* shape, int ndim, double init_val) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    std::vector<int> shp(shape, shape + ndim);
    int id = g_tensor_counter++;
    g_tensors[id] = GhostNDTensor(shp, init_val);
    return id;
}

int ghost_tensor_from_data(const int* shape, int ndim, const double* data, int data_len) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    std::vector<int> shp(shape, shape + ndim);
    std::vector<double> dt(data, data + data_len);
    int id = g_tensor_counter++;
    g_tensors[id] = GhostNDTensor(shp, dt);
    return id;
}

void ghost_tensor_free(int handle) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    g_tensors.erase(handle);
}

int ghost_tensor_ndim(int handle) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) return it->second.ndim();
    return 0;
}

int ghost_tensor_dim(int handle, int axis) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        const auto& shp = it->second.shape();
        if (axis >= 0 && axis < (int)shp.size()) return shp[axis];
    }
    return 0;
}

int ghost_tensor_size(int handle) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) return it->second.size();
    return 0;
}

double ghost_tensor_get(int handle, const int* indices, int idx_len) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        std::vector<int> idx(indices, indices + idx_len);
        return it->second.get(idx);
    }
    return 0.0;
}

void ghost_tensor_set(int handle, const int* indices, int idx_len, double val) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        std::vector<int> idx(indices, indices + idx_len);
        it->second.set(idx, val);
    }
}

double ghost_tensor_get_flat(int handle, int i) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) return it->second.get_flat(i);
    return 0.0;
}

void ghost_tensor_set_flat(int handle, int i, double val) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) it->second.set_flat(i, val);
}

int ghost_tensor_reshape(int handle, const int* new_shape, int new_ndim) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        std::vector<int> shp(new_shape, new_shape + new_ndim);
        int id = g_tensor_counter++;
        g_tensors[id] = it->second.reshape(shp);
        return id;
    }
    return 0;
}

int ghost_tensor_transpose(int handle, const int* perm, int perm_len) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        std::vector<int> p(perm, perm + perm_len);
        int id = g_tensor_counter++;
        g_tensors[id] = it->second.transpose(p);
        return id;
    }
    return 0;
}

int ghost_tensor_slice(int handle, int start, int end) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        int id = g_tensor_counter++;
        g_tensors[id] = it->second.slice(start, end);
        return id;
    }
    return 0;
}

int ghost_tensor_add(int hA, int hB) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto itA = g_tensors.find(hA);
    auto itB = g_tensors.find(hB);
    if (itA != g_tensors.end() && itB != g_tensors.end()) {
        int id = g_tensor_counter++;
        g_tensors[id] = itA->second.add(itB->second);
        return id;
    }
    return 0;
}

int ghost_tensor_sub(int hA, int hB) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto itA = g_tensors.find(hA);
    auto itB = g_tensors.find(hB);
    if (itA != g_tensors.end() && itB != g_tensors.end()) {
        int id = g_tensor_counter++;
        g_tensors[id] = itA->second.sub(itB->second);
        return id;
    }
    return 0;
}

int ghost_tensor_mul(int hA, int hB) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto itA = g_tensors.find(hA);
    auto itB = g_tensors.find(hB);
    if (itA != g_tensors.end() && itB != g_tensors.end()) {
        int id = g_tensor_counter++;
        g_tensors[id] = itA->second.mul(itB->second);
        return id;
    }
    return 0;
}

int ghost_tensor_div(int hA, int hB) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto itA = g_tensors.find(hA);
    auto itB = g_tensors.find(hB);
    if (itA != g_tensors.end() && itB != g_tensors.end()) {
        int id = g_tensor_counter++;
        g_tensors[id] = itA->second.div(itB->second);
        return id;
    }
    return 0;
}

int ghost_tensor_matmul(int hA, int hB) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto itA = g_tensors.find(hA);
    auto itB = g_tensors.find(hB);
    if (itA != g_tensors.end() && itB != g_tensors.end()) {
        int id = g_tensor_counter++;
        g_tensors[id] = itA->second.matmul(itB->second);
        return id;
    }
    return 0;
}

double ghost_tensor_sum(int handle) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) return it->second.sum();
    return 0.0;
}

double ghost_tensor_mean(int handle) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) return it->second.mean();
    return 0.0;
}

const char* ghost_tensor_to_string(int handle) {
    std::lock_guard<std::mutex> lock(g_tensor_mtx);
    auto it = g_tensors.find(handle);
    if (it != g_tensors.end()) {
        g_str_buffer = it->second.to_string();
        return g_str_buffer.c_str();
    }
    return "Tensor[null]";
}

} // extern "C"
