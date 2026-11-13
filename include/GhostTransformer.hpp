#pragma once
#include <memory>
#include <string>
#include <vector>
struct GhostGenerationResult {
    bool accepted = false;
    std::string text;
    std::string reason;
    int generated_bytes = 0;
};
class GhostTransformer {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    GhostTransformer(int context=384, int width=96, int heads=4, int layers=2, int batch=8, int seed=42);
    ~GhostTransformer();
    GhostTransformer(GhostTransformer&&) noexcept;
    GhostTransformer& operator=(GhostTransformer&&) noexcept;
    void prepare(std::string train, std::string validation);
    double steps(int count, double rate);
    double validate(int batches=32);
    void save(std::string path);
    void load(std::string path);
    std::string generate(std::string prompt, int max_bytes=256);
    // Buffered, structural guard for chat; raw generation remains available for evaluation.
    GhostGenerationResult generate_checked(std::string prompt, int max_bytes=256);
    std::string stream(std::string prompt, int max_bytes=256, int delay_ms=-1, std::string prefix="");
    void info();
    void self_test();
    int updates() const;
    int context_size() const;
};
inline GhostTransformer transformer_model(int context=384,int width=96,int heads=4,int layers=2,int batch=8,int seed=42) {
    return GhostTransformer(context,width,heads,layers,batch,seed);
}
namespace ghost_transformer {
std::string output_issue(const std::string& text);
std::vector<int> encode(const std::string& text);
std::string decode(const std::vector<int>& tokens);
}
