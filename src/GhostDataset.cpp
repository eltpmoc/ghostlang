#include "../include/GhostDataset.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <mutex>

namespace GhostData {

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::shared_ptr<GhostDataset> GhostDataset::loadCSV(const std::string& path, bool has_header) {
    auto ds = std::make_shared<GhostDataset>();
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[GhostDataset] Nao foi possivel abrir CSV: " << path << std::endl;
        return ds;
    }

    std::string line;
    bool first = true;
    while (std::getline(file, line)) {
        std::string t = trim(line);
        if (t.empty()) continue;

        std::stringstream ss(t);
        std::string cell;
        std::vector<std::string> rowTokens;
        while (std::getline(ss, cell, ',')) {
            rowTokens.push_back(trim(cell));
        }

        if (first && has_header) {
            ds->headers = rowTokens;
            first = false;
            continue;
        }
        first = false;

        if (rowTokens.empty()) continue;

        std::vector<double> rowVals;
        for (size_t i = 0; i < rowTokens.size() - 1; ++i) {
            try {
                rowVals.push_back(std::stod(rowTokens[i]));
            } catch (...) {
                rowVals.push_back(0.0);
            }
        }

        double target = 0.0;
        try {
            target = std::stod(rowTokens.back());
        } catch (...) {
            target = 0.0;
        }

        ds->data.push_back(rowVals);
        ds->targets.push_back(target);
    }
    return ds;
}

std::shared_ptr<GhostDataset> GhostDataset::fromMatrix(size_t rows, size_t cols, const std::vector<double>& vals, bool last_col_is_target) {
    auto ds = std::make_shared<GhostDataset>();
    if (rows == 0 || cols == 0) return ds;

    for (size_t r = 0; r < rows; ++r) {
        std::vector<double> row;
        size_t featCols = (last_col_is_target && cols > 1) ? (cols - 1) : cols;
        for (size_t c = 0; c < featCols; ++c) {
            size_t idx = r * cols + c;
            row.push_back(idx < vals.size() ? vals[idx] : 0.0);
        }
        ds->data.push_back(row);

        double tgt = 0.0;
        if (last_col_is_target && cols > 1) {
            size_t idx = r * cols + (cols - 1);
            tgt = (idx < vals.size()) ? vals[idx] : 0.0;
        }
        ds->targets.push_back(tgt);
    }
    return ds;
}

void GhostDataset::split(double train_ratio, std::shared_ptr<GhostDataset>& train_set, std::shared_ptr<GhostDataset>& test_set, int seed) {
    train_set = std::make_shared<GhostDataset>();
    test_set = std::make_shared<GhostDataset>();

    train_set->headers = headers;
    test_set->headers = headers;

    size_t total = data.size();
    if (total == 0) return;

    std::vector<size_t> indices(total);
    for (size_t i = 0; i < total; ++i) indices[i] = i;

    std::mt19937 g(seed);
    std::shuffle(indices.begin(), indices.end(), g);

    size_t splitPoint = static_cast<size_t>(total * train_ratio);
    if (splitPoint == 0 && total > 0) splitPoint = 1;

    for (size_t i = 0; i < total; ++i) {
        size_t idx = indices[i];
        if (i < splitPoint) {
            train_set->data.push_back(data[idx]);
            train_set->targets.push_back(targets[idx]);
        } else {
            test_set->data.push_back(data[idx]);
            test_set->targets.push_back(targets[idx]);
        }
    }
}

void GhostDataset::createBatches(size_t bsize) {
    batch_size = (bsize == 0) ? 1 : bsize;
    batches.clear();

    size_t total = data.size();
    for (size_t i = 0; i < total; i += batch_size) {
        DatasetBatch b;
        size_t end = std::min(i + batch_size, total);
        b.size = end - i;
        for (size_t k = i; k < end; ++k) {
            b.features.push_back(data[k]);
            b.targets.push_back(targets[k]);
        }
        batches.push_back(b);
    }
}

const DatasetBatch& GhostDataset::getBatch(size_t idx) const {
    static DatasetBatch empty;
    if (idx >= batches.size()) return empty;
    return batches[idx];
}

double GhostDataset::getItem(size_t row, size_t col) const {
    if (row >= data.size()) return 0.0;
    if (col < data[row].size()) return data[row][col];
    if (col == data[row].size() && row < targets.size()) return targets[row];
    return 0.0;
}

// -------------------------------------------------------------
// TABELA GLOBAL DE DATASETS
// -------------------------------------------------------------

static std::mutex g_dataset_mtx;
static std::map<int, DatasetPtr> g_datasets;
static int g_dataset_counter = 1;

int ghost_dataset_create_csv(const std::string& path, bool has_header) {
    std::lock_guard<std::mutex> lock(g_dataset_mtx);
    auto ds = GhostDataset::loadCSV(path, has_header);
    int id = g_dataset_counter++;
    g_datasets[id] = ds;
    return id;
}

int ghost_dataset_create_matrix(int rows, int cols, const std::vector<double>& vals) {
    std::lock_guard<std::mutex> lock(g_dataset_mtx);
    auto ds = GhostDataset::fromMatrix(rows, cols, vals, true);
    int id = g_dataset_counter++;
    g_datasets[id] = ds;
    return id;
}

int ghost_dataset_size(int handle) {
    std::lock_guard<std::mutex> lock(g_dataset_mtx);
    auto it = g_datasets.find(handle);
    return (it != g_datasets.end()) ? static_cast<int>(it->second->size()) : 0;
}

int ghost_dataset_features(int handle) {
    std::lock_guard<std::mutex> lock(g_dataset_mtx);
    auto it = g_datasets.find(handle);
    return (it != g_datasets.end()) ? static_cast<int>(it->second->numFeatures()) : 0;
}

void ghost_dataset_split(int handle, double ratio, int& out_train, int& out_test) {
    std::lock_guard<std::mutex> lock(g_dataset_mtx);
    auto it = g_datasets.find(handle);
    if (it == g_datasets.end()) {
        out_train = 0;
        out_test = 0;
        return;
    }
    DatasetPtr tr, te;
    it->second->split(ratio, tr, te);
    out_train = g_dataset_counter++;
    g_datasets[out_train] = tr;
    out_test = g_dataset_counter++;
    g_datasets[out_test] = te;
}

std::vector<int> ghost_dataset_split(int handle, double ratio) {
    int tr = 0, te = 0;
    ghost_dataset_split(handle, ratio, tr, te);
    return { tr, te };
}

void ghost_dataset_create_batches(int handle, int bsize) {
    std::lock_guard<std::mutex> lock(g_dataset_mtx);
    auto it = g_datasets.find(handle);
    if (it != g_datasets.end()) {
        it->second->createBatches(bsize);
    }
}

int ghost_dataset_num_batches(int handle) {
    std::lock_guard<std::mutex> lock(g_dataset_mtx);
    auto it = g_datasets.find(handle);
    return (it != g_datasets.end()) ? static_cast<int>(it->second->numBatches()) : 0;
}

double ghost_dataset_get_item(int handle, int row, int col) {
    std::lock_guard<std::mutex> lock(g_dataset_mtx);
    auto it = g_datasets.find(handle);
    return (it != g_datasets.end()) ? it->second->getItem(row, col) : 0.0;
}

} // namespace GhostData
