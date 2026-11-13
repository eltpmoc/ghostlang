#include "GhostTextModel.hpp"
#include "GhostDevice.hpp"
#include <stdexcept>
#include "GhostTextLayout.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <functional>
#include <thread>
#include <chrono>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef GHOST_HAS_CUDA
extern "C" void* ghost_text_cuda_create();
extern "C" void ghost_text_cuda_destroy(void*);
extern "C" const char* ghost_text_cuda_error();
extern "C" int ghost_text_cuda_setup(void*,GhostTextLayout, const float*, const float*, const float*);
extern "C" int ghost_text_cuda_step(void*,const int*, const int*, int, int, float, uint64_t, float*, float*, float*);
extern "C" int ghost_text_cuda_download(void*,float*, float*, float*);
#endif

[[noreturn]] void text_error(std::string message) { throw std::runtime_error("[Ghost textmodel] " + message); }
int text_int(std::string value,int low,int high) {
    if (value.empty() || value.size()>10 || value.find_first_not_of("0123456789")!=std::string::npos) text_error("Inteiro invalido: "+value);
    unsigned long long parsed=std::stoull(value);
    if (parsed<static_cast<unsigned>(low) || parsed>static_cast<unsigned>(high)) text_error("Inteiro fora dos limites: "+value);
    return parsed;
}
double text_decimal(std::string value,double low,double high) {
    try { size_t end; double parsed=std::stod(value,&end);
        if (end!=value.size() || !std::isfinite(parsed) || parsed<low || parsed>high) text_error("Decimal fora dos limites: "+value);
        return parsed;
    } catch (...) { text_error("Decimal invalido: "+value); }
    return 0;
}
double text_perplexity(double loss) { return std::exp(std::min(loss,80.0)); }

namespace {
int stream_delay(int requested) {
    int delay=requested==-1?text_stream_delay():requested;
    if (delay<0 || delay>100) text_error("Atraso de streaming deve estar entre 0 e 100 ms.");
    return delay;
}
void emit_text(const std::string& text,int delay) {
    if (!delay) { std::cout<<text<<std::flush; return; }
    // Preserva pontos de codigo UTF-8 completos durante a exibicao.
    for(size_t begin=0;begin<text.size();) {
        size_t end=begin+1;
        while(end<text.size() && (static_cast<unsigned char>(text[end])&0xc0)==0x80) ++end;
        std::cout.write(text.data()+begin,end-begin); std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        begin=end;
    }
}
}
int text_stream_delay() {
    if (const char* env=std::getenv("GHOST_STREAM_MS")) return text_int(env,0,100);
#ifdef _WIN32
    return _isatty(_fileno(stdout))?12:0;
#else
    return isatty(fileno(stdout))?12:0;
#endif
}
void text_stream(std::string text,int delay_ms) {
    emit_text(text,stream_delay(delay_ms));
    std::cout<<'\n'<<std::flush;
}

struct GhostTextModel::Impl {
using string=std::string;
template<class T> using vector=std::vector<T>;
GhostTextLayout config;
int seed;
void* cuda_ctx=nullptr;
Impl(int c,int e,int h,int b,int v,int random_seed): config(),seed(random_seed) {
    if(c<4||c>128||e<4||e>64||h<8||h>256||b<1||b>128||v<8||v>4096||seed<0)
        text_error("Dimensoes invalidas: contexto 4..128, embedding 4..64, oculta 8..256, lote 1..128, vocabulario 8..4096, seed >= 0.");
    config=GhostTextLayout(c,e,h,v,b);
}
~Impl() {
#ifdef GHOST_HAS_CUDA
    ghost_text_cuda_destroy(cuda_ctx);
#endif
}

struct Dataset {
    vector<vector<int>> docs;
    vector<std::pair<int,int>> positions;
    size_t unknown = 0, total = 0;
};
struct Model {
    GhostTextLayout shape;
    vector<string> words;
    std::map<string,int> index;
    vector<float> weights, m, v;
    std::mt19937 rng{42};
    uint64_t step = 0;
    int epochs = 0;
    Dataset train, validation;
    bool ready = false;
} model;
bool gpu = false;
double accuracy = 0;
static constexpr size_t MAX_FILE = 32 * 1024 * 1024;

int env_int(const char* name, int fallback, int low, int high) {
    const char* value = std::getenv(name);
    return value ? text_int(value, low, high) : fallback;
}
double env_double(const char* name, double fallback, double low, double high) {
    const char* value = std::getenv(name);
    return value ? text_decimal(value, low, high) : fallback;
}
string read_text(const string& path, size_t maximum=MAX_FILE) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) text_error("Arquivo inacessivel: " + path);
    auto length = stream.tellg();
    if (length < 0 || static_cast<uint64_t>(length) > maximum) text_error("Arquivo excede o limite: " + path);
    string text(static_cast<size_t>(length), '\0');
    stream.seekg(0);
    if (!text.empty() && !stream.read(text.data(), text.size())) text_error("Leitura incompleta: " + path);
    return text;
}
void utf8_check(const string& text) {
    for (size_t i=0; i<text.size();) {
        uint32_t first=static_cast<unsigned char>(text[i++]), cp=first;
        int continuation=0; uint32_t minimum=0;
        if (first < 128) { if (first == 0) text_error("Texto contem byte nulo."); continue; }
        if (first >= 0xc2 && first <= 0xdf) { continuation=1; cp=first&31; minimum=128; }
        else if (first >= 0xe0 && first <= 0xef) { continuation=2; cp=first&15; minimum=2048; }
        else if (first >= 0xf0 && first <= 0xf4) { continuation=3; cp=first&7; minimum=65536; }
        else text_error("Texto nao e UTF-8 valido.");
        for (int k=0; k<continuation; ++k) {
            if (i>=text.size()) text_error("Sequencia UTF-8 truncada.");
            uint32_t byte=static_cast<unsigned char>(text[i++]);
            if ((byte&192)!=128) text_error("Sequencia UTF-8 invalida.");
            cp=(cp<<6)|(byte&63);
        }
        if (cp<minimum || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff)) text_error("Codigo Unicode invalido.");
    }
}
vector<string> tokenize(const string& text) {
    utf8_check(text);
    vector<string> result; string current;
    auto flush=[&] { if (!current.empty()) { result.push_back(current); current.clear(); } };
    const string punctuation=".,!?;:()[]{}\"/\\=+-*<>";
    for (unsigned char byte : text) {
        if (byte=='\r') continue;
        if (byte=='\n') { flush(); result.emplace_back("\n"); }
        else if (byte==' ' || byte=='\t') flush();
        else if (byte<128 && punctuation.find(static_cast<char>(byte))!=string::npos) {
            flush(); result.emplace_back(1,static_cast<char>(byte));
        } else current+=static_cast<char>(byte);
    }
    flush();
    if (result.size()>4000000) text_error("Corpus excede 4 milhoes de tokens.");
    for (const auto& word:result) if (word.size()>256) text_error("Token muito longo (limite 256 bytes).");
    return result;
}
vector<string> documents(const string& raw) {
    string text;
    for (char ch : raw) if (ch!='\r') text+=ch;
    vector<string> docs; size_t start=0;
    while (start<text.size()) {
        size_t end=text.find("\n\n",start);
        if (end==string::npos) end=text.size();
        string piece=text.substr(start,end-start);
        if (piece.find_first_not_of(" \t\n")!=string::npos) docs.push_back(piece);
        start=end+2;
    }
    if (docs.empty()) text_error("Corpus vazio.");
    return docs;
}
void make_index() {
    model.index.clear();
    for (size_t i=0;i<model.words.size();++i) model.index[model.words[i]]=static_cast<int>(i);
}
Dataset dataset(const vector<string>& texts) {
    Dataset result;
    for (const auto& text:texts) {
        vector<int> ids;
        auto tokens=tokenize(text);
        bool dialogue=false;
        for (size_t i=0;i+1<tokens.size();++i)
            if (tokens[i]=="Assistente" && tokens[i+1]==":") dialogue=true;
        for (const auto& token:tokens) {
            auto found=model.index.find(token);
            int id=found==model.index.end()?0:found->second;
            ids.push_back(id); result.unknown+=(id==0); ++result.total;
        }
        if (ids.empty()) continue;
        ids.push_back(2);
        int doc=result.docs.size();
        bool supervise=!dialogue;
        for (size_t p=0;p<ids.size();++p) {
            if (dialogue && p+1<tokens.size() && tokens[p+1]==":") {
                if (tokens[p]=="Usuário") { supervise=false; continue; }
                if (tokens[p]=="Assistente") { supervise=true; continue; }
            }
            if (dialogue && p>0 && p<tokens.size() && tokens[p]==":" &&
                (tokens[p-1]=="Usuário" || tokens[p-1]=="Assistente")) continue;
            if (supervise) result.positions.emplace_back(doc,p);
        }
        result.docs.push_back(std::move(ids));
    }
    if (result.positions.size()<8) text_error("Corpus precisa ter pelo menos 8 tokens.");
    if (result.positions.size()>4000000) text_error("Corpus excede o limite de tokens.");
    return result;
}
void batch_for(const Dataset& data, const vector<size_t>& order, size_t start, int size,
               vector<int>& input, vector<int>& targets) {
    int context=model.shape.context;
    input.assign(size*context,1); targets.resize(size);
    for (int b=0;b<size;++b) {
        auto [doc,pos]=data.positions[order[start+b]];
        targets[b]=data.docs[doc][pos];
        for (int c=0;c<context;++c) {
            int source=pos-context+c;
            if (source>=0) input[b*context+c]=data.docs[doc][source];
        }
    }
}

// Referencia CPU: embeddings concatenados -> tanh -> logits -> softmax/CE.
float cpu_batch(const GhostTextLayout& s, const vector<float>& w, const int* ids, const int* targets,
                int batch, vector<float>& probabilities, vector<float>* gradient) {
    int K=s.width,H=s.hidden,V=s.vocab,E=s.embedding,C=s.context;
    vector<float> x(batch*K), h(batch*H), dh(batch*H,0), dx(batch*K,0);
    probabilities.assign(batch*V,0);
    if (gradient) gradient->assign(s.count,0);
    for (int b=0;b<batch;++b) {
        for (int c=0;c<C;++c) for (int e=0;e<E;++e) x[b*K+c*E+e]=w[ids[b*C+c]*E+e];
        for (int r=0;r<H;++r) {
            float sum=w[s.b1+r];
            for (int k=0;k<K;++k) sum+=w[s.w1+r*K+k]*x[b*K+k];
            h[b*H+r]=std::tanh(sum);
        }
        float maximum=-std::numeric_limits<float>::infinity();
        for (int r=0;r<V;++r) {
            float sum=w[s.b2+r];
            for (int k=0;k<H;++k) sum+=w[s.w2+r*H+k]*h[b*H+k];
            probabilities[b*V+r]=sum; maximum=std::max(maximum,sum);
        }
        float total=0;
        for (int r=0;r<V;++r) { probabilities[b*V+r]=std::exp(probabilities[b*V+r]-maximum); total+=probabilities[b*V+r]; }
        for (int r=0;r<V;++r) probabilities[b*V+r]/=total;
    }
    if (!targets) return 0;
    double loss=0;
    for (int b=0;b<batch;++b) {
        loss-=std::log(std::max(probabilities[b*V+targets[b]],1e-30f));
        if (!gradient) continue;
        for (int r=0;r<V;++r) {
            float delta=(probabilities[b*V+r]-(r==targets[b]?1.0f:0.0f))/batch;
            (*gradient)[s.b2+r]+=delta;
            for (int k=0;k<H;++k) {
                (*gradient)[s.w2+r*H+k]+=delta*h[b*H+k];
                dh[b*H+k]+=w[s.w2+r*H+k]*delta;
            }
        }
        for (int r=0;r<H;++r) {
            float delta=dh[b*H+r]*(1-h[b*H+r]*h[b*H+r]);
            (*gradient)[s.b1+r]+=delta;
            for (int k=0;k<K;++k) {
                (*gradient)[s.w1+r*K+k]+=delta*x[b*K+k];
                dx[b*K+k]+=w[s.w1+r*K+k]*delta;
            }
        }
        for (int c=0;c<C;++c) for (int e=0;e<E;++e) (*gradient)[ids[b*C+c]*E+e]+=dx[b*K+c*E+e];
    }
    return loss/batch;
}
void adam(vector<float>& w, vector<float>& m, vector<float>& v, const vector<float>& grad,
          uint64_t step, float rate) {
    double norm=0;
    for (float g:grad) norm+=double(g)*g;
    if (!std::isfinite(norm)) text_error("Gradiente nao finito.");
    float scale=static_cast<float>(1.0/std::max(1.0,std::sqrt(norm)));
    float c1=1-std::pow(0.9,step),c2=1-std::pow(0.999,step);
    for (size_t i=0;i<w.size();++i) {
        float g=grad[i]*scale;
        m[i]=0.9f*m[i]+0.1f*g; v[i]=0.999f*v[i]+0.001f*g*g;
        w[i]-=rate*(m[i]/c1/(std::sqrt(v[i]/c2)+1e-8f)+0.0001f*w[i]);
    }
}
void upload() {
#ifdef GHOST_HAS_CUDA
    if (gpu && !ghost_text_cuda_setup(cuda_ctx,model.shape,model.weights.data(),model.m.data(),model.v.data()))
        text_error(ghost_text_cuda_error());
#endif
}
void download() {
#ifdef GHOST_HAS_CUDA
    if (gpu && model.ready && !ghost_text_cuda_download(cuda_ctx,model.weights.data(),model.m.data(),model.v.data())) text_error(ghost_text_cuda_error());
#endif
}
float operate(const vector<int>& input,const vector<int>& targets,int batch,bool train,float rate,
              vector<float>& probabilities) {
    float loss=0;
    if (train) ++model.step;
#ifdef GHOST_HAS_CUDA
    if (gpu) {
        probabilities.resize(batch*model.shape.vocab);
        if (!ghost_text_cuda_step(cuda_ctx,input.data(),targets.empty()?nullptr:targets.data(),batch,train,rate,model.step,
                          probabilities.data(),&loss,nullptr)) text_error(ghost_text_cuda_error());
    } else
#endif
    {
        vector<float> gradient;
        loss=cpu_batch(model.shape,model.weights,input.data(),targets.empty()?nullptr:targets.data(),batch,
                       probabilities,train?&gradient:nullptr);
        if (train) adam(model.weights,model.m,model.v,gradient,model.step,rate);
    }
    if (!std::isfinite(loss)) text_error("Loss nao finita; treino interrompido.");
    return loss;
}
double evaluate(const Dataset& data) {
    if (data.positions.empty()) text_error("Nenhum alvo de avaliacao carregado.");
    vector<size_t> order(data.positions.size()); std::iota(order.begin(),order.end(),0);
    vector<int> inputs,targets; vector<float> probabilities;
    double total=0; size_t correct=0;
    for (size_t start=0;start<order.size();start+=model.shape.batch) {
        int size=std::min<size_t>(model.shape.batch,order.size()-start);
        batch_for(data,order,start,size,inputs,targets);
        total+=operate(inputs,targets,size,false,0,probabilities)*size;
        for (int b=0;b<size;++b) {
            auto first=probabilities.begin()+b*model.shape.vocab;
            int predicted=std::max_element(first,first+model.shape.vocab)-first;
            correct+=(predicted==targets[b]);
        }
    }
    accuracy=double(correct)/order.size();
    return total/order.size();
}
uint64_t checksum(const string& data) {
    uint64_t hash=14695981039346656037ull;
    for (unsigned char byte:data) { hash^=byte; hash*=1099511628211ull; }
    return hash;
}
template<class T> void append(string& data,T value) { data.append(reinterpret_cast<const char*>(&value),sizeof(value)); }
struct Reader {
    const string& data; size_t offset=0;
    template<class T> T get() {
        if (offset+sizeof(T)>data.size()) text_error("Checkpoint truncado.");
        T value; std::memcpy(&value,data.data()+offset,sizeof(value)); offset+=sizeof(value); return value;
    }
    string text(size_t n) {
        if (n>data.size()-offset) text_error("Checkpoint truncado.");
        string result=data.substr(offset,n); offset+=n; return result;
    }
};
void require_ready() { if (!model.ready) text_error("Carregue ou inicialize um modelo primeiro."); }
string render_piece(const string& word,const string& previous,const string& output) {
    bool closing=word.size()==1 && string(".,!?;:)]}").find(word[0])!=string::npos;
    bool opening=previous.size()==1 && string("([{").find(previous[0])!=string::npos;
    bool space=!output.empty() && output.back()!='\n' && word!="\n" && !closing && !opening;
    return (space?" ":"")+word;
}

void initialize_device() {
    gpu=ghost::backend()=="cuda";
#ifdef GHOST_HAS_CUDA
    if (gpu && !cuda_ctx) cuda_ctx=ghost_text_cuda_create();
#endif
}
void prepare(std::string train_path,std::string val_path,std::string origin) {
    initialize_device();
    auto train_docs=documents(read_text(train_path)), val_docs=documents(read_text(val_path));
    std::set<vector<string>> training;
    for (const auto& doc:train_docs) training.insert(tokenize(doc));
    for (const auto& doc:val_docs) if (training.count(tokenize(doc))) text_error("Documento de validacao duplicado no treino.");
    if (!origin.empty()) load(origin);
    else {
        std::map<string,int> frequency;
        for (const auto& doc:train_docs) for (const auto& word:tokenize(doc)) ++frequency[word];
        vector<std::pair<string,int>> words(frequency.begin(),frequency.end());
        std::sort(words.begin(),words.end(),[](const auto& a,const auto& b){return a.second!=b.second?a.second>b.second:a.first<b.first;});
        int maximum=config.vocab;
        model.words={"<UNK>","<BOS>","<EOS>"};
        for (const auto& pair:words) {
            if (model.words.size()>=static_cast<size_t>(maximum)) break;
            if (pair.first!="<UNK>" && pair.first!="<BOS>" && pair.first!="<EOS>") model.words.push_back(pair.first);
        }
        if (model.words.size()<8) text_error("Corpus precisa de pelo menos 5 tokens distintos.");
        model.shape=GhostTextLayout(config.context,config.embedding,config.hidden,model.words.size(),config.batch);
        model.rng.seed(seed);
        model.weights.assign(model.shape.count,0); model.m.assign(model.shape.count,0); model.v.assign(model.shape.count,0);
        auto fill=[&](int begin,int end,float limit) {
            std::uniform_real_distribution<float> distribution(-limit,limit);
            for (int i=begin;i<end;++i) model.weights[i]=distribution(model.rng);
        };
        fill(0,model.shape.w1,0.15f);
        fill(model.shape.w1,model.shape.b1,std::sqrt(6.0f/(model.shape.width+model.shape.hidden)));
        fill(model.shape.w2,model.shape.b2,std::sqrt(6.0f/(model.shape.hidden+model.shape.vocab)));
        model.step=0; model.epochs=0; model.ready=true; make_index(); upload();
    }
    model.train=dataset(train_docs); model.validation=dataset(val_docs);
    std::cout<<"Treino: "<<model.train.positions.size()<<" alvos supervisionados; validacao: "<<model.validation.positions.size()
             <<"; desconhecidos validacao: "<<100.0*model.validation.unknown/model.validation.total<<"%\n";
    if (model.validation.unknown>model.validation.total/4) std::cerr<<"[Ghost textmodel] Muitos tokens de validacao desconhecidos; amplie o corpus/vocabulario.\n";
    info();
}
double epoch(double rate) {
    require_ready();
    if (!std::isfinite(rate) || rate<=0 || rate>1) text_error("Taxa de aprendizado deve estar em (0,1].");
    if (model.train.positions.empty()) text_error("Nenhum corpus de treino carregado.");
    vector<size_t> order(model.train.positions.size()); std::iota(order.begin(),order.end(),0);
    std::shuffle(order.begin(),order.end(),model.rng);
    vector<int> input,targets; vector<float> probabilities; double total=0;
    for (size_t start=0;start<order.size();start+=model.shape.batch) {
        int size=std::min<size_t>(model.shape.batch,order.size()-start);
        batch_for(model.train,order,start,size,input,targets);
        total+=operate(input,targets,size,true,rate,probabilities)*size;
    }
    ++model.epochs;
    return total/order.size();
}
double validate() { require_ready(); return evaluate(model.validation); }
double get_accuracy() { return accuracy; }
int epochs() { return model.epochs; }
int patience() { return env_int("GHOST_TEXT_PATIENCE",8,1,200); }
void info() {
    if (!model.ready) { std::cout<<"Ghost textmodel: embeddings, contexto configuravel, tanh, softmax, AdamW.\n"; return; }
    auto& s=model.shape;
    std::cout<<"textmodel: "<<s.count<<" parametros; contexto="<<s.context<<" tokens; embeddings="<<s.embedding
             <<"; oculta="<<s.hidden<<"; vocabulario="<<s.vocab<<"; lote="<<s.batch<<"; epocas="<<model.epochs<<'\n';
}
void save(std::string path) {
    require_ready(); download();
    string data="GHOSTV2\n";
    append<uint32_t>(data,0x01020304); // Identifica endianness; floats IEEE-754 float32.
    for (int n:{model.shape.context,model.shape.embedding,model.shape.hidden,model.shape.vocab,model.shape.batch}) append<uint32_t>(data,n);
    append<uint64_t>(data,model.step); append<uint32_t>(data,model.epochs);
    for (const auto& word:model.words) { append<uint32_t>(data,word.size()); data+=word; }
    std::ostringstream rng; rng<<model.rng; string state=rng.str(); append<uint32_t>(data,state.size()); data+=state;
    for (const auto* array:{&model.weights,&model.m,&model.v}) for (float f:*array) {
        if (!std::isfinite(f)) text_error("Checkpoint recusado: parametro nao finito.");
        append<float>(data,f);
    }
    append<uint64_t>(data,checksum(data));
    std::ofstream stream(path+".tmp",std::ios::binary|std::ios::trunc);
    stream.write(data.data(),data.size()); stream.close();
    if (!stream || read_text(path+".tmp",128*1024*1024)!=data) text_error("Falha ao gravar checkpoint: "+path);
    if (std::rename((path+".tmp").c_str(),path.c_str())!=0) text_error("Falha na troca atomica do checkpoint.");
}
void load(std::string path) {
    initialize_device();
    model.ready=false;
    model.train=Dataset{}; model.validation=Dataset{};
    string bytes=read_text(path,128*1024*1024);
    if (bytes.size()<64 || bytes.substr(0,8)!="GHOSTV2\n") text_error("Checkpoint de texto invalido (formato GHOSTV2).");
    uint64_t saved; std::memcpy(&saved,bytes.data()+bytes.size()-8,8); bytes.resize(bytes.size()-8);
    if (saved!=checksum(bytes)) text_error("Checksum do checkpoint invalido.");
    Reader reader{bytes,8};
    if (reader.get<uint32_t>()!=0x01020304) text_error("Endianness incompativel.");
    int c=reader.get<uint32_t>(),e=reader.get<uint32_t>(),h=reader.get<uint32_t>(),v=reader.get<uint32_t>(),b=reader.get<uint32_t>();
    if (c<4||c>128||e<4||e>64||h<8||h>256||v<8||v>4096||b<1||b>128) text_error("Dimensoes de checkpoint invalidas.");
    model.shape=GhostTextLayout(c,e,h,v,b); model.step=reader.get<uint64_t>(); model.epochs=reader.get<uint32_t>();
    if (model.step>1000000000 || model.epochs<0 || model.epochs>1000000) text_error("Estado de treino invalido.");
    model.words.clear();
    for (int i=0;i<v;++i) {
        uint32_t length=reader.get<uint32_t>(); if (length==0||length>256) text_error("Token de checkpoint invalido.");
        string word=reader.text(length); utf8_check(word); model.words.push_back(word);
    }
    if (model.words[0]!="<UNK>"||model.words[1]!="<BOS>"||model.words[2]!="<EOS>"||std::set<string>(model.words.begin(),model.words.end()).size()!=model.words.size())
        text_error("Vocabulario de checkpoint invalido.");
    uint32_t length=reader.get<uint32_t>(); if (length>20000) text_error("Estado aleatorio invalido.");
    std::istringstream rng(reader.text(length)); if (!(rng>>model.rng)) text_error("Estado aleatorio invalido.");
    for (auto* array:{&model.weights,&model.m,&model.v}) {
        array->resize(model.shape.count);
        for (float& f:*array) { f=reader.get<float>(); if (!std::isfinite(f)) text_error("Parametro nao finito."); }
    }
    for (float f:model.v) if (f<0) text_error("Variancia Adam negativa.");
    if (reader.offset!=bytes.size()) text_error("Tamanho de checkpoint incorreto.");
    model.ready=true; make_index(); upload();
}
double evaluate_file(std::string path) { require_ready(); return evaluate(dataset(documents(read_text(path)))); }

std::string generate(std::string prompt,int count,const std::function<void(const string&)>& emit={}) {
    require_ready();
    if (prompt.size()>16000 || count<1 || count>2048) text_error("Limite: prompt 16000 bytes; geracao 1..2048 tokens.");
    float temperature=env_double("GHOST_TEXT_TEMP",0.65,0,2);
    int topk=env_int("GHOST_TEXT_TOPK",20,1,4096);
    int no_repeat=env_int("GHOST_TEXT_NO_REPEAT",3,0,8);
    double topp=env_double("GHOST_TEXT_TOPP",0.9,0.01,1), penalty=env_double("GHOST_TEXT_REPETITION",1.15,1,3);
    std::mt19937 random(env_int("GHOST_TEXT_GENERATION_SEED",42,0,2147483647));
    vector<int> history, generated,targets;
    string output,previous;
    for (const auto& word:tokenize(prompt)) { auto found=model.index.find(word); history.push_back(found==model.index.end()?0:found->second); }
    int unknown=std::count(history.begin(),history.end(),0);
    if (unknown) std::cerr<<"[Ghost textmodel] Prompt contem "<<unknown<<" tokens fora do vocabulario.\n";
    for (int n=0;n<count;++n) {
        vector<int> input(model.shape.context,1);
        int available=std::min<int>(history.size(),model.shape.context);
        std::copy(history.end()-available,history.end(),input.end()-available);
        vector<float> probabilities; operate(input,targets,1,false,0,probabilities);
        vector<std::pair<double,int>> candidates;
        std::set<int> recent(generated.end()-std::min<size_t>(generated.size(),32),generated.end());
        std::set<int> blocked;
        if (no_repeat>0 && generated.size()>=static_cast<size_t>(no_repeat-1)) {
            size_t prefix=no_repeat-1;
            for (size_t start=0;start+prefix<generated.size();++start) {
                if (std::equal(generated.end()-prefix,generated.end(),generated.begin()+start)) blocked.insert(generated[start+prefix]);
            }
        }
        for (int i=2;i<model.shape.vocab;++i) {
            if (i!=2 && blocked.count(i)) continue;
            if (model.words[i]=="Usuário") continue;
            double probability=probabilities[i];
            if (recent.count(i)) probability/=penalty;
            candidates.emplace_back(std::log(std::max(probability,1e-30)),i);
        }
        std::sort(candidates.begin(),candidates.end(),[](auto a,auto b){return a.first!=b.first?a.first>b.first:a.second<b.second;});
        int selected=candidates[0].second;
        if (temperature>0) {
            candidates.resize(std::min<size_t>(topk,candidates.size()));
            vector<double> weights; double sum=0,maximum=candidates[0].first;
            for (auto item:candidates) { double value=std::exp((item.first-maximum)/temperature); weights.push_back(value); sum+=value; }
            double accumulated=0; size_t keep=0;
            do { accumulated+=weights[keep++]/sum; } while (keep<weights.size() && accumulated<topp);
            weights.resize(keep); std::discrete_distribution<int> distribution(weights.begin(),weights.end());
            selected=candidates[distribution(random)].second;
        }
        if (selected==2) break;
        // Impede que a resposta comece a simular uma nova fala do usuario.
        if (model.words[selected]=="Usuário" && !generated.empty()) break;
        history.push_back(selected); generated.push_back(selected);
        const string& word=model.words[selected];
        string piece=render_piece(word,previous,output);
        output+=piece; previous=word;
        if (emit) emit(piece); // Antes de prever o proximo token.

    }
    return output;
}
void chat() {
    require_ready();
    std::cout<<"Chat experimental local. /sair encerra; /limpar reinicia o contexto.\n";
    string history,line;
    bool remember=env_int("GHOST_TEXT_HISTORY",0,0,1);
    while (std::cout<<"Voce> " && std::getline(std::cin,line)) {
        if (line=="/sair") break;
        if (line=="/limpar") { history.clear(); continue; }
        if (line.empty()) continue;
        if (line.size()>4000) { std::cout<<"Mensagem longa demais (4000 bytes).\n"; continue; }
        if (!remember) history.clear();
        string prompt=history+"Usuário: "+line+"\nAssistente:";
        string answer=generate(prompt,160);
        std::cout<<"IA> "<<(answer.empty()?"[fim de resposta]":answer)<<'\n';
        history=prompt+" "+answer+"\n";
        if (history.size()>8000) history.clear();
    }
}

void self_test() {
    initialize_device();
    GhostTextLayout s(4,4,8,9,2); std::mt19937 rng(19); std::uniform_real_distribution<float> dist(-0.1f,0.1f);
    vector<float> w(s.count),m(s.count,0),v(s.count,0),p,g;
    for (auto& f:w) f=dist(rng);
    vector<int> input={1,3,3,4,1,4,5,6},target={5,7};
    float loss=cpu_batch(s,w,input.data(),target.data(),2,p,&g);
    for (int index:{0,12,13,s.w1,s.w1+12,s.b1,s.w2,s.b2,s.count-1}) {
        float old=w[index],eps=0.01f;
        w[index]=old+eps; float positive=cpu_batch(s,w,input.data(),target.data(),2,p,nullptr);
        w[index]=old-eps; float negative=cpu_batch(s,w,input.data(),target.data(),2,p,nullptr); w[index]=old;
        float numeric=(positive-negative)/(2*eps);
        if (std::abs(numeric-g[index])>0.0001f) text_error("Gradient check falhou no parametro "+std::to_string(index));
    }
#ifdef GHOST_HAS_CUDA
    if (gpu) {
        download();
        struct Restore { Impl* self; ~Restore() { if (self->model.ready) { try { self->upload(); } catch (...) { self->model.ready=false; } } } } restore{this};
        if (!ghost_text_cuda_setup(cuda_ctx,s,w.data(),m.data(),v.data())) text_error(ghost_text_cuda_error());
        vector<float> gp(18),gg(s.count),gw(s.count),gm(s.count),gv(s.count); float gl;
        if (!ghost_text_cuda_step(cuda_ctx,input.data(),target.data(),2,1,0.001f,1,gp.data(),&gl,gg.data())) text_error(ghost_text_cuda_error());
        if (std::abs(gl-loss)>1e-5f) text_error("Loss CPU/CUDA divergiu.");
        for (int i=0;i<s.count;++i) if (std::abs(gg[i]-g[i])>1e-5f) text_error("Gradiente CPU/CUDA divergiu.");
        adam(w,m,v,g,1,0.001f);
        if (!ghost_text_cuda_download(cuda_ctx,gw.data(),gm.data(),gv.data())) text_error(ghost_text_cuda_error());
        for (int i=0;i<s.count;++i) if (std::abs(gw[i]-w[i])>1e-5f) text_error("AdamW CPU/CUDA divergiu.");
    }
#endif
    std::cout<<"OK: gradientes por diferencas finitas"<<(gpu?", loss/gradientes/AdamW CPU-CUDA":"")<<".\n";
}

};
GhostTextModel::GhostTextModel(int c,int e,int h,int b,int v,int seed):impl(std::make_unique<Impl>(c,e,h,b,v,seed)) {}
GhostTextModel::~GhostTextModel()=default;
GhostTextModel::GhostTextModel(GhostTextModel&&) noexcept=default;
GhostTextModel& GhostTextModel::operator=(GhostTextModel&&) noexcept=default;
void GhostTextModel::prepare(std::string a,std::string b,std::string c) { impl->prepare(a,b,c); }
double GhostTextModel::epoch(double rate) { return impl->epoch(rate); }
double GhostTextModel::validate() { return impl->validate(); }
double GhostTextModel::accuracy() const { return impl->accuracy; }
int GhostTextModel::epochs() const { return impl->model.epochs; }
void GhostTextModel::save(std::string p) { impl->save(p); }
void GhostTextModel::load(std::string p) { impl->load(p); }
double GhostTextModel::evaluate(std::string p) { return impl->evaluate_file(p); }
std::string GhostTextModel::generate(std::string p,int n) { return impl->generate(p,n); }
std::string GhostTextModel::stream(std::string prompt,int count,int delay_ms,std::string prefix) {
    int delay=stream_delay(delay_ms);
    bool started=false;
    auto emit=[&](const std::string& piece) {
        if (!started) { std::cout<<prefix<<std::flush; started=true; }
        emit_text(piece,delay);
    };
    try {
        // Validacoes e avisos de vocabulario precedem a emissao do prefixo.
        std::string result=impl->generate(prompt,count,emit);
        if (!started) std::cout<<prefix<<"[fim de resposta]";
        std::cout<<'\n'<<std::flush;
        return result;
    } catch (...) {
        if (started) std::cout<<'\n'<<std::flush;
        throw;
    }
}
void GhostTextModel::chat() { impl->chat(); }
void GhostTextModel::info() { impl->info(); }
void GhostTextModel::self_test() { Impl probe(4,4,8,2,9,19); probe.self_test(); }
std::string GhostTextModel::backend() const { return impl->gpu ? "cuda" : "cpu"; }
