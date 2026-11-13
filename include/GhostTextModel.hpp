#pragma once
#include <memory>
#include <string>

// Modelo de linguagem nativo. Cada instancia possui parametros, RNG e buffers proprios.
class GhostTextModel {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    GhostTextModel(int context=64,int embedding=32,int hidden=128,int batch=32,int vocab=2048,int seed=42);
    ~GhostTextModel();
    GhostTextModel(GhostTextModel&&) noexcept;
    GhostTextModel& operator=(GhostTextModel&&) noexcept;
    GhostTextModel(const GhostTextModel&)=delete;
    GhostTextModel& operator=(const GhostTextModel&)=delete;
    void prepare(std::string train,std::string validation,std::string checkpoint="");
    double epoch(double rate);
    double validate();
    double accuracy() const;
    int epochs() const;
    void save(std::string path);
    void load(std::string path);
    double evaluate(std::string corpus);
    std::string generate(std::string prompt,int max_tokens=100);
    // Emite cada trecho assim que previsto; retorna o mesmo texto de generate.
    std::string stream(std::string prompt,int max_tokens=100,int delay_ms=-1,std::string prefix="");
    void chat();
    void info();
    void self_test();
    std::string backend() const;
};
inline GhostTextModel text_model(int context=64,int embedding=32,int hidden=128,int batch=32,int vocab=2048,int seed=42) {
    return GhostTextModel(context,embedding,hidden,batch,vocab,seed);
}
int text_int(std::string value,int low,int high);
double text_decimal(std::string value,double low,double high);
double text_perplexity(double loss);
[[noreturn]] void text_error(std::string message);

// -1 usa GHOST_STREAM_MS ou 12 ms em terminal/0 em pipe; limite de 100 ms.
int text_stream_delay();
void text_stream(std::string text,int delay_ms=-1);
