// GhostLang Dataset Library (libs/dataset.g)
function int dataset_load_csv(string path, bool has_header = true) {
    return ghost_dataset_create_csv(path, has_header);
}

function int dataset_from_matrix(int rows, int cols, list<double> vals) {
    return ghost_dataset_create_matrix(rows, cols, vals);
}

function int dataset_len(int handle) {
    return ghost_dataset_size(handle);
}

function int dataset_num_features(int handle) {
    return ghost_dataset_features(handle);
}

function list<int> dataset_train_test_split(int handle, double ratio = 0.8) {
    return ghost_dataset_split(handle, ratio);
}

function void dataset_make_batches(int handle, int bsize) {
    ghost_dataset_create_batches(handle, bsize);
}

function int dataset_get_num_batches(int handle) {
    return ghost_dataset_num_batches(handle);
}

function double dataset_cell(int handle, int row, int col) {
    return ghost_dataset_get_item(handle, row, col);
}
