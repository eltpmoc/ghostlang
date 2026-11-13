// tensor_matmul maior que um bloco (70x50 por 50x40): o resultado tem de ser o mesmo com o
// OpenBLAS (GHOST_USE_BLAS=ON) e com o laço nativo. Valores inteiros pequenos tornam as
// somas exatas em double, então a ordem das somas não muda nada.
import "../../libs/tensor.g"

function main() {
    int a = tensor_create_2d(70, 50, 0.0);
    int b = tensor_create_2d(50, 40, 0.0);
    for (int i = 0; i < 70; i++) {
        for (int p = 0; p < 50; p++) {
            tensor_set_2d(a, i, p, (i % 7) - 3 + (p % 5));
        }
    }
    for (int p = 0; p < 50; p++) {
        for (int j = 0; j < 40; j++) {
            tensor_set_2d(b, p, j, (p % 4) - (j % 3) + 1);
        }
    }
    int c = tensor_matmul(a, b);
    print(tensor_dim(c, 0), "x", tensor_dim(c, 1));
    print(tensor_get_2d(c, 0, 0));
    print(tensor_get_2d(c, 33, 17));
    print(tensor_get_2d(c, 69, 39));
    print(tensor_sum(c));
}
