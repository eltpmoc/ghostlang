#ifndef GHOST_DATASET_HPP
#define GHOST_DATASET_HPP

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <iostream>

namespace GhostData {

struct DatasetBatch {
    size_t size = 0;
    std::vector<std::vector<double>> features;
    std::vector<double> targets;
};

class GhostDataset {
public:
    std::vector<std::string> headers;
    std::vector<std::vector<double>> data;
    std::vector<double> targets;
    size_t batch_size = 32;
    std::vector<DatasetBatch> batches;

    GhostDataset() = default;

    static std::shared_ptr<GhostDataset> loadCSV(const std::string& path, bool has_header = true);
    static std::shared_ptr<GhostDataset> fromMatrix(size_t rows, size_t cols, const std::vector<double>& vals, bool last_col_is_target = true);

    size_t size() const { return data.size(); }
    size_t numFeatures() const { return data.empty() ? 0 : data[0].size(); }

    void split(double train_ratio, std::shared_ptr<GhostDataset>& train_set, std::shared_ptr<GhostDataset>& test_set, int seed = 42);
    void createBatches(size_t bsize);
    size_t numBatches() const { return batches.size(); }
    const DatasetBatch& getBatch(size_t idx) const;
    double getItem(size_t row, size_t col) const;
};

using DatasetPtr = std::shared_ptr<GhostDataset>;

// Handles para chamadas do interpretador e C++
int ghost_dataset_create_csv(const std::string& path, bool has_header);
int ghost_dataset_create_matrix(int rows, int cols, const std::vector<double>& vals);
int ghost_dataset_size(int handle);
int ghost_dataset_features(int handle);
void ghost_dataset_split(int handle, double ratio, int& out_train, int& out_test);
std::vector<int> ghost_dataset_split(int handle, double ratio);
void ghost_dataset_create_batches(int handle, int bsize);
int ghost_dataset_num_batches(int handle);
double ghost_dataset_get_item(int handle, int row, int col);

} // namespace GhostData

#endif // GHOST_DATASET_HPP
