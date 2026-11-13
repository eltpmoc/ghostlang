import "libs/autograd.g" as ag;

function main() {
    var W = ag.tensor_create(2, 2, true, 0.5);
    var X = ag.tensor_create(1, 2, false, 2.0);
    var Y = ag.tensor_matmul(X, W);
    print(ag.tensor_get(Y, 0, 0));
    print(ag.tensor_get(Y, 0, 1));
    var Target = ag.tensor_create(1, 2, false, 1.0);
    var Loss = ag.tensor_mse(Y, Target);
    print(ag.tensor_get(Loss, 0, 0));
    ag.tensor_backward(Loss);
    print(ag.tensor_grad(W, 0, 0) > 0.0);
    ag.tensor_step(W, 0.1);
    print(ag.tensor_get(W, 0, 0) < 0.5);
}
