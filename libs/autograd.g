// GhostLang Autograd Library (libs/autograd.g)
function int tensor_create(int rows, int cols, bool req_grad = false, double init_val = 0.0) {
    return ghost_autograd_create(rows, cols, req_grad, init_val);
}

function void tensor_set(int handle, int r, int c, double val) {
    ghost_autograd_set(handle, r, c, val);
}

function double tensor_get(int handle, int r, int c) {
    return ghost_autograd_get(handle, r, c);
}

function double tensor_grad(int handle, int r, int c) {
    return ghost_autograd_grad(handle, r, c);
}

function int tensor_matmul(int hA, int hB) {
    return ghost_autograd_matmul(hA, hB);
}

function int tensor_add(int hA, int hB) {
    return ghost_autograd_add(hA, hB);
}

function int tensor_relu(int hA) {
    return ghost_autograd_relu(hA);
}

function int tensor_mse(int hPred, int hTarget) {
    return ghost_autograd_mse(hPred, hTarget);
}

function void tensor_backward(int hLoss) {
    ghost_autograd_backward(hLoss);
}

function void tensor_step(int handle, double taxa) {
    ghost_autograd_step(handle, taxa);
}

function void tensor_zero_grad(int handle) {
    ghost_autograd_zero_grad(handle);
}
