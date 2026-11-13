import "../../libs/tensor.g"

function main() {
    int t2d = tensor_create_2d(2, 3, 0.0);
    print(tensor_ndim(t2d));
    print(tensor_size(t2d));

    tensor_set_2d(t2d, 0, 0, 1.0);
    tensor_set_2d(t2d, 0, 1, 2.0);
    tensor_set_2d(t2d, 0, 2, 3.0);
    tensor_set_2d(t2d, 1, 0, 4.0);
    tensor_set_2d(t2d, 1, 1, 5.0);
    tensor_set_2d(t2d, 1, 2, 6.0);

    print(tensor_get_2d(t2d, 0, 1));
    print(tensor_get_2d(t2d, 1, 2));
    print(tensor_sum(t2d));
    print(tensor_mean(t2d));

    // Reshape 2x3 -> 3x2
    int t_reshaped = tensor_reshape_2d(t2d, 3, 2);
    print(tensor_dim(t_reshaped, 0));
    print(tensor_dim(t_reshaped, 1));

    // Transpose 2x3 -> 3x2
    int t_transposed = tensor_transpose_2d(t2d);
    print(tensor_get_2d(t_transposed, 1, 0));

    // MatMul: (2x3) x (3x2) -> (2x2)
    int t_mm = tensor_matmul(t2d, t_transposed);
    print(tensor_get_2d(t_mm, 0, 0));

    // Broadcasting: Tensor (2x3) + Tensor (1x3)
    int t_row = tensor_create_2d(1, 3, 10.0);
    int t_bcast = tensor_add(t2d, t_row);
    print(tensor_get_2d(t_bcast, 0, 0));
    print(tensor_get_2d(t_bcast, 1, 2));
}
