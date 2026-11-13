#pragma once
#include <memory>
#include <string>
#include <vector>
#include "GhostNumberG.hpp"

struct GhostGenerationResultG {
    bool accepted = false;
    std::string text;
    std::string reason;
    int generated_bytes = 0;
};

// Estrutura POD Quântica otimizada para a Placa de Vídeo
// Isso permite transferir Números Fantasmas diretamente para a VRAM (CUDA)
struct GhostQubit {
    float real;
    float imag;
    float theta; // incerteza quântica do GhostNumberG
};

class GhostTransformerG {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    GhostTransformerG(int context=384, int width=192, int heads=6, int layers=4, int batch=4, int seed=42);
    ~GhostTransformerG();
    GhostTransformerG(GhostTransformerG&&) noexcept;
    GhostTransformerG& operator=(GhostTransformerG&&) noexcept;
    
    // Suporte nativo a CSV
    void prepare_csv(std::string csv_path);
    void prepare(std::string train, std::string validation);
    
    double steps(int count, double rate);
    double validate(int batches=32);
    void save(std::string path);
    void load(std::string path);
    std::string generate(std::string prompt, int max_bytes=256);
    GhostGenerationResultG generate_checked(std::string prompt, int max_bytes=256);
    void info();
    void self_test();
    int updates() const;
    int context_size() const;
};

inline GhostTransformerG transformer_model_v5(int context=384,int width=192,int heads=6,int layers=4,int batch=4,int seed=42) {
    return GhostTransformerG(context,width,heads,layers,batch,seed);
}
