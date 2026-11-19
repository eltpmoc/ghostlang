#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstdio> // Adicionado para std::remove (delecao segura de arquivos)
#include "../include/GhostDiag.hpp"
#include "../include/GhostModules.hpp"
#include "../include/GhostAst.hpp"
#include "../include/GhostTypes.hpp"
#include "../include/GhostCodegen.hpp"
#include "../include/GhostFormatCode.hpp"
#include <regex>
#include <algorithm>
#include <sstream>
#include "../include/Interpreter.hpp" // Novo módulo do REPL
#include "../include/GhostLSP.hpp" // Language Server Protocol (LSP)
#include "../include/GhostPM.hpp" // Gerenciador de Pacotes GhostPM
#include "../include/GhostDAP.hpp" // Debug Adapter Protocol (ghost debug --dap)
#ifdef _WIN32
#define PATH_SEP_GHOST ";"
#else
#define PATH_SEP_GHOST ":"
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <limits.h>
#endif
#include <filesystem>
#include <set>

using namespace std;
namespace fs = std::filesystem;

// Citar argumentos evita que caminhos sejam interpretados como comandos pelo shell.
string shellQuote(const string& value) {
#ifdef _WIN32
    if (value.find_first_of("\"%\r\n") != string::npos) throw std::runtime_error("Caminho invalido");
    return "\"" + value + "\"";
#else
    string out="'";
    for (char c:value) out += c=='\'' ? "'\\''" : string(1,c);
    return out + "'";
#endif
}

string getExecutablePath() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return string(buffer);
#else
    char buffer[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
    return string(buffer, (count > 0) ? count : 0);
#endif
}

void printHelp() {
    cout << "GhostLang - uso:" << endl;
    cout << "  ghost                         REPL interativo (tambem: ghost repl)" << endl;
    cout << "  ghost run arquivo.g [args]    compila numa pasta temporaria e executa (passa os argumentos)" << endl;
    cout << "  ghost build arquivo.g [-o s]  gera o executavel (padrao: arquivo.exe ao lado do .g)" << endl;
    cout << "  ghost check arquivo.g ...     confere a sintaxe sem compilar o C++" << endl;
    cout << "  ghost ast arquivo.g           mostra a arvore sintatica (AST) e todos os erros de sintaxe" << endl;
    cout << "  ghost cpp arquivo.g           mostra o C++ gerado a partir da AST (ou o que ainda nao e coberto)" << endl;
    cout << "  ghost cpp --comparar a.g ...  compara o C++ da AST com os oraculos congelados" << endl;
    cout << "  ghost cpp --comparar-com <pasta> a.g ...  compara o C++ da AST com pasta de arquivos congelados" << endl;
    cout << "  ghost test [pasta]            roda cada X.g que tem X.expected (ao lado ou em ../golden/);" << endl;
    cout << "                                com X.erros, confere que a compilacao falha com essas mensagens" << endl;
    cout << "  ghost fmt arquivo.g [-w]      formata o codigo-fonte (com -w, altera o arquivo; --check confere)" << endl;
    cout << "  ghost lsp                     Language Server Protocol (JSON-RPC para IDEs e editores)" << endl;
    cout << "  ghost debug arquivo.g [args]  compila com simbolos de depuracao (-g) e inicia depurador" << endl;
    cout << "  ghost init [nome]             inicializa um novo projeto GhostLang com ghost.toml" << endl;
    cout << "  ghost pm <subcomando>         gerenciador de pacotes GhostPM (init, add, remove, list, install)" << endl;
    cout << "  ghost arquivo.g -o saida      uso antigo, igual a ghost build" << endl;
    cout << "  ghost version | help" << endl;
    cout << "Opcoes de compilacao:" << endl;
    cout << "  -x      Linkagem estatica: o executavel roda sem o MSYS2 (padrao no Windows)" << endl;
    cout << "  -d      Linkagem dinamica (precisa das DLLs do g++ no PATH; padrao fora do Windows)" << endl;
    cout << "  --wasm  Compilar para WebAssembly (requer emscripten)" << endl;
}

fs::path findGhostRoot(const fs::path& exeDir) {
    std::error_code ec;
    const fs::path candidates[] = { exeDir, exeDir.parent_path(), fs::current_path(ec) };
    for (const auto& c : candidates) {
        if (!c.empty() && fs::exists(c / "include" / "GhostNumberG.hpp", ec)) return c;
    }
    return exeDir;
}

fs::path findRuntimeLib(const fs::path& exeDir, const fs::path& ghostRoot) {
    std::error_code ec;
    const fs::path candidates[] = { exeDir / "libghost_runtime.a", ghostRoot / "lib" / "libghost_runtime.a" };
    for (const auto& c : candidates) {
        if (fs::exists(c, ec)) return c;
    }
    return fs::path();
}

string trim_import(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Raiz da GhostLang (para achar libs/*.g) e arquivos ja importados (evita duplicar codigo).
static fs::path g_ghostRoot;
static std::set<string> g_imported;

// import "x.g" procura, nesta ordem: pasta do arquivo que importa, pasta atual e raiz da GhostLang.
fs::path locateImport(const string& filename, const fs::path& fromDir) {
    std::error_code ec;
    fs::path p(filename);
    if (fromDir.empty() || p.is_absolute()) return p;
    const fs::path candidates[] = { fromDir / p, fs::current_path(ec) / p, g_ghostRoot / p };
    for (const auto& c : candidates) {
        if (!c.empty() && fs::exists(c, ec)) return c;
    }
    return fromDir / p;
}

// Junta o texto do programa com o dos imports (o Parser recebe um texto só; GhostDiag guarda de que arquivo
// e linha veio cada linha). import "x.g" as m;  -> as declarações de x.g ganham o prefixo m__ (GhostModules.hpp)
// e o lexer lê m.nome como m__nome. Cada arquivo entra uma vez por apelido; import circular dá aviso.
static std::vector<string> g_importStack; // arquivos sendo lidos (para mostrar a cadeia de um ciclo)
string resolveImports(const string& filename, const fs::path& fromDir = fs::path(), const string& alias = "") {
    fs::path path = locateImport(filename, fromDir);
    ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        cerr << "erro GHOST-E1001: nao foi possivel importar o arquivo " << filename << endl;
        return "";
    }
    std::error_code ec;
    fs::path canon = fs::weakly_canonical(path, ec);
    const string canonKey = (ec ? path : canon).string();
    // Nome mostrado nos erros: o caminho como foi pedido (principal) ou relativo à pasta atual (imports)
    string display = filename;
    if (!fromDir.empty()) {
        fs::path rel = fs::relative(ec ? path : canon, fs::current_path(ec), ec);
        display = (ec || rel.empty()) ? path.string() : rel.string();
    }
    for (auto& c : display) if (c == '\\') c = '/';   // o mesmo nome escrito nos #line do C++ gerado
    if (std::find(g_importStack.begin(), g_importStack.end(), canonKey) != g_importStack.end()) {
        string chain;
        bool on = false;
        for (const auto& f : g_importStack) {
            if (f == canonKey) on = true;
            if (on) chain += fs::path(f).filename().string() + " -> ";
        }
        cerr << "[Ghost] aviso: import circular: " << chain << fs::path(canonKey).filename().string()
             << " (o import repetido foi ignorado)" << endl;
        return "";
    }
    if (!g_imported.insert(canonKey + "|" + alias).second) return ""; // ja importado (com este apelido)
    const fs::path baseDir = (ec ? path : canon).parent_path();

    // texto inteiro (sem BOM e sem \r)
    std::stringstream buf;
    buf << file.rdbuf();
    string text = buf.str();
    if (text.size() >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF) text.erase(0, 3);
    {
        string t2;
        for (char c : text) if (c != '\r') t2 += c;
        text.swap(t2);
    }
    const string original = text;
    if (!alias.empty()) { // import ... as m: renomeia as declarações do próprio arquivo
        const std::set<string> names = ghost_module_exports(text);
        ghost_module_aliases()[alias].insert(names.begin(), names.end());
        text = ghost_mangle_module(text, alias, names);
    }
    auto splitLines = [](const string& t) {
        std::vector<string> v;
        string cur;
        for (char c : t) { if (c == '\n') { v.push_back(cur); cur.clear(); } else cur += c; }
        if (!cur.empty()) v.push_back(cur);
        return v;
    };
    const std::vector<string> origLines = splitLines(original), lines = splitLines(text);
    GhostDiag& diag = GhostDiag::get();
    diag.fileLines[display] = origLines; // o trecho mostrado nos erros é o texto original

    g_importStack.push_back(canonKey);
    string sourceCode;
    for (size_t k = 0; k < lines.size(); ++k) {
        const int lineNo = static_cast<int>(k) + 1;
        string importFile, importAlias;
        if (ghost_parse_import_line(lines[k], importFile, importAlias)) {
            if (!importAlias.empty() && !ghost_module_aliases().count(importAlias)) ghost_module_aliases()[importAlias];
            sourceCode += resolveImports(importFile, baseDir, importAlias) + "\n";
            diag.lineMap.push_back({display, lineNo}); // a linha em branco que fica no lugar do import
        } else {
            sourceCode += lines[k] + "\n";
            diag.lineMap.push_back({display, lineNo});
        }
    }
    g_importStack.pop_back();
    return sourceCode;
}


// O g++ recusou o C++ gerado: mostra cada erro na linha do .g (erro GHOST-E9001) em vez de no _temp.cpp.
// Erros dentro dos cabeçalhos (templates) usam o último "arquivo.g:linha" citado antes deles ("required from").
static void reportCppErrors(const string& logPath, const string& cppPath) {
    ifstream in(logPath);
    vector<string> lines;
    for (string l; getline(in, l);) { if (!l.empty() && l.back() == '\r') l.pop_back(); lines.push_back(l); }
    const std::regex loc(R"(^(.*?\.g):(\d+):(\d+):\s*(?:fatal )?error:\s*(.*)$)");
    const std::regex anyErr(R"(^(.*?):(\d+):(\d+):\s*(?:fatal )?error:\s*(.*)$)");
    const std::regex gRef(R"(([^\s:]+\.g):(\d+):(\d+))");
    std::set<string> seen;
    string lastFile; int lastLine = 0, lastCol = 0;
    int shown = 0;
    for (const string& l : lines) {
        std::smatch m;
        if (std::regex_search(l, m, gRef)) { lastFile = m[1]; lastLine = std::stoi(m[2]); lastCol = std::stoi(m[3]); }
        string file, msg; int ln = 0, col = 0;
        if (std::regex_match(l, m, loc)) { file = m[1]; ln = std::stoi(m[2]); col = std::stoi(m[3]); msg = m[4]; }
        else if (std::regex_match(l, m, anyErr)) {
            msg = m[4];
            if (!lastFile.empty()) { file = lastFile; ln = lastLine; col = lastCol; }
        } else continue;
        const string key = file + ":" + std::to_string(ln) + ":" + msg;
        if (!seen.insert(key).second) continue;
        if (++shown > 8) break;
        if (file.empty()) cerr << "erro GHOST-E9001: o C++ gerado nao compilou: " << msg << "\n";
        else ghost_diag_print(file, ln, 0, "GHOST-E9001", "o C++ gerado nao compilou: " + msg); // a coluna do g++ é do C++, não do .g
        (void)col;
    }
    if (shown == 0) for (const string& l : lines) cerr << l << "\n"; // formato desconhecido: mostra o log inteiro
    cerr << "[Ghost] Log completo do g++: " << logPath << "  (C++ gerado: " << cppPath << ")" << endl;
}

// Compila um .g para executável (ou WebAssembly). quiet: sem as mensagens "[Ghost] ..." (ghost run / ghost test).
// Erros de sintaxe pela AST: o Parser (gerador de C++) para no primeiro erro e deixa passar alguns
// (ex.: ';' faltando antes de outro comando); a AST lê o programa inteiro e se recupera dos erros.
// - Parser encerra com erro: mostra esse erro junto com os da AST, em ordem de linha (um por linha).
// - Parser termina sem erro mas a AST achou erros: mostra os da AST e a compilação para.
// GHOST_AST=0 desliga (só o Parser decide).
// AST + verificador de tipos
static AstParseResult parseAndCheck(const string& source) {
    AstParseResult r = ghost_ast_parse(source);
    if (r.errors.empty() && r.program) r.errors = ghost_typecheck(*r.program);
    return r;
}
static void emitErrors(const std::vector<AstDiagnostic>& errors) {
    for (const auto& e : errors) {
        const auto loc = GhostDiag::get().locate(e.line);
        ghost_diag_print(loc.first, loc.second, e.col, e.code.empty() ? ghost_diag_code(e.msg) : e.code, e.msg);
    }
    if (errors.size() > 1) cerr << "[Ghost] " << errors.size() << " erros encontrados." << endl;
}

// ghost cpp arquivo.g: C++ gerado a partir da AST (c40, etapa 2).
// ghost cpp --comparar a.g ...: para cada arquivo, compara com o C++ do Parser (mesmos tokens e #line).
// ghost cpp --congelar <pasta> a.g ...: grava o C++ normalizado do Parser em <pasta>/<caminho__com__>.cpp.txt
// ghost cpp --comparar-com <pasta> a.g ...: compara o C++ da AST com esses arquivos congelados (oráculo
//   que substitui o Parser antigo; c40, etapa 3). Os arquivos só mudam de propósito (ver docs/NUCLEO.md).
static string frozenName(const string& file) {
    string p = file;
    std::replace(p.begin(), p.end(), '\\', '/');
    while (p.rfind("./", 0) == 0) p = p.substr(2);
    if (p.size() > 2 && p.compare(p.size() - 2, 2, ".g") == 0) p = p.substr(0, p.size() - 2);
    string out;
    for (char c : p) {
        if (c == '/') out += "__";
        else if (c == ':') out += "_";
        else out += c;
    }
    return out + ".cpp.txt";
}

static int cmdCpp(int argc, char* argv[]) {
    int first = 2;
    const string opt = argc > 2 ? string(argv[2]) : string();
    const bool compare = opt == "--comparar";
    const bool freeze = opt == "--congelar";
    const bool frozen = opt == "--comparar-com";
    const bool wasm = opt == "--wasm";
    string folder;
    if (compare) {
        first = 3;
        // --comparar padrão compara com tests/codegen/esperado
        fs::path root = findGhostRoot(fs::path(getExecutablePath()).parent_path());
        folder = (root / "tests" / "codegen" / "esperado").string();
    }
    if (freeze || frozen) {
        if (argc <= 3) { cerr << "Uso: ghost cpp " << opt << " <pasta> a.g b.g ..." << endl; return 1; }
        folder = argv[3];
        first = 4;
    }
    if (wasm) {
        first = 3;
    }
    if (argc <= first) {
        cerr << "Uso: ghost cpp arquivo.g  |  ghost cpp --wasm arquivo.g  |  ghost cpp --comparar a.g ...  |  "
                "ghost cpp --congelar <pasta> a.g ...  |  ghost cpp --comparar-com <pasta> a.g ..." << endl;
        return 1;
    }
    if (freeze) fs::create_directories(folder);
    int same = 0, uncovered = 0, different = 0;
    for (int i = first; i < argc; ++i) {
        g_imported.clear();
        ghost_module_aliases().clear();
        GhostDiag::get().lineMap.clear();
        GhostDiag::get().fileLines.clear();
        GhostDiag::get().cppFile = "";
        g_ghostRoot = findGhostRoot(fs::path(getExecutablePath()).parent_path());
        const string src = resolveImports(argv[i]);
        if (src.empty()) return 1;
        if (freeze) {
            const AstParseResult r = ghost_ast_parse(src);
            if (!r.errors.empty() || !r.program) { emitErrors(r.errors); return 1; }
            const GhostCodegenResult cg = ghost_codegen(*r.program, false, "");
            if (!cg.ok) { cerr << argv[i] << ": nao coberto: " << cg.reason << endl; return 1; }
            const string norm = ghost_cpp_normalized(cg.cpp);
            const fs::path dest = fs::path(folder) / frozenName(argv[i]);
            ofstream o(dest, std::ios::binary);
            o << norm;
            if (!o) { cerr << "[Ghost] nao consegui gravar " << dest.string() << endl; return 1; }
            ++same;
            cout << "congelado    " << argv[i] << " -> " << dest.generic_string() << endl;
            continue;
        }
        // comparar: basta a sintaxe (projetos com C++ nativo têm funções que o verificador não conhece)
        const AstParseResult r = (compare || frozen) ? ghost_ast_parse(src) : parseAndCheck(src);
        if (!r.errors.empty()) {
            emitErrors(r.errors);
            return 1;
        }
        const GhostCodegenResult cg = ghost_codegen(*r.program, wasm, "");
        if (!compare && !frozen) {
            if (!cg.ok) { cerr << argv[i] << ": ainda nao coberto pelo gerador da AST: " << cg.reason << endl; return 2; }
            cout << cg.cpp;
            return 0;
        }
        if (!cg.ok) {
            ++uncovered;
            cout << "nao coberto  " << argv[i] << ": " << cg.reason << endl;
            continue;
        }
        if (compare || frozen) {
            const fs::path ref = fs::path(folder) / frozenName(argv[i]);
            ifstream in(ref, std::ios::binary);
            if (!in) {
                ++different;
                cout << "SEM ESPERADO " << argv[i] << " (falta " << ref.generic_string() << ")" << endl;
                continue;
            }
            std::stringstream buf;
            buf << in.rdbuf();
            string want = buf.str();
            want.erase(std::remove(want.begin(), want.end(), '\r'), want.end());
            const string got = ghost_cpp_normalized(cg.cpp);
            if (got == want) {
                ++same;
                cout << "igual        " << argv[i] << endl;
            } else {
                ++different;
                // primeira linha diferente, com o número da linha no arquivo congelado
                std::istringstream a(want), b(got);
                string la, lb;
                int n = 0;
                while (true) {
                    const bool ha = static_cast<bool>(std::getline(a, la)), hb = static_cast<bool>(std::getline(b, lb));
                    ++n;
                    if (!ha && !hb) break;
                    if (!ha) la = "(fim)";
                    if (!hb) lb = "(fim)";
                    if (la != lb || !ha || !hb) break;
                }
                cout << "DIFERENTE    " << argv[i] << "\n  linha " << n << " de " << ref.generic_string()
                     << "\n  esperado: " << la.substr(0, 160) << "\n  AST:      " << lb.substr(0, 160) << endl;
            }
            continue;
        }
    }
    cout << same << " igual(is), " << different << " diferente(s), " << uncovered << " nao coberto(s)" << endl;
    return different == 0 ? 0 : 1;
}
// ghost ast arquivo.g: árvore sintática e erros de sintaxe (todos, não só o primeiro)
static int cmdAst(int argc, char* argv[]) {
    if (argc < 3) { cerr << "Uso: ghost ast arquivo.g" << endl; return 1; }
    g_imported.clear();
    GhostDiag::get().lineMap.clear();
    GhostDiag::get().fileLines.clear();
    g_ghostRoot = findGhostRoot(fs::path(getExecutablePath()).parent_path());
    const string src = resolveImports(argv[2]);
    if (src.empty()) return 1;
    const AstParseResult r = parseAndCheck(src);
    for (const auto& e : r.errors) {
        const auto loc = GhostDiag::get().locate(e.line);
        ghost_diag_print(loc.first, loc.second, e.col, e.code.empty() ? ghost_diag_code(e.msg) : e.code, e.msg);
    }
    if (argc < 4 || string(argv[3]) != "--erros") ghost_ast_dump(*r.program, cout);
    if (!r.errors.empty()) cerr << "[Ghost] " << r.errors.size() << " erro(s)." << endl;
    return r.errors.empty() ? 0 : 1;
}

static bool commandExists(const string& cmd) {
#ifdef _WIN32
    string check = "where " + cmd + " >nul 2>nul";
#else
    string check = "which " + cmd + " >/dev/null 2>&1";
#endif
    return std::system(check.c_str()) == 0;
}

static int compileProgram(const string& inputFile, const string& outputFile, bool isStatic, bool isWasm, bool quiet, bool isDebug = false) {
    // ghost test chama a compilação várias vezes no mesmo processo? Não: cada teste usa um processo filho.
    // Mesmo assim, o estado global é zerado aqui (imports já lidos e mapa de linhas dos diagnósticos).
    g_imported.clear();
    GhostDiag::get().lineMap.clear();
    GhostDiag::get().fileLines.clear();
    std::streambuf* savedOut = nullptr;
    std::ostringstream sink;
    if (quiet) savedOut = cout.rdbuf(sink.rdbuf());
    struct Restore { std::streambuf* b; ~Restore() { if (b) cout.rdbuf(b); } } restore{savedOut};

    // Resolvendo imports recursivamente (Pre-Processador)
    g_ghostRoot = findGhostRoot(fs::path(getExecutablePath()).parent_path());
    string sourceCode = resolveImports(inputFile);
    
    // Se sourceCode vier vazio (falha no arquivo principal), encerra
    if (sourceCode.empty()) return 1;

    cout << "[Ghost] Transpilando " << inputFile << (isWasm ? " para WebAssembly" : "") << "..." << endl;
    
    // 1. Parsing e Geracao de Codigo C++ (exclusivo via AST + verificador de tipos + GhostCodegen)
    GhostDiag::get().cppFile = outputFile + "_temp.cpp";
    const AstParseResult ast = parseAndCheck(sourceCode);
    if (!ast.errors.empty() || !ast.program) {
        emitErrors(ast.errors);
        return 1;
    }

    const GhostCodegenResult cg = ghost_codegen(*ast.program, isWasm, GhostDiag::get().cppFile);
    if (!cg.ok) {
        cerr << "[Ghost] Erro: gerador da AST nao cobre: " << cg.reason << endl;
        return 1;
    }
    string cppCode = cg.cpp;

    // 2. Salvando o arquivo temporario
    string tempCppFile = outputFile + "_temp.cpp";
    ofstream outCpp(tempCppFile);
    outCpp << cppCode;
    outCpp.close();

    // Determinar qual g++ usar (se há um portatil ou sistema)
    fs::path exeDir = fs::path(getExecutablePath()).parent_path();
#ifdef _WIN32
    fs::path internalGpp = exeDir / "compiler" / "bin" / "g++.exe";
#else
    fs::path internalGpp = exeDir / "compiler" / "bin" / "g++";
#endif

    string gppBaseCmd = "g++"; // Fallback para o global
    if (fs::exists(internalGpp)) {
        gppBaseCmd = "\"" + internalGpp.string() + "\""; // Usa aspas caso tenha espaço no caminho
        // O GCC 16 do MSYS2 e configurado com --with-sysroot=/ucrt64: fora do MSYS2, os headers do
        // sistema (wchar.h...) e as bibliotecas so sao achados com o sysroot apontando para compiler/.
        gppBaseCmd += " --sysroot=" + shellQuote(internalGpp.parent_path().parent_path().generic_string());
        // O cc1plus/as/ld embutidos dependem das DLLs de compiler/bin (gmp, mpfr, zstd...), e o
        // Windows só as acha pela pasta do executável ou pelo PATH: põe compiler/bin na frente.
        const std::string binDir = internalGpp.parent_path().string();
        const char* oldPath = std::getenv("PATH");
        const std::string newPath = binDir + (oldPath ? std::string(PATH_SEP_GHOST) + oldPath : std::string());
#ifdef _WIN32
        _putenv_s("PATH", newPath.c_str());
#else
        setenv("PATH", newPath.c_str(), 1);
#endif
    }

    // 3. Compilando o Binario
    cout << "[Ghost] Gerando " << (isWasm ? "bundle WebAssembly..." : "executavel hiper-otimizado...") << endl;
    
    // Raiz da GhostLang: pasta que contem include/GhostNumberG.hpp. Procurar a partir
    // do executavel permite compilar de qualquer diretorio, nao so da raiz do projeto.
    fs::path ghostRoot = findGhostRoot(exeDir);

    // Compila chamando o g++ ou emcc
    string compileCmd;
    if (isWasm) {
        if (!commandExists("emcc")) {
            fs::path wasmCppPath = outputFile + ".wasm.cpp";
            std::error_code ec;
            fs::copy_file(tempCppFile, wasmCppPath, fs::copy_options::overwrite_existing, ec);

            fs::path htmlPath = outputFile + ".html";
            ofstream hf(htmlPath);
            hf << "<!DOCTYPE html>\n<html lang=\"pt-BR\">\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n  <title>GhostLang WebAssembly</title>\n";
            hf << "  <style>\n    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0f172a; color: #f8fafc; padding: 24px; }\n";
            hf << "    .card { background: #1e293b; border-radius: 8px; padding: 20px; max-width: 600px; margin: 0 auto; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.1); }\n";
            hf << "    h1 { color: #38bdf8; margin-top: 0; }\n    pre { background: #0f172a; padding: 12px; border-radius: 6px; overflow-x: auto; color: #a5f3fc; }\n  </style>\n</head>\n<body>\n";
            hf << "  <div class=\"card\">\n    <h1>GhostLang WebAssembly</h1>\n    <p>Módulo GhostLang transpilado com sucesso para WebAssembly C++ (Emscripten).</p>\n";
            hf << "    <pre id=\"output\">Carregando módulo WASM...</pre>\n  </div>\n";
            hf << "  <script>\n    var Module = {\n      print: function(text) { var el = document.getElementById('output'); if (el) el.innerText += text + '\\n'; console.log(text); },\n";
            hf << "      onRuntimeInitialized: function() {\n        var el = document.getElementById('output');\n        if (el) el.innerText = 'GhostLang WASM inicializado com sucesso!\\n';\n      }\n    };\n  </script>\n";
            hf << "  <script async src=\"" << fs::path(outputFile).stem().string() << ".js\"></script>\n</body>\n</html>\n";
            hf.close();

            cout << "[Ghost] Sucesso! Codigo WebAssembly transpilado com sucesso." << endl;
            cout << "[Ghost] Arquivo C++ (WASM): " << wasmCppPath.string() << endl;
            cout << "[Ghost] Template Web: " << htmlPath.string() << endl;
            cout << "[Ghost] Aviso: Compilador Emscripten ('emcc') nao foi detectado no PATH." << endl;
            cout << "[Ghost] Para gerar os binarios .wasm/.js no navegador via emsdk:" << endl;
            cout << "[Ghost]   emcc -O3 -I" << ghostRoot.string() << " " << wasmCppPath.string() << " -o " << outputFile << ".html -s WASM=1 -s NO_EXIT_RUNTIME=1 -s \"EXPORTED_RUNTIME_METHODS=['ccall','cwrap']\"" << endl;
            return 0;
        }
        compileCmd = "emcc -O3 -I" + shellQuote(ghostRoot.string()) + " -I. " + tempCppFile + " -o " + outputFile + ".html -s WASM=1 -s NO_EXIT_RUNTIME=1 -s \"EXPORTED_RUNTIME_METHODS=['ccall','cwrap']\"";
    } else {
        string staticFlags = isStatic ? "-static -static-libgcc -static-libstdc++ " : "";
        string includes = "-I" + shellQuote(ghostRoot.string()) + " -I" + shellQuote(exeDir.string()) + " -I. ";
        // Sem ghost-runtime.conf, procura a libghost_runtime.a ao lado do ghost.exe
        // (copiada pelo buildWin.ps1). Sem ela, GhostNumberG/GhostMatrix nao linkam.
        fs::path runtimeLib = findRuntimeLib(exeDir, ghostRoot);
        string runtimeArgs = "";
        if (!runtimeLib.empty()) {
            runtimeArgs = " " + shellQuote(runtimeLib.string());
        } else {
            cerr << "[Ghost] Aviso: libghost_runtime.a nao encontrada ao lado do ghost. "
                 << "Programas que usam ghost, gate, state ou IA vao falhar no link." << endl;
        }
#ifdef _WIN32
        compileCmd = gppBaseCmd + " -std=c++17 " + (isDebug ? "-g -O0 " : "-O3 ") + "-fopenmp -pthread " + staticFlags + (runtimeLib.empty() ? "" : "-DGHOST_NATIVE_RUNTIME=1 ") + includes + shellQuote(tempCppFile) + runtimeArgs + " -o " + shellQuote(outputFile) + " -lws2_32 -lwininet";
#else
        // -ldl: o simulador quantico carrega o backend CUDA com dlopen
        compileCmd = gppBaseCmd + " -std=c++17 " + (isDebug ? "-g -O0 " : "-O3 ") + "-fopenmp -pthread " + staticFlags + (runtimeLib.empty() ? "" : "-DGHOST_NATIVE_RUNTIME=1 ") + includes + shellQuote(tempCppFile) + runtimeArgs + " -o " + shellQuote(outputFile) + " -ldl";
#endif
    }
    
    // Compiladores antigos, sem este manifesto, conservam sua forma de linkagem.
    if (!isWasm) {
        ifstream config(exeDir / "ghost-runtime.conf");
        string root,library,cuda,cudart;
        if (config && getline(config,root) && getline(config,library) && getline(config,cuda) && getline(config,cudart)) {
            if (!fs::exists(library)) { cerr << "Runtime nao compilado: " << library << endl; return 1; }
            compileCmd=shellQuote(fs::exists(internalGpp)?internalGpp.string():"g++") +
                (fs::exists(internalGpp) ? " --sysroot=" + shellQuote(internalGpp.parent_path().parent_path().generic_string()) : string()) +
                " -std=c++17 " + (isDebug ? "-g -O0 " : "-O3 ") + "-fopenmp -pthread -DGHOST_NATIVE_RUNTIME=1 -I"+shellQuote(root)+
                " -I"+shellQuote((fs::path(root)/"include").string())+" "+shellQuote(tempCppFile)+
                " "+shellQuote(library)+" -o "+shellQuote(outputFile);
            if (cuda=="1") compileCmd += " -DGHOST_HAS_CUDA=1 "+shellQuote(cudart)+" -ldl -lrt";
#ifndef _WIN32
            else compileCmd += " -ldl";
#endif
#ifdef _WIN32
            compileCmd += " -lws2_32 -lwininet";
#endif
            if (isStatic) compileCmd += " -static -static-libgcc -static-libstdc++";
        }
    }

    // A saída do g++ vai para um log; em caso de erro, cada erro é mostrado na linha do .g (#line no C++ gerado)
    const string gppLog = outputFile + "_gpp.log";
    if (!isWasm) compileCmd += " 2> " + shellQuote(gppLog);
#ifdef _WIN32
    string finalCmd = "\"" + compileCmd + "\"";
    int compileResult = system(finalCmd.c_str());
#else
    int compileResult = system(compileCmd.c_str());
#endif
    if (!isWasm) {
        if (compileResult != 0) reportCppErrors(gppLog, tempCppFile);
        else { std::error_code rmEc; fs::remove(gppLog, rmEc); }
    }

    if (compileResult == 0) {
        if (isWasm) {
            cout << "[Ghost] Sucesso! Hospede os arquivos web e abra o " << outputFile << ".html no navegador!" << endl;
        } else {
            cout << "[Ghost] Sucesso! Execute com: ./" << outputFile << endl;
            // Programas quanticos: leva o backend CUDA (se existir ao lado do ghost) para junto do executavel.
            // Sem ele, o simulador roda na CPU normalmente.
            if (cppCode.find("GhostQReg") != string::npos || cppCode.find("GhostQCircuit") != string::npos || cppCode.find("GhostQLayer") != string::npos) {
#ifdef _WIN32
                const char* backendName = "ghost_quantum_cuda.dll";
#else
                const char* backendName = "libghost_quantum_cuda.so";
#endif
                std::error_code ec;
                fs::path backend = exeDir / backendName;
                fs::path outDir = fs::absolute(fs::path(outputFile), ec).parent_path();
                if (fs::exists(backend, ec) && !outDir.empty() && !fs::equivalent(outDir, exeDir, ec)) {
                    fs::copy_file(backend, outDir / backendName, fs::copy_options::update_existing, ec);
                    if (!ec) cout << "[Ghost] Backend quantico CUDA copiado: " << (outDir / backendName).string() << endl;
                }
            }
        }
        
        // Forma correta e nativa (C++) de deletar o arquivo temporario
        if (0) {
            cerr << "[Ghost] Aviso: Nao foi possivel deletar o lixo temporario: " << tempCppFile << endl;
        }
    } else {
        cerr << "[Ghost] Erro critico durante a geracao do " << (isWasm ? "WASM." : "binario.") << endl;
    }

    return compileResult == 0 ? 0 : 1;
}


// ---------------------------------------------------------------- CLI (c45): ghost run / build / check / test
static const char* GHOST_VERSAO = "GhostLang 0.9 (compilador C++17 + REPL)";

static bool isFlag(const string& a) { return a == "-x" || a == "-d" || a == "--wasm"; }

static string defaultOutput(const string& input, bool isWasm) {
    fs::path p(input);
    string out = (p.parent_path() / p.stem()).string();
#ifdef _WIN32
    if (!isWasm) out += ".exe";
#else
    (void)isWasm;
#endif
    return out;
}

static int exitCodeOf(int status) {
#ifdef _WIN32
    return status;
#else
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
#endif
}

static int runCommand(const string& cmd) {
#ifdef _WIN32
    return exitCodeOf(system(("\"" + cmd + "\"").c_str()));
#else
    return exitCodeOf(system(cmd.c_str()));
#endif
}

// ghost build arquivo.g [-o saida] [-x|-d|--wasm|-g]   (e o uso antigo: ghost arquivo.g -o saida)
static int cmdBuild(int argc, char* argv[], int first) {
    bool isWasm = false;
    bool isDebug = false;
#ifdef _WIN32
    bool isStatic = true;  // Windows: estatico por padrao (o programa roda sem libstdc++-6.dll e cia.)
#else
    bool isStatic = false;
#endif
    string inputFile, outputFile;
    for (int i = first; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-x") isStatic = true;
        else if (arg == "-d") isStatic = false;
        else if (arg == "--wasm") isWasm = true;
        else if (arg == "-g") isDebug = true;
        else if (arg == "-o" && i + 1 < argc) outputFile = argv[++i];
        else if (inputFile.empty()) inputFile = arg;
        else { cerr << "[Ghost] Argumento a mais: " << arg << endl; printHelp(); return 1; }
    }
    if (inputFile.empty()) { printHelp(); return 1; }
    if (outputFile.empty()) outputFile = defaultOutput(inputFile, isWasm);
    return compileProgram(inputFile, outputFile, isStatic, isWasm, false, isDebug);
}

// ghost run [-x|-d] arquivo.g [--] [argumentos do programa]
static int cmdRun(int argc, char* argv[]) {
#ifdef _WIN32
    bool isStatic = true;
#else
    bool isStatic = false;
#endif
    string inputFile;
    vector<string> progArgs;
    int i = 2;
    for (; i < argc; ++i) {
        string arg = argv[i];
        if (inputFile.empty() && arg == "-x") isStatic = true;
        else if (inputFile.empty() && arg == "-d") isStatic = false;
        else if (inputFile.empty()) inputFile = arg;
        else { if (arg != "--" || !progArgs.empty()) progArgs.push_back(arg); }
    }
    if (inputFile.empty()) { cerr << "Uso: ghost run arquivo.g [argumentos]" << endl; return 1; }
    std::error_code ec;
    fs::path dir = fs::temp_directory_path(ec) / "ghost-run";
    fs::create_directories(dir, ec);
    // Nome único por arquivo e por execução: dois 'ghost run prog.g' de pastas diferentes
    // (ou ao mesmo tempo) não podem sobrescrever o executável um do outro.
    const fs::path absInput = fs::absolute(inputFile, ec);
    std::ostringstream unico;
    unico << fs::path(inputFile).stem().string() << "_" << std::hex
          << (std::hash<std::string>{}(absInput.generic_string()) & 0xffffffffu) << "_"
#ifdef _WIN32
          << GetCurrentProcessId();
#else
          << getpid();
#endif
    const string exe = (dir / (unico.str() + (
#ifdef _WIN32
        ".exe"
#else
        ""
#endif
    ))).string();
    const int rc = compileProgram(inputFile, exe, isStatic, false, true);
    if (rc != 0) return rc;
    string cmd = shellQuote(exe);
    for (const auto& a : progArgs) cmd += " " + shellQuote(a);
    const int runRc = runCommand(cmd);
    fs::remove(exe, ec); // o executável temporário não fica acumulando em %TEMP%\ghost-run
    return runRc;
}

// ghost check arquivo.g: confere a sintaxe e tipos (AST + verificador) sem chamar o g++
static int cmdCheck(int argc, char* argv[]) {
    if (argc < 3) { cerr << "Uso: ghost check arquivo.g" << endl; return 1; }
    int bad = 0;
    for (int i = 2; i < argc; ++i) {
        g_imported.clear();
        GhostDiag::get().lineMap.clear();
        GhostDiag::get().fileLines.clear();
        g_ghostRoot = findGhostRoot(fs::path(getExecutablePath()).parent_path());
        const string src = resolveImports(argv[i]);
        if (src.empty()) { ++bad; continue; }
        GhostDiag::get().cppFile = "";
        const AstParseResult ast = parseAndCheck(src);
        if (!ast.errors.empty()) {
            emitErrors(ast.errors);
            ++bad;
            continue;
        }
        cout << "ok: " << argv[i] << endl;
    }
    return bad == 0 ? 0 : 1;
}

static string readNormalized(const fs::path& p) {
    ifstream in(p, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    string t = ss.str(), out;
    for (char c : t) if (c != '\r') out += c;
    return out;
}

// ghost test [pasta]: cada X.g que tem X.expected ao lado (ou em ../golden/X.expected) é compilado, executado
// e a saída comparada com o esperado. Cada teste roda em processos separados.
static int cmdTest(int argc, char* argv[]) {
    std::error_code ec;
    fs::path root = argc >= 3 ? fs::path(argv[2]) : (fs::exists("tests", ec) ? fs::path("tests") : fs::path("."));
    if (!fs::exists(root, ec)) { cerr << "[Ghost] Pasta nao encontrada: " << root.string() << endl; return 1; }
    vector<std::pair<fs::path, fs::path>> cases;
    for (auto it = fs::recursive_directory_iterator(root, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        const fs::path f = it->path();
        if (!it->is_regular_file(ec) || f.extension() != ".g") continue;
        const fs::path errs = f.parent_path() / (f.stem().string() + ".erros"); // teste de erro: deve falhar com essas mensagens
        if (fs::exists(errs, ec)) { cases.push_back({f, errs}); continue; }
        fs::path exp = f.parent_path() / (f.stem().string() + ".expected");
        if (!fs::exists(exp, ec)) exp = f.parent_path().parent_path() / "golden" / (f.stem().string() + ".expected");
        if (fs::exists(exp, ec)) cases.push_back({f, exp});
    }
    std::sort(cases.begin(), cases.end());
    if (cases.empty()) { cout << "[Ghost] Nenhum teste em " << root.string() << " (X.g com X.expected)" << endl; return 0; }
    fs::path work = fs::temp_directory_path(ec) / "ghost-test";
    fs::create_directories(work, ec);
    const string self = getExecutablePath();
#ifdef _WIN32
    _putenv_s("OMP_NUM_THREADS", "4");
#else
    setenv("OMP_NUM_THREADS", "4", 0);
#endif
    int passed = 0;
    vector<string> failed;
    for (const auto& c : cases) {
        const string name = c.first.stem().string();
        const fs::path exe = work / (name +
#ifdef _WIN32
            ".exe"
#else
            ""
#endif
        );
        const fs::path log = work / (name + ".log"), out = work / (name + ".out");
        const string build = shellQuote(self) + " build " + shellQuote(c.first.string()) + " -o " + shellQuote(exe.string()) +
                             " > " + shellQuote(log.string()) + " 2>&1";
        const bool expectError = c.second.extension() == ".erros";
        const int buildRc = runCommand(build);
        if (expectError) { // cada linha do .erros precisa aparecer na mensagem do compilador
            const string msg = readNormalized(log), want = readNormalized(c.second);
            std::istringstream w(want);
            string line, missing;
            while (std::getline(w, line))
                if (!line.empty() && msg.find(line) == string::npos) { missing = line; break; }
            if (buildRc == 0) { cout << "FALHOU  " << name << "  (deveria dar erro de compilacao)" << endl; failed.push_back(name); }
            else if (!missing.empty()) { cout << "FALHOU  " << name << "  (mensagem sem: \"" << missing << "\")" << endl; failed.push_back(name); }
            else { cout << "ok      " << name << "  (erro esperado)" << endl; ++passed; }
            continue;
        }
        if (buildRc != 0) {
            cout << "FALHOU  " << name << "  (nao compilou; veja " << log.string() << ")" << endl;
            failed.push_back(name);
            continue;
        }
        const string run = shellQuote(exe.string()) + " > " + shellQuote(out.string());
        runCommand(run);
        const string got = readNormalized(out), want = readNormalized(c.second);
        if (got == want) {
            cout << "ok      " << name << endl;
            ++passed;
        } else {
            // primeira linha diferente
            std::istringstream g(got), w(want);
            string lg, lw;
            int n = 0;
            while (true) {
                ++n;
                const bool hg = static_cast<bool>(std::getline(g, lg)), hw = static_cast<bool>(std::getline(w, lw));
                if (!hg && !hw) break;
                if (!hg) lg = "<fim>";
                if (!hw) lw = "<fim>";
                if (lg != lw) break;
            }
            cout << "FALHOU  " << name << "  (linha " << n << ": esperado \"" << lw << "\", obtido \"" << lg << "\")" << endl;
            failed.push_back(name);
        }
    }
    cout << "\n" << passed << " de " << cases.size() << " testes passaram";
    if (!failed.empty()) {
        cout << "; falharam:";
        for (const auto& f : failed) cout << " " << f;
    }
    cout << endl;
    return failed.empty() ? 0 : 1;
}

// ghost fmt arquivo.g [-w|--write] [--check]
static int cmdFmt(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Uso: ghost fmt arquivo.g [-w|--write] [--check]" << endl;
        return 1;
    }
    bool writeInPlace = false;
    bool checkOnly = false;
    std::vector<string> files;
    for (int i = 2; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-w" || arg == "--write") writeInPlace = true;
        else if (arg == "--check" || arg == "-c") checkOnly = true;
        else files.push_back(arg);
    }
    if (files.empty()) {
        cerr << "Nenhum arquivo especificado para formatar." << endl;
        return 1;
    }
    int unformatted = 0;
    for (const auto& file : files) {
        ifstream in(file, std::ios::binary);
        if (!in) {
            cerr << "erro GHOST-E1001: nao foi possivel abrir o arquivo " << file << endl;
            ++unformatted;
            continue;
        }
        std::stringstream ss;
        ss << in.rdbuf();
        string src = ss.str();
        string formatted = ghost_format_code(src);
        if (checkOnly) {
            // Fim de linha CRLF (padrao dos editores no Windows) nao conta como diferenca de formatacao
            string semCr = src;
            semCr.erase(std::remove(semCr.begin(), semCr.end(), '\r'), semCr.end());
            if (formatted != semCr) {
                cout << file << ": precisa de formatacao" << endl;
                ++unformatted;
            } else {
                cout << file << ": formatado" << endl;
            }
        } else if (writeInPlace) {
            if (formatted != src) {
                ofstream out(file, std::ios::binary);
                out << formatted;
                cout << "formatado: " << file << endl;
            } else {
                cout << "ja estava formatado: " << file << endl;
            }
        } else {
            cout << formatted;
        }
    }
    return (checkOnly && unformatted > 0) ? 1 : 0;
}

// ghost lsp: Language Server Protocol (JSON-RPC 2.0 sobre stdin/stdout)
static int cmdLsp(int argc, char* argv[]) {
    GhostLanguageServer::GhostLSP server;
    server.setRoot(findGhostRoot(fs::path(getExecutablePath()).parent_path()).string());
    for (int i = 2; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--test") {
            return server.runTestMode();
        }
    }
    server.run();
    return 0;
}

// gdb usado pelo depurador: GHOST_GDB, <pasta do ghost>/gdb/bin, <raiz>/third_party/gdb/bin, PATH
static string findGdb() {
    if (const char* env = std::getenv("GHOST_GDB")) if (*env) return env;
    std::error_code ec;
    const fs::path exeDir = fs::path(getExecutablePath()).parent_path();
    const fs::path root = findGhostRoot(exeDir);
#ifdef _WIN32
    const char* nome = "gdb.exe";
#else
    const char* nome = "gdb";
#endif
    for (const fs::path& c : {exeDir / "gdb" / "bin" / nome, root / "third_party" / "gdb" / "bin" / nome})
        if (fs::exists(c, ec)) return c.string();
    return "gdb";
}

// ghost debug arquivo.g [args]: compila com simbolos de depuracao (-g) e ativa DWARF #line
static int cmdDebug(int argc, char* argv[]) {
    if (argc < 3 || (argc >= 3 && (string(argv[2]) == "--help" || string(argv[2]) == "-h"))) {
        cout << "Uso: ghost debug arquivo.g [argumentos]   (compila com -g e inicia depurador)" << endl;
        cout << "     ghost debug --map arquivo.g           (exibe mapa de simbolos e linhas)" << endl;
        cout << "     ghost debug --dap                     (adaptador DAP para VS Code, usa o gdb)" << endl;
        return (argc >= 3 && (string(argv[2]) == "--help" || string(argv[2]) == "-h")) ? 0 : 1;
    }
    bool showMap = false;
    bool dapMode = false;
    string saidaSo; // --saida <exe>: so compila com simbolos (usado pelo adaptador DAP)
    string inputFile;
    vector<string> progArgs;
    for (int i = 2; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--map" || arg == "--symbols") showMap = true;
        else if (arg == "--dap") dapMode = true;
        else if (arg == "--saida" && i + 1 < argc) saidaSo = argv[++i];
        else if (inputFile.empty()) inputFile = arg;
        else progArgs.push_back(arg);
    }
    if (dapMode) {
        // DAP por stdin/stdout, controlando o gdb pela interface MI (src/GhostDAP.cpp)
        return GhostDAP::run(getExecutablePath(), findGdb());
    }
    if (inputFile.empty()) {
        cerr << "[Ghost Debugger] Arquivo .g nao especificado." << endl;
        return 1;
    }

    if (showMap) {
        cout << "[Ghost Debugger] Mapa de simbolos e linhas para: " << inputFile << endl;
        g_imported.clear();
        GhostDiag::get().lineMap.clear();
        GhostDiag::get().fileLines.clear();
        g_ghostRoot = findGhostRoot(fs::path(getExecutablePath()).parent_path());
        string src = resolveImports(inputFile);
        if (src.empty()) return 1;
        std::istringstream stream(src);
        string line;
        int ln = 1;
        while (std::getline(stream, line)) {
            string t = line;
            while (!t.empty() && (t[0] == ' ' || t[0] == '\t')) t.erase(0, 1);
            if (!t.empty() && t.rfind("//", 0) != 0) {
                cout << "  " << inputFile << ":" << ln << " -> " << line << endl;
            }
            ++ln;
        }
        cout << "[Ghost Debugger] Total de linhas mapeadas: " << (ln - 1) << endl;
        return 0;
    }

    std::error_code ec;
    fs::path dir = fs::temp_directory_path(ec) / "ghost-debug";
    fs::create_directories(dir, ec);
    const string exe = (dir / (fs::path(inputFile).stem().string() + (
#ifdef _WIN32
        ".exe"
#else
        ""
#endif
    ))).string();

    if (!saidaSo.empty()) return compileProgram(inputFile, saidaSo, true, false, false, true);

    cout << "[Ghost Debugger] Compilando com simbolos de depuracao (-g)..." << endl;
    const int rc = compileProgram(inputFile, exe, true, false, false, true);
    if (rc != 0) {
        cerr << "[Ghost Debugger] Falha na compilacao com simbolos de depuracao." << endl;
        return rc;
    }
    cout << "[Ghost Debugger] Binario de depuracao pronto: " << exe << endl;

    const string gdbExe = findGdb();
    string checkGdb = shellQuote(gdbExe) + " --version > nul 2>&1";
#ifndef _WIN32
    checkGdb = shellQuote(gdbExe) + " --version > /dev/null 2>&1";
#endif
    bool hasGdb = (system(checkGdb.c_str()) == 0);

    if (hasGdb) {
#ifdef _WIN32
        const bool terminal = _isatty(_fileno(stdin)) != 0;
#else
        const bool terminal = isatty(fileno(stdin)) != 0;
#endif
        // Sem terminal (CI, scripts): roda o programa no gdb em lote e mostra a pilha se ele quebrar
        string runCmd = shellQuote(gdbExe) + (terminal ? " -q" : " -q -batch -ex run -ex bt") + " --args " + shellQuote(exe);
        for (const auto& a : progArgs) runCmd += " " + shellQuote(a);
        cout << "[Ghost Debugger] " << (terminal ? "Iniciando sessao gdb conectada ao codigo .g..." : "Sem terminal: gdb em lote (run + bt)") << endl;
        const int grc = runCommand(runCmd);
        return terminal ? grc : (grc == 0 ? 0 : 1);
    } else {
        cout << "[Ghost Debugger] gdb nao detectado no PATH." << endl;
        cout << "[Ghost Debugger] Mapeamento DWARF de linhas ativado via #line:" << endl;
        cout << "  Executavel com simbolos: " << exe << endl;
        cout << "  Para depurar no GDB / VSCode:" << endl;
        cout << "    gdb " << exe << endl;
        cout << "    (gdb) break " << inputFile << ":1" << endl;
        cout << "    (gdb) run" << endl;
        string cmd = shellQuote(exe);
        for (const auto& a : progArgs) cmd += " " + shellQuote(a);
        return runCommand(cmd);
    }
}

int main(int argc, char* argv[]) {
    // Sem argumentos (ou "ghost repl"): REPL interativo
    if (argc == 1 || string(argv[1]) == "repl") {
        GhostInterpreter interpreter;
        interpreter.repl();
        return 0;
    }
    const string cmd = argv[1];
    if (cmd == "help" || cmd == "--help" || cmd == "-h") { printHelp(); return 0; }
    if (cmd == "version" || cmd == "--version") { cout << GHOST_VERSAO << endl; return 0; }
    if (cmd == "run") return cmdRun(argc, argv);
    if (cmd == "build") return cmdBuild(argc, argv, 2);
    if (cmd == "check") return cmdCheck(argc, argv);
    if (cmd == "ast") return cmdAst(argc, argv);
    if (cmd == "cpp") return cmdCpp(argc, argv);
    if (cmd == "test") return cmdTest(argc, argv);
    if (cmd == "fmt") return cmdFmt(argc, argv);
    if (cmd == "lsp") return cmdLsp(argc, argv);
    if (cmd == "debug") return cmdDebug(argc, argv);
    if (cmd == "init") return GhostPM::PackageManager::cmdInit(argc >= 3 ? argv[2] : "");
    if (cmd == "pm") return GhostPM::PackageManager::dispatch(argc, argv);
    return cmdBuild(argc, argv, 1); // uso antigo: ghost [-x|-d|--wasm] arquivo.g [-o saida]
}
