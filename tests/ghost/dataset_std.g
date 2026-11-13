import "libs/dataset.g" as ds;

function main() {
    var dados = [1.0, 2.0, 10.0, 3.0, 4.0, 20.0, 5.0, 6.0, 30.0, 7.0, 8.0, 40.0];
    var d = ds.dataset_from_matrix(4, 3, dados);
    print(ds.dataset_len(d));
    print(ds.dataset_num_features(d));
    print(ds.dataset_cell(d, 0, 0));
    print(ds.dataset_cell(d, 0, 2));
    ds.dataset_make_batches(d, 2);
    print(ds.dataset_get_num_batches(d));
}
