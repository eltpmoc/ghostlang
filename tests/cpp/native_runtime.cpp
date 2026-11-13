#include "GhostMatrix.hpp"
#include "GhostTextModel.hpp"
#include "GhostDevice.hpp"
#include <cmath>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <sstream>
void require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
void near(double a,double b,double tolerance=1e-10) { require(std::abs(a-b)<=tolerance*(1+std::abs(a)),"Valores divergentes"); }
template<class F> void rejects(F operation) { bool caught=false; try { operation(); } catch(const std::exception&) {caught=true;} require(caught,"Operacao invalida nao rejeitada"); }
int main(int argc,char** argv) {
    try {
        const std::string device=argc>1?argv[1]:"cpu";
        use_device(device);
        const bool cuda=device_backend()=="cuda";
        std::cout<<device_info()<<'\n';
        GhostNumberG i(0.0,1.0,0.0), theta(0.0,0.0,1.0), zero(0.0), one(1.0);
        near((theta*theta).p,0); near((i*theta).p,0);
        near(((i*i)*theta).p,-1); near((i*(i*theta)).p,0);
        near((one/zero).p,1); near((GhostNumberG(3.0,4.0,2.0)/zero).p,5);
        near(GhostNumberG(3.0,4.0,2.0,2.0).norm_squared(),33);
        for(int n: {1,3,17,65}) {
            GhostMatrix a(n,n+2),b(n+2,n+1);
            for(int r=0;r<n;++r) for(int c=0;c<n+2;++c) a(r,c)=GhostNumberG((r-c)*0.01,(r+c)*0.02,0.13,2.0);
            for(int r=0;r<n+2;++r) for(int c=0;c<n+1;++c) b(r,c)=GhostNumberG((r+c)*0.03,(r-c)*0.01,-0.07,3.0);
            use_device("cpu"); auto expected=a*b;
            use_device(device); auto actual=a*b;
            for(int r=0;r<n;++r) for(int c=0;c<n+1;++c) {
                near(expected(r,c).x,actual(r,c).x); near(expected(r,c).y,actual(r,c).y);
                near(expected(r,c).p,actual(r,c).p); near(actual(r,c).theta2,0);
            }
        }
        if(cuda) require(gpu_matrix_calls()>0,"Nenhum kernel matricial executado");
        rejects([] { GhostTextModel invalid(0); });
        GhostTextModel first(4,4,8,2,64,19),second(4,4,8,2,64,23);
        first.self_test();
        const auto base=std::filesystem::temp_directory_path()/("ghost-native-runtime-"+device+"-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(base);
        struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code ec; std::filesystem::remove_all(path,ec); } } cleanup{base};
        const auto train=(base/"treino.txt").string(), val=(base/"validacao.txt").string();
        std::ofstream(train)<<"Usuário: ola mundo\nAssistente: ola pessoa amiga\n\nUsuário: boa noite\nAssistente: boa noite amiga\n";
        std::ofstream(val)<<"Usuário: ola amiga\nAssistente: ola pessoa amiga boa noite ola pessoa amiga\n";
        first.prepare(train,val); second.prepare(train,val);
        double before=first.validate(); first.epoch(0.001); double after=first.validate();
        require(std::isfinite(before)&&std::isfinite(after)&&before!=after,"Treino nao mudou o modelo");
        first.save((base/"first.bin").string()); second.epoch(0.001);
        near(first.validate(),after,1e-7); // Buffers de segunda instancia nao alteram primeira.
        first.self_test(); near(first.validate(),after,1e-7);
        GhostTextModel restored; restored.load((base/"first.bin").string());
        near(restored.evaluate(val),after,1e-7);
        require(restored.epochs()==1,"Epocas nao restauradas");
        // Streaming conserva texto, UTF-8 e estado do modelo; avisos precedem o rotulo.
        const std::string prompt="Usuário: ola\nAssistente:";
        const auto expected=first.generate(prompt,12);
        std::ostringstream captured;
        struct Capture {
            std::streambuf *out,*err;
            Capture(std::ostream& target):out(std::cout.rdbuf(target.rdbuf())),err(std::cerr.rdbuf(target.rdbuf())) {}
            ~Capture() { std::cout.rdbuf(out); std::cerr.rdbuf(err); }
        };
        {
            Capture capture(captured);
            require(first.stream(prompt,12,0,"IA> ")==expected,"Texto stream divergiu");
        }
        require(captured.str()=="IA> "+(expected.empty()?std::string("[fim de resposta]"):expected)+"\n","Saida incremental divergiu");
        captured.str(""); captured.clear();
        {
            Capture capture(captured);
            text_stream("Olá! θ 🙂\nCódigo: 1 + 2",1);
        }
        require(captured.str()=="Olá! θ 🙂\nCódigo: 1 + 2\n","UTF-8 alterado");
        captured.str(""); captured.clear();
        {
            Capture capture(captured);
            first.stream("token_inexistente",12,0,"IA> ");
        }
        require(captured.str().find("[Ghost textmodel]")<captured.str().find("IA> "),"Aviso misturado a resposta");
        rejects([&] { first.stream(prompt,12,101); });
        rejects([&] { text_stream("invalido",-2); });
        near(first.validate(),after,1e-7);

        rejects([&] { restored.validate(); });
        rejects([&] { first.epoch(NAN); });
        rejects([&] { first.prepare(train,train); });
        std::ofstream(base/"bad.bin")<<"bad checkpoint";
        rejects([&] { restored.load((base/"bad.bin").string()); });
        rejects([&] { restored.generate("ola"); });
        std::cout<<"OK: matrizes x/y/p, regras G, treino, isolamento, checkpoint e erros.\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
