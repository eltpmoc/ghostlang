#include "GhostTransformer.hpp"
#include "GhostTransformerOps.hpp"
#include "GhostDevice.hpp"
#include "GhostTextModel.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <set>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cstdint>
#ifdef __linux__
#include <unistd.h>
#endif
namespace ghost_transformer {
std::string output_issue(const std::string& text) {
    if (text.find_first_not_of(" \t\r\n")==std::string::npos) return "saida vazia";
    // Long contiguous loops, measured on bytes, without changing emitted UTF-8.
    for (size_t width=1;width<=24;++width) {
        size_t matching=0;
        for(size_t pos=width;pos<text.size();++pos) {
            matching=text[pos]==text[pos-width]?matching+1:0;
            if (matching>=width*5 && matching+width>=24) return "repeticao excessiva";
        }
    }
    std::vector<std::string> words; std::string word;
    for(unsigned char c:text) {
        if(c>=128 || (c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9'))
            word+=char(c>='A'&&c<='Z'?c+'a'-'A':c);
        else if(!word.empty()) { words.push_back(word); word.clear(); }
    }
    if(!word.empty()) words.push_back(word);
    if(words.size()>=20) {
        size_t short_words=0;
        std::set<std::string> distinct(words.begin(),words.end());
        for(const auto& value:words) {
            size_t chars=0; for(unsigned char c:value) if((c&0xc0)!=0x80) ++chars;
            if(chars<=3) ++short_words;
        }
        if(short_words*100>=words.size()*85 && distinct.size()*100<=words.size()*60)
            return "fragmentos curtos e repetitivos";
    }
    return ""; // Passing a structural check is not a correctness judgment.
}
}
namespace tr=ghost_tr;
namespace {
constexpr int VOCAB=259, BOS=256, EOS=257, PAD=258;
void require(bool ok,const std::string& why) { if(!ok) throw std::runtime_error("[Ghost transformer] "+why); }
struct Utf8 {
    int remaining=0,low=128,high=191;
    bool accepts(int b) const {
        if(remaining) return b>=low&&b<=high;
        return b==9||b==10||(b>=32&&b<127)||(b>=194&&b<=244);
    }
    void feed(int b) {
        if(remaining) { --remaining; low=128; high=191; return; }
        if(b<128) return;
        remaining=b<224?1:(b<240?2:3); low=128; high=191;
        if(b==224) low=160; if(b==237) high=159;
        if(b==240) low=144; if(b==244) high=143;
    }
};
struct Tensor {
    bool gpu; int b,r,c;
    float *data=nullptr,*grad=nullptr,*m=nullptr,*v=nullptr;
    size_t size() const { return size_t(b)*r*c; }
    Tensor(bool g,int bb,int rr,int cc,bool parameter=false):gpu(g),b(bb),r(rr),c(cc) {
        try { data=tr::allocate(gpu,size()); grad=tr::allocate(gpu,size());
            if(parameter) { m=tr::allocate(gpu,size()); v=tr::allocate(gpu,size()); }
        } catch(...) { clear(); throw; }
    }
    void clear() { for(float* p:{data,grad,m,v}) if(p) tr::release(gpu,p); }
    ~Tensor() { clear(); }
    std::vector<float> host(float* ptr=nullptr) const {
        std::vector<float> out(size()); tr::download(gpu,out.data(),ptr?ptr:data,size()); return out;
    }
};
enum Op { EMBED, MAT, ADD, NORM, GELU, HEADS, UNHEADS, SOFTMAX };
struct Node {
    Op op; Tensor *a,*b;
    std::unique_ptr<Tensor> out;
    bool tb=false; float scale=1;
};
struct Graph {
    bool gpu; int B,T,D,H; std::vector<Node> nodes;
    Tensor ids,targets,losses;
    Graph(bool g,int b,int t,int d,int h):gpu(g),B(b),T(t),D(d),H(h),
        ids(g,1,b*t,1),targets(g,1,b*t,1),losses(g,1,b*t,1) {}
    Tensor* push(Op op,Tensor* a,Tensor* b,int bb,int r,int c,bool tb=false,float scale=1) {
        nodes.push_back({op,a,b,std::make_unique<Tensor>(gpu,bb,r,c),tb,scale}); return nodes.back().out.get();
    }
    Tensor* mat(Tensor* a,Tensor* b,bool transpose=false,float scale=1) {
        require(a->b==b->b&&a->c==(transpose?b->c:b->r),"Dimensoes internas incompatíveis.");
        return push(MAT,a,b,a->b,a->r,transpose?b->r:b->c,transpose,scale);
    }
    Tensor* unary(Op op,Tensor* a) { return push(op,a,nullptr,a->b,a->r,a->c); }
    Tensor* add(Tensor* a,Tensor* b) { return push(ADD,a,b,a->b,a->r,a->c); }
    void forward() {
        for(auto& node:nodes) {
            auto &o=*node.out,&a=*node.a; auto* b=node.b;
            switch(node.op) {
                case EMBED: tr::embedding(gpu,ids.data,a.data,b->data,o.data,nullptr,nullptr,B*T,T,D,false); break;
                case MAT: tr::gemm(gpu,a.data,b->data,o.data,o.b,o.r,o.c,a.c,false,node.tb,node.scale,false); break;
                case ADD: tr::add(gpu,a.data,b->data,o.data,o.size()); break;
                case NORM: tr::norm(gpu,a.data,nullptr,o.data,a.b*a.r,a.c,false); break;
                case GELU: tr::gelu(gpu,a.data,nullptr,o.data,a.size(),false); break;
                case HEADS: tr::heads(gpu,a.data,o.data,B,T,D,H,false,false); break;
                case UNHEADS: tr::heads(gpu,a.data,o.data,B,T,D,H,true,false); break;
                case SOFTMAX: tr::softmax(gpu,a.data,nullptr,o.data,a.b,T,false); break;
            }
        }
    }
    void backward() {
        for(auto it=nodes.rbegin();it!=nodes.rend();++it) {
            auto& node=*it; auto &o=*node.out,&a=*node.a; auto* b=node.b;
            switch(node.op) {
                case EMBED: tr::embedding(gpu,ids.data,a.data,b->data,o.grad,a.grad,b->grad,B*T,T,D,true); break;
                case MAT:
                    tr::gemm(gpu,o.grad,b->data,a.grad,o.b,o.r,a.c,o.c,false,!node.tb,node.scale,true);
                    if(node.tb) tr::gemm(gpu,o.grad,a.data,b->grad,o.b,o.c,a.c,o.r,true,false,node.scale,true);
                    else tr::gemm(gpu,a.data,o.grad,b->grad,o.b,a.c,o.c,o.r,true,false,node.scale,true);
                    break;
                case ADD: tr::add(gpu,o.grad,nullptr,a.grad,a.size(),true); tr::add(gpu,o.grad,nullptr,b->grad,b->size(),true); break;
                case NORM: tr::norm(gpu,a.data,o.grad,a.grad,a.b*a.r,a.c,true); break;
                case GELU: tr::gelu(gpu,a.data,o.grad,a.grad,a.size(),true); break;
                case HEADS: tr::heads(gpu,o.grad,a.grad,B,T,D,H,true,true); break;
                case UNHEADS: tr::heads(gpu,o.grad,a.grad,B,T,D,H,false,true); break;
                case SOFTMAX: tr::softmax(gpu,o.data,o.grad,a.grad,a.b,T,true); break;
            }
        }
    }
};
struct Example { std::vector<int> tokens; int prompt; };
std::vector<char> read_file(const std::string& path,size_t maximum) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    require(bool(f),"Arquivo inacessivel: "+path);
    auto n=f.tellg(); require(n>=0&&uint64_t(n)<=maximum,"Tamanho de arquivo invalido.");
    std::vector<char> bytes(static_cast<size_t>(n)); f.seekg(0);
    require(bytes.empty()||bool(f.read(bytes.data(),bytes.size())),"Leitura incompleta."); return bytes;
}
struct Reader {
    const std::vector<char>& bytes; size_t pos=0;
    template<class T> T get() { require(pos+sizeof(T)<=bytes.size(),"Arquivo truncado."); T value; std::memcpy(&value,bytes.data()+pos,sizeof(T)); pos+=sizeof(T); return value; }
    std::string text(size_t n) { require(n<=bytes.size()-pos,"Texto truncado."); std::string s(bytes.data()+pos,n); pos+=n; return s; }
};
template<class T> void put(std::vector<char>& bytes,const T& value) {
    auto* p=reinterpret_cast<const char*>(&value); bytes.insert(bytes.end(),p,p+sizeof(T));
}
uint64_t hash(const char* p,size_t n) { uint64_t h=14695981039346656037ULL; for(size_t i=0;i<n;++i) { h^=static_cast<unsigned char>(p[i]); h*=1099511628211ULL; } return h; }
void atomic_write(const std::string& path,const std::vector<char>& data) {
    namespace fs=std::filesystem;
    auto parent=fs::path(path).parent_path(); if(!parent.empty()) fs::create_directories(parent);
#ifdef __linux__
    std::string name=path+".tmp-XXXXXX"; std::vector<char> tmp(name.begin(),name.end()); tmp.push_back(0);
    int fd=mkstemp(tmp.data()); require(fd>=0,"Não foi possível criar checkpoint temporário.");
    try { size_t done=0; while(done<data.size()) { auto n=write(fd,data.data()+done,data.size()-done); if(n<0&&errno==EINTR) continue; require(n>0,"Falha ao gravar checkpoint."); done+=n; }
        require(fsync(fd)==0,"Falha ao sincronizar checkpoint."); close(fd); fd=-1; fs::rename(tmp.data(),path);
    } catch(...) { if(fd>=0) close(fd); fs::remove(tmp.data()); throw; }
#else
    auto tmp=path+".tmp"; std::ofstream f(tmp,std::ios::binary); f.write(data.data(),data.size()); f.close(); require(bool(f),"Falha ao gravar checkpoint."); fs::rename(tmp,path);
#endif
}
}
namespace ghost_transformer {
std::vector<int> encode(const std::string& text) {
    Utf8 state; std::vector<int> ids;
    for(unsigned char c:text) {
        // All valid Unicode is represented as bytes; no unknown token or learned word dictionary.
        require(state.accepts(c),"Entrada deve ser UTF-8 valido, sem controles de terminal.");
        state.feed(c); ids.push_back(c);
    }
    require(state.remaining==0,"Sequencia UTF-8 incompleta."); return ids;
}
std::string decode(const std::vector<int>& tokens) {
    std::string out; for(int t:tokens) { require(t>=0&&t<256,"Token especial nao e texto."); out+=char(t); }
    encode(out); return out;
}
}
struct GhostTransformer::Impl {
    int C,D,H,L,B,seed; bool gpu; uint64_t update=0; std::mt19937 rng;
    std::vector<std::unique_ptr<Tensor>> params;
    std::unique_ptr<Graph> graph;
    std::vector<Example> train,validation;
    Impl(int c,int d,int h,int l,int b,int s,bool g):C(c),D(d),H(h),L(l),B(b),seed(s),gpu(g),rng(s) {
        require(C>=16&&C<=1024&&D>=8&&D<=512&&H>=1&&H<=16&&D%H==0&&L>=1&&L<=16&&B>=1&&B<=32&&s>=0,"Dimensoes invalidas do Transformer.");
        require(uint64_t(B)*H*C*C*L*16<3500000000ULL,"Configuracao excede o limite de buffers de atencao.");
        parameter(VOCAB,D); parameter(C,D);
        for(int layer=0;layer<L;++layer) {
            for(int n=0;n<4;++n) parameter(D,D);
            parameter(D,D*4); parameter(D*4,D);
        }
        parameter(D,VOCAB);
    }
    Tensor* parameter(int r,int c) {
        auto t=std::make_unique<Tensor>(gpu,1,r,c,true);
        std::normal_distribution<float> normal(0,0.02f);
        std::vector<float> w(t->size()); for(auto& x:w) x=normal(rng);
        tr::upload(gpu,t->data,w.data(),w.size()); params.push_back(std::move(t)); return params.back().get();
    }
    Graph& shape(int batch,int time) {
        if(graph&&graph->B==batch&&graph->T==time) return *graph;
        graph=std::make_unique<Graph>(gpu,batch,time,D,H);
        auto& g=*graph;
        auto* x=g.push(EMBED,params[0].get(),params[1].get(),1,batch*time,D);
        int p=2;
        for(int layer=0;layer<L;++layer) {
            auto* n=g.unary(NORM,x);
            auto head=[&](Tensor* w) { auto* flat=g.mat(n,w); return g.push(HEADS,flat,nullptr,batch*H,time,D/H); };
            auto* q=head(params[p++].get()); auto* k=head(params[p++].get()); auto* v=head(params[p++].get());
            auto* scores=g.mat(q,k,true,1.0f/std::sqrt(float(D/H)));
            auto* probs=g.unary(SOFTMAX,scores);
            auto* att=g.mat(probs,v);
            auto* flat=g.push(UNHEADS,att,nullptr,1,batch*time,D);
            x=g.add(x,g.mat(flat,params[p++].get()));
            auto* norm=g.unary(NORM,x);
            auto* up=g.unary(GELU,g.mat(norm,params[p++].get()));
            x=g.add(x,g.mat(up,params[p++].get()));
        }
        g.mat(g.unary(NORM,x),params[p].get());
        return g;
    }
    void clear_grad(Graph& g) {
        for(auto& p:params) tr::zero(gpu,p->grad,p->size());
        for(auto& n:g.nodes) tr::zero(gpu,n.out->grad,n.out->size());
    }
    double objective(Graph& g,const std::vector<float>& ids,const std::vector<float>& targets,bool back) {
        int labeled=std::count_if(targets.begin(),targets.end(),[](float t){return t>=0;});
        require(labeled>0,"Lote sem respostas supervisionadas.");
        tr::upload(gpu,g.ids.data,ids.data(),ids.size()); tr::upload(gpu,g.targets.data,targets.data(),targets.size());
        if(back) clear_grad(g);
        g.forward();
        auto& out=*g.nodes.back().out;
        tr::loss(gpu,out.data,g.targets.data,out.grad,g.losses.data,g.B*g.T,VOCAB,labeled);
        auto losses=g.losses.host(); double value=std::accumulate(losses.begin(),losses.end(),0.0);
        require(std::isfinite(value),"Loss nao finita; treinamento interrompido.");
        if(back) g.backward();
        return value;
    }
    std::vector<Example> dataset(const std::string& path) {
        auto bytes=read_file(path,64*1024*1024); Reader f{bytes};
        require(f.text(8)=="GTRDATA1","Formato de corpus invalido.");
        uint32_t count=f.get<uint32_t>(); require(count>0&&count<=100000,"Quantidade de exemplos invalida.");
        std::vector<Example> records; records.reserve(count);
        for(uint32_t i=0;i<count;++i) {
            auto np=f.get<uint32_t>(),na=f.get<uint32_t>();
            if (np == 0 || na == 0 || uint64_t(np) + na + 1 > uint64_t(C)) {
                f.pos += np + na;
                continue;
            }
            auto prompt=ghost_transformer::encode(f.text(np)),answer=ghost_transformer::encode(f.text(na));
            Example e; e.prompt=prompt.size(); e.tokens.push_back(BOS);
            e.tokens.insert(e.tokens.end(),prompt.begin(),prompt.end()); e.tokens.insert(e.tokens.end(),answer.begin(),answer.end()); e.tokens.push_back(EOS);
            records.push_back(std::move(e));
        }
        require(f.pos==bytes.size(),"Dados extras no corpus."); return records;
    }
    double batch(const std::vector<Example>& data,bool training,int offset=0) {
        auto& g=shape(B,C); std::vector<float> ids(B*C,PAD),targets(B*C,-1);
        std::uniform_int_distribution<size_t> choose(0,data.size()-1);
        for(int b=0;b<B;++b) {
            auto& e=data[training?choose(rng):(offset+b)%data.size()];
            for(size_t i=0;i+1<e.tokens.size();++i) {
                ids[b*C+i]=e.tokens[i]; if(int(i)>=e.prompt) targets[b*C+i]=e.tokens[i+1];
            }
        }
        return objective(g,ids,targets,training);
    }
    std::string generate(const std::string& prompt,int maxbytes,const std::function<void(const std::string&)>& emit={}, std::string* rejection=nullptr, bool* ended=nullptr) {
        require(maxbytes>=1&&maxbytes<=2048,"Limite de geracao 1..2048 bytes.");
        auto bytes=ghost_transformer::encode(prompt); require(!bytes.empty()&&bytes.size()<=65536,"Prompt vazio ou maior que 64 KiB.");
        std::vector<int> tokens{BOS}; tokens.insert(tokens.end(),bytes.begin(),bytes.end());
        if(tokens.size()>size_t(C)) std::cerr<<"[Ghost transformer] Contexto inicial truncado para "<<C<<" tokens de byte.\n";
        if(ended) *ended=false;
        size_t next_check=48;
        std::string answer,pending; Utf8 state;
        for(int step=0;step<maxbytes;++step) {
            if(tokens.size()>size_t(C)) tokens.erase(tokens.begin(),tokens.end()-C);
            int time=std::min(C,int((tokens.size()+31)/32)*32);
            auto& g=shape(1,time); std::vector<float> input(time,PAD);
            std::copy(tokens.begin(),tokens.end(),input.begin()); tr::upload(gpu,g.ids.data,input.data(),input.size());
            g.forward(); auto& output=*g.nodes.back().out;
            std::vector<float> logits(VOCAB);
            tr::download(gpu,logits.data(),output.data+(tokens.size()-1)*VOCAB,VOCAB);
            int best=-1; float score=-INFINITY;
            for(int t=0;t<VOCAB;++t) if((t==EOS&&state.remaining==0)||(t<256&&state.accepts(t))) {
                require(std::isfinite(logits[t]),"Logits nao finitos.");
                if(logits[t]>score) { best=t; score=logits[t]; }
            }
            require(best>=0,"Nenhum byte UTF-8 valido."); if(best==EOS) { if(ended) *ended=true; break; }
            tokens.push_back(best); pending+=char(best); state.feed(best);
            if(!state.remaining) {
                answer+=pending; if(emit) emit(pending); pending.clear();
                if(rejection && answer.size()>=next_check) {
                    next_check=answer.size()+32;
                    *rejection=ghost_transformer::output_issue(answer);
                    if(!rejection->empty()) break;
                }
            }
        }
        return answer;
    }
};
GhostTransformer::GhostTransformer(int c,int d,int h,int l,int b,int s):impl(std::make_unique<Impl>(c,d,h,l,b,s,ghost::backend()=="cuda")) {}
GhostTransformer::~GhostTransformer()=default;
int GhostTransformer::context_size() const { return impl->C; }
GhostTransformer::GhostTransformer(GhostTransformer&&) noexcept=default;
GhostTransformer& GhostTransformer::operator=(GhostTransformer&&) noexcept=default;
void GhostTransformer::prepare(std::string train,std::string validation) {
    auto t=impl->dataset(train),v=impl->dataset(validation);
    impl->train=std::move(t); impl->validation=std::move(v);
}
double GhostTransformer::steps(int count,double rate) {
    require(!impl->train.empty(),"Chame prepare antes do treino."); require(count>=1&&count<=10000&&std::isfinite(rate)&&rate>=1e-6&&rate<=0.01,"Passos ou taxa invalidos.");
    double sum=0;
    for(int step=0;step<count;++step) {
        sum+=impl->batch(impl->train,true); ++impl->update;
        float c1=1-std::pow(0.9,impl->update),c2=1-std::pow(0.999,impl->update);
        for(auto& p:impl->params) tr::adam(impl->gpu,p->data,p->m,p->v,p->grad,p->size(),rate,c1,c2);
    }
    return sum/count;
}
double GhostTransformer::validate(int batches) {
    require(!impl->validation.empty()&&batches>=1&&batches<=10000,"Validacao indisponivel ou limite invalido.");
    int n=std::min(batches,int((impl->validation.size()+impl->B-1)/impl->B)); double sum=0;
    for(int i=0;i<n;++i) sum+=impl->batch(impl->validation,false,i*impl->B); return sum/n;
}
int GhostTransformer::updates() const { return impl->update; }
void GhostTransformer::info() {
    size_t n=0; for(auto& p:impl->params) n+=p->size();
    std::cout<<"Transformer byte nativo | "<<(impl->gpu?"CUDA":"CPU")<<" | contexto="<<impl->C<<" bytes | largura="<<impl->D
        <<" | cabecas="<<impl->H<<" | blocos="<<impl->L<<" | lote="<<impl->B<<" | parametros="<<n<<" | vocabulario=259 (256 bytes + BOS/EOS/PAD) | passos="<<impl->update<<"\n";
}
std::string GhostTransformer::generate(std::string prompt,int maxbytes) { return impl->generate(prompt,maxbytes); }
GhostGenerationResult GhostTransformer::generate_checked(std::string prompt,int maxbytes) {
    GhostGenerationResult result; bool ended=false;
    auto candidate=impl->generate(prompt,maxbytes,{},&result.reason,&ended);
    result.generated_bytes=static_cast<int>(candidate.size());
    if(result.reason.empty()) result.reason=ghost_transformer::output_issue(candidate);
    if(result.reason.empty() && !ended) result.reason="limite de geracao atingido sem concluir a resposta";
    result.accepted=result.reason.empty();
    if(result.accepted) result.text=std::move(candidate);
    return result;
}
std::string GhostTransformer::stream(std::string prompt,int maxbytes,int delay,std::string prefix) {
    if(delay==-1) delay=text_stream_delay(); require(delay>=0&&delay<=100,"Atraso invalido.");
    bool started=false;
    auto result=impl->generate(prompt,maxbytes,[&](const std::string& piece){if(!started) { std::cout<<prefix; started=true; } std::cout<<piece<<std::flush; if(delay) std::this_thread::sleep_for(std::chrono::milliseconds(delay));});
    if(!started) std::cout<<prefix;
    std::cout<<"\n"<<std::flush; return result;
}
void GhostTransformer::save(std::string path) {
    std::vector<char> bytes; const std::string magic="GTRBYTE1"; bytes.insert(bytes.end(),magic.begin(),magic.end());
    for(int x:{impl->C,impl->D,impl->H,impl->L,impl->B,impl->seed}) put(bytes,int32_t(x));
    put(bytes,impl->update); std::ostringstream state; state<<impl->rng; auto rng=state.str(); put(bytes,uint32_t(rng.size())); bytes.insert(bytes.end(),rng.begin(),rng.end());
    put(bytes,uint32_t(impl->params.size()));
    for(auto& p:impl->params) {
        put(bytes,uint32_t(p->size()));
        for(float* ptr:{p->data,p->m,p->v}) {
            auto values=p->host(ptr); for(float x:values) { require(std::isfinite(x),"Checkpoint possui valores nao finitos."); put(bytes,x); }
        }
    }
    put(bytes,hash(bytes.data(),bytes.size())); atomic_write(path,bytes);
}
void GhostTransformer::load(std::string path) {
    auto bytes=read_file(path,128*1024*1024); require(bytes.size()>=64,"Checkpoint truncado.");
    uint64_t checksum; std::memcpy(&checksum,bytes.data()+bytes.size()-8,8); require(checksum==hash(bytes.data(),bytes.size()-8),"Integridade do checkpoint invalida.");
    Reader f{bytes}; require(f.text(8)=="GTRBYTE1","Formato de checkpoint incompativel com Transformer.");
    std::array<int,6> s; for(auto& x:s) x=f.get<int32_t>();
    auto candidate=std::make_unique<Impl>(s[0],s[1],s[2],s[3],s[4],s[5],impl->gpu);
    candidate->update=f.get<uint64_t>(); require(candidate->update<100000000,"Contador de treino invalido.");
    auto nr=f.get<uint32_t>(); require(nr<20000,"Estado aleatorio invalido."); std::istringstream state(f.text(nr)); require(bool(state>>candidate->rng),"RNG invalido.");
    require(f.get<uint32_t>()==candidate->params.size(),"Quantidade de tensores invalida.");
    for(auto& p:candidate->params) {
        require(f.get<uint32_t>()==p->size(),"Dimensao de tensor invalida.");
        for(float* ptr:{p->data,p->m,p->v}) {
            std::vector<float> values(p->size());
            for(auto& x:values) { x=f.get<float>(); require(std::isfinite(x)&&(ptr!=p->v||x>=0),"Parametro ou estado AdamW invalido."); }
            tr::upload(candidate->gpu,ptr,values.data(),values.size());
        }
    }
    require(f.pos+8==bytes.size(),"Checkpoint com dados extras."); impl=std::move(candidate);
}
void GhostTransformer::self_test() {
    auto input=std::string("ação, vocẽ, 🧠, 未知");
    require(ghost_transformer::decode(ghost_transformer::encode(input))==input,"Roundtrip de bytes falhou.");
    Impl cpu(32,8,2,1,2,17,false);
    std::vector<float> ids(16),targets(16,-1); for(int i=0;i<16;++i) { ids[i]=65+i%9; if(i%8>=4) targets[i]=66+i%7; }
    auto& g=cpu.shape(2,8); double initial=cpu.objective(g,ids,targets,true); float worst=0;
    for(auto& p:cpu.params) {
        auto gradients=p->host(p->grad); std::vector<size_t> order(gradients.size()); std::iota(order.begin(),order.end(),0);
        std::partial_sort(order.begin(),order.begin()+std::min<size_t>(2,order.size()),order.end(),[&](size_t a,size_t b){return std::abs(gradients[a])>std::abs(gradients[b]);});
        for(int n=0;n<2;++n) {
            auto i=order[n]; float old=p->data[i],eps=0.001f;
            p->data[i]=old+eps; double plus=cpu.objective(g,ids,targets,false);
            p->data[i]=old-eps; double minus=cpu.objective(g,ids,targets,false); p->data[i]=old;
            worst=std::max(worst,float(std::abs((plus-minus)/(2*eps)-gradients[i])));
        }
    }
    require(worst<0.003,"Gradientes nao conferem com diferencas finitas: "+std::to_string(worst));
    cpu.objective(g,ids,targets,true); auto baseline=g.nodes.back().out->host();
    auto changed=ids; changed[7]=99; cpu.objective(g,changed,targets,false); auto future=g.nodes.back().out->host();
    for(int i=0;i<7*VOCAB;++i) require(baseline[i]==future[i],"Atencao vazou token futuro.");
    if(impl->gpu) {
        Impl cuda(32,8,2,1,2,17,true); auto& gg=cuda.shape(2,8);
        double loss=cuda.objective(gg,ids,targets,true); require(std::abs(loss-initial)<1e-4,"Loss CPU/CUDA divergente.");
        cpu.objective(g,ids,targets,true);
        for(size_t i=0;i<cpu.params.size();++i) {
            auto a=cpu.params[i]->host(cpu.params[i]->grad),b=cuda.params[i]->host(cuda.params[i]->grad);
            for(size_t k=0;k<a.size();++k) require(std::abs(a[k]-b[k])<0.002,"Gradiente CPU/CUDA divergente.");
        }
    }
    for(int step=1;step<=100;++step) {
        cpu.objective(g,ids,targets,true);
        for(auto& p:cpu.params) tr::adam(false,p->data,p->m,p->v,p->grad,p->size(),0.003,1-std::pow(0.9,step),1-std::pow(0.999,step));
    }
    double final_loss=cpu.objective(g,ids,targets,false);
    require(final_loss<initial*0.7,"Teste de aprendizado falhou: "+std::to_string(initial)+" -> "+std::to_string(final_loss));
    std::cout<<"OK: bytes UTF-8, gradientes (erro maximo "<<worst<<"), mascara causal, aprendizado"<<(impl->gpu?", paridade CPU/CUDA":"")<<".\n";
}
