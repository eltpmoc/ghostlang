#include "GhostTransformer.hpp"
#include "GhostDevice.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <sstream>
#include <chrono>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
namespace fs=std::filesystem;
void check(bool ok,const char* reason) { if(!ok) throw std::runtime_error(reason); }
template<class T> void write(std::ofstream& f,T n) { f.write(reinterpret_cast<const char*>(&n),sizeof(n)); }
std::string bytes(const fs::path& path) { std::ifstream f(path,std::ios::binary); return {std::istreambuf_iterator<char>(f),{}}; }

static std::string create_temp_folder(const std::string& prefix) {
    auto base = fs::temp_directory_path() / (prefix + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(base);
    return base.string();
}

int main(int argc,char** argv) {
    std::string mode=argc>1?argv[1]:"cpu"; use_device(mode);
    GhostTransformer model(32,8,2,1,2,73); model.self_test();
    check(model.context_size()==32,"context accessor matches model configuration");
    const std::string reported="mo to r de duhojo tu derario toro orororo co dero tu do do de de tro do do tr  to do to tro to      to  o     de  r  r r  do    ro  de  t    de to d  de  to de de r to r do d do  to do d do    do  r r          do        do do       do d       do  do do";
    check(!ghost_transformer::output_issue(reported).empty(),"reported degeneration rejected");
    check(!ghost_transformer::output_issue("abcabcabcabcabcabcabcabcabcabc").empty(),"character loop rejected");
    check(!ghost_transformer::output_issue("   ").empty(),"blank output rejected");
    for(auto valid : {"Olá! Como posso ajudar você?", "42", "G", "Sim.", "x + y = 10", "A teoria dos Números G define uma álgebra própria. Podemos estudar as operações, construir exemplos e testar os resultados em GhostLang.", "O tempo é medido pelo relógio local. A data segue o calendário do sistema. Entre duas mensagens, usamos um contador independente para medir a duração."})
        check(ghost_transformer::output_issue(valid).empty(),"legitimate short, UTF8 and prose answers preserved");
    bool rejected=false;
    try { ghost_transformer::encode(std::string("\xc0\xaf",2)); } catch(...) { rejected=true; }
    check(rejected,"invalid UTF8 must be rejected");
    std::string root = create_temp_folder("ghost-transformer");
    struct Cleanup { fs::path p; ~Cleanup() { std::error_code ec; fs::remove_all(p, ec); } } cleanup{root};
    auto corpus=fs::path(root)/"data.gtd";
    {
        std::ofstream f(corpus,std::ios::binary); f.write("GTRDATA1",8); write(f,uint32_t(2));
        for(auto question:{"abc","def"}) {
            write(f,uint32_t(3)); write(f,uint32_t(2)); f.write(question,3); f.write("OK",2);
        }
    }
    model.prepare(corpus.string(),corpus.string()); model.steps(3,.001);
    auto a=fs::path(root)/"a.bin",b=fs::path(root)/"b.bin",c=fs::path(root)/"c.bin";
    model.save(a.string()); model.steps(4,.001); model.save(b.string());
    GhostTransformer resumed; resumed.load(a.string()); resumed.prepare(corpus.string(),corpus.string()); resumed.steps(4,.001); resumed.save(c.string());
    check(resumed.updates()==7,"optimizer step must resume");
    if(mode=="cpu") check(bytes(b)==bytes(c),"CPU resume must reproduce weights, moments and RNG");
    auto response=resumed.generate("abc",20);
    auto checked=resumed.generate_checked("abc",20);
    check((checked.accepted && checked.text==response && checked.reason.empty()) ||
          (!checked.accepted && checked.text.empty() && !checked.reason.empty()),"checked generation never exposes rejected text");
    check(response==resumed.generate("abc",20),"generation must be deterministic");
    std::ostringstream captured;
    auto* previous=std::cout.rdbuf(captured.rdbuf());
    std::string streamed;
    try { streamed=resumed.stream("abc",20,0,"PREFIX: "); }
    catch(...) { std::cout.rdbuf(previous); throw; }
    std::cout.rdbuf(previous);
    check(streamed==response && captured.str()=="PREFIX: "+response+"\n","stream must preserve generated UTF8 text");
    check(ghost_transformer::decode(ghost_transformer::encode(response))==response,"generated UTF8");
    auto corrupt=bytes(c); corrupt[corrupt.size()/2]^=1;
    { std::ofstream f(c,std::ios::binary); f.write(corrupt.data(),corrupt.size()); }
    rejected=false; try { resumed.load(c.string()); } catch(...) { rejected=true; }
    check(rejected,"corrupt checkpoint must be rejected");
    check(resumed.generate("abc",20)==response,"failed load must preserve current model");
    {
        std::ofstream f(corpus,std::ios::binary); f.write("GTRDATA1",8); write(f,uint32_t(1)); write(f,uint32_t(4000)); write(f,uint32_t(1));
    }
    rejected=false; try { resumed.prepare(corpus.string(),corpus.string()); } catch(...) { rejected=true; }
    check(rejected,"oversized or truncated data must be rejected");
    std::cout<<"OK: checkpoint, retomada, RNG, UTF-8 e dados invalidos ("<<mode<<").\n";
}
