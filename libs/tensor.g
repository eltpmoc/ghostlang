// GhostLang N-Dimensional Tensor & Broadcasting Library (libs/tensor.g)

function int tensor_create_1d(int n, double init_val) {
    list<int> shape = [n];
    return ghost_tensor_create(shape, 1, init_val);
}

function int tensor_create_2d(int r, int c, double init_val) {
    list<int> shape = [r, c];
    return ghost_tensor_create(shape, 2, init_val);
}

function int tensor_create_3d(int d1, int d2, int d3, double init_val) {
    list<int> shape = [d1, d2, d3];
    return ghost_tensor_create(shape, 3, init_val);
}

function int tensor_create_4d(int d1, int d2, int d3, int d4, double init_val) {
    list<int> shape = [d1, d2, d3, d4];
    return ghost_tensor_create(shape, 4, init_val);
}

function void tensor_free(int handle) {
    ghost_tensor_free(handle);
}

function int tensor_ndim(int handle) {
    return ghost_tensor_ndim(handle);
}

function int tensor_dim(int handle, int axis) {
    return ghost_tensor_dim(handle, axis);
}

function int tensor_size(int handle) {
    return ghost_tensor_size(handle);
}

function double tensor_get_2d(int handle, int r, int c) {
    list<int> idx = [r, c];
    return ghost_tensor_get(handle, idx, 2);
}

function void tensor_set_2d(int handle, int r, int c, double val) {
    list<int> idx = [r, c];
    ghost_tensor_set(handle, idx, 2, val);
}

function double tensor_get_3d(int handle, int d1, int d2, int d3) {
    list<int> idx = [d1, d2, d3];
    return ghost_tensor_get(handle, idx, 3);
}

function void tensor_set_3d(int handle, int d1, int d2, int d3, double val) {
    list<int> idx = [d1, d2, d3];
    ghost_tensor_set(handle, idx, 3, val);
}

function double tensor_get_flat(int handle, int i) {
    return ghost_tensor_get_flat(handle, i);
}

function void tensor_set_flat(int handle, int i, double val) {
    ghost_tensor_set_flat(handle, i, val);
}

function int tensor_reshape_2d(int handle, int r, int c) {
    list<int> shape = [r, c];
    return ghost_tensor_reshape(handle, shape, 2);
}

function int tensor_transpose_2d(int handle) {
    list<int> perm = [1, 0];
    return ghost_tensor_transpose(handle, perm, 2);
}

function int tensor_slice(int handle, int start, int fim) {
    return ghost_tensor_slice(handle, start, fim);
}

function int tensor_add(int hA, int hB) {
    return ghost_tensor_add(hA, hB);
}

function int tensor_sub(int hA, int hB) {
    return ghost_tensor_sub(hA, hB);
}

function int tensor_mul(int hA, int hB) {
    return ghost_tensor_mul(hA, hB);
}

function int tensor_div(int hA, int hB) {
    return ghost_tensor_div(hA, hB);
}

function int tensor_matmul(int hA, int hB) {
    return ghost_tensor_matmul(hA, hB);
}

function double tensor_sum(int handle) {
    return ghost_tensor_sum(handle);
}

function double tensor_mean(int handle) {
    return ghost_tensor_mean(handle);
}

function string tensor_to_str(int handle) {
    return ghost_tensor_to_string(handle);
}
