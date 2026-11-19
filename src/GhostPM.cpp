#include "../include/GhostPM.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace fs = std::filesystem;

namespace GhostPM {

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

static std::string stripQuotes(const std::string& str) {
    std::string s = trim(str);
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

bool PackageManager::loadToml(const std::string& path, PackageInfo& info) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    std::string currentSection;
    while (std::getline(file, line)) {
        std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;

        if (t.front() == '[' && t.back() == ']') {
            currentSection = t.substr(1, t.size() - 2);
            continue;
        }

        size_t eq = t.find('=');
        if (eq != std::string::npos) {
            std::string key = trim(t.substr(0, eq));
            std::string val = stripQuotes(t.substr(eq + 1));

            if (currentSection == "package") {
                if (key == "name") info.name = val;
                else if (key == "version") info.version = val;
                else if (key == "description") info.description = val;
                else if (key == "author" || key == "authors") info.author = val;
                else if (key == "entry") info.entry = val;
            } else if (currentSection == "dependencies") {
                info.dependencies[key] = val;
            }
        }
    }
    return true;
}

bool PackageManager::saveToml(const std::string& path, const PackageInfo& info) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "[package]\n";
    file << "name = \"" << info.name << "\"\n";
    file << "version = \"" << info.version << "\"\n";
    file << "description = \"" << info.description << "\"\n";
    file << "author = \"" << info.author << "\"\n";
    file << "entry = \"" << info.entry << "\"\n\n";

    file << "[dependencies]\n";
    for (const auto& pair : info.dependencies) {
        file << pair.first << " = \"" << pair.second << "\"\n";
    }
    return true;
}

bool PackageManager::saveLock(const std::string& path, const PackageInfo& info,
                              const std::vector<ResolvedDep>& resolved) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "# Gerado automaticamente por GhostPM (GhostLang Package Manager).\n";
    file << "# NAO edite manualmente este arquivo.\n";
    file << "lock_version = 2\n\n";

    file << "[[package]]\n";
    file << "name = \"" << info.name << "\"\n";
    file << "version = \"" << info.version << "\"\n\n";

    for (const auto& pair : info.dependencies) {
        const ResolvedDep* r = nullptr;
        for (const auto& d : resolved) if (d.name == pair.first) r = &d;
        file << "[[package]]\n";
        file << "name = \"" << pair.first << "\"\n";
        file << "spec = \"" << pair.second << "\"\n";
        if (r) {
            file << "version = \"" << r->version << "\"\n";
            file << "source = \"" << r->source << "\"\n";
            if (!r->commit.empty()) file << "commit = \"" << r->commit << "\"\n";
        } else {
            file << "source = \"pendente (rode ghost pm install)\"\n";
        }
        file << "\n";
    }
    return true;
}

int PackageManager::cmdInit(const std::string& projectName) {
    std::string name = projectName;
    if (name.empty()) {
        std::error_code ec;
        name = fs::current_path(ec).filename().string();
        if (name.empty()) name = "meu_projeto";
    }

    if (fs::exists("ghost.toml")) {
        std::cerr << "[GhostPM] Arquivo ghost.toml ja existe neste diretorio." << std::endl;
        return 1;
    }

    PackageInfo info;
    info.name = name;
    info.version = "0.1.0";
    info.description = "Projeto GhostLang";
    info.author = "GhostLang Dev";
    info.entry = "src/main.g";

    std::error_code ec;
    fs::create_directories("src", ec);
    if (!fs::exists("src/main.g")) {
        std::ofstream mainFile("src/main.g");
        if (mainFile.is_open()) {
            mainFile << "// Projeto GhostLang: " << name << "\n";
            mainFile << "function main() {\n";
            mainFile << "    print(\"Ola do projeto " << name << " em GhostLang!\");\n";
            mainFile << "}\n";
        }
    }

    if (!saveToml("ghost.toml", info)) {
        std::cerr << "[GhostPM] Erro ao gravar ghost.toml." << std::endl;
        return 1;
    }

    if (!saveLock("ghost.lock", info)) {
        std::cerr << "[GhostPM] Erro ao gravar ghost.lock." << std::endl;
        return 1;
    }

    std::cout << "[GhostPM] Projeto '" << name << "' inicializado com sucesso!" << std::endl;
    std::cout << "  - ghost.toml criado" << std::endl;
    std::cout << "  - ghost.lock gerado" << std::endl;
    std::cout << "  - src/main.g pronto" << std::endl;
    return 0;
}

int PackageManager::cmdAdd(const std::string& pkgName, const std::string& version) {
    if (!fs::exists("ghost.toml")) {
        std::cerr << "[GhostPM] Erro: ghost.toml nao encontrado. Execute 'ghost pm init' antes." << std::endl;
        return 1;
    }

    PackageInfo info;
    if (!loadToml("ghost.toml", info)) {
        std::cerr << "[GhostPM] Erro ao carregar ghost.toml existente." << std::endl;
        return 1;
    }

    info.dependencies[pkgName] = version;
    if (!saveToml("ghost.toml", info) || !saveLock("ghost.lock", info)) {
        std::cerr << "[GhostPM] Falha ao atualizar ghost.toml / ghost.lock." << std::endl;
        return 1;
    }

    std::cout << "[GhostPM] Dependencia '" << pkgName << "' (" << version << ") adicionada com sucesso!" << std::endl;
    return 0;
}

int PackageManager::cmdRemove(const std::string& pkgName) {
    if (!fs::exists("ghost.toml")) {
        std::cerr << "[GhostPM] Erro: ghost.toml nao encontrado." << std::endl;
        return 1;
    }

    PackageInfo info;
    if (!loadToml("ghost.toml", info)) {
        std::cerr << "[GhostPM] Erro ao carregar ghost.toml." << std::endl;
        return 1;
    }

    auto it = info.dependencies.find(pkgName);
    if (it == info.dependencies.end()) {
        std::cerr << "[GhostPM] Pacote '" << pkgName << "' nao consta nas dependencias." << std::endl;
        return 1;
    }

    info.dependencies.erase(it);
    if (!saveToml("ghost.toml", info) || !saveLock("ghost.lock", info)) {
        std::cerr << "[GhostPM] Falha ao atualizar ghost.toml / ghost.lock." << std::endl;
        return 1;
    }

    std::cout << "[GhostPM] Dependencia '" << pkgName << "' removida com sucesso." << std::endl;
    return 0;
}

int PackageManager::cmdList() {
    if (!fs::exists("ghost.toml")) {
        std::cerr << "[GhostPM] Nenhum ghost.toml encontrado no diretorio atual." << std::endl;
        return 1;
    }

    PackageInfo info;
    if (!loadToml("ghost.toml", info)) {
        std::cerr << "[GhostPM] Erro ao ler ghost.toml." << std::endl;
        return 1;
    }

    std::cout << "=========================================" << std::endl;
    std::cout << " Pacote: " << info.name << " v" << info.version << std::endl;
    std::cout << " Autor:  " << info.author << std::endl;
    std::cout << " Entrada: " << info.entry << std::endl;
    std::cout << " Descricao: " << info.description << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << " Dependencias (" << info.dependencies.size() << "):" << std::endl;
    if (info.dependencies.empty()) {
        std::cout << "  (nenhuma dependencia declarada)" << std::endl;
    } else {
        for (const auto& pair : info.dependencies) {
            std::cout << "  - " << pair.first << " : " << pair.second << std::endl;
        }
    }
    return 0;
}

namespace {
std::vector<int> versionParts(const std::string& v) {
    std::vector<int> out;
    std::stringstream ss(v);
    std::string part;
    while (std::getline(ss, part, '.')) {
        try { out.push_back(std::stoi(part)); } catch (...) { out.push_back(0); }
    }
    while (out.size() < 3) out.push_back(0);
    return out;
}

// Só caracteres seguros: a URL e a ref vão para a linha de comando do git
bool safeForCommand(const std::string& s, bool url) {
    if (s.empty() || s.size() > 512 || s[0] == '-') return false;
    for (unsigned char c : s) {
        if (std::isalnum(c)) continue;
        if (std::string("._-/").find(static_cast<char>(c)) != std::string::npos) continue;
        if (url && std::string(":@~+%").find(static_cast<char>(c)) != std::string::npos) continue;
        return false;
    }
    return true;
}

std::string readVersion(const fs::path& dir) {
    PackageInfo pkg;
    if (fs::exists(dir / "ghost.toml") && PackageManager::loadToml((dir / "ghost.toml").string(), pkg)) return pkg.version;
    return "";
}

std::string runCapture(const std::string& cmd) {
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) return "";
    std::string out;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) out += buf;
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}
} // namespace

// ---------------------------------------------------------------- indice de pacotes
namespace {
std::string g_root;

// Leitor JSON minimo para o indice: objeto de objetos com valores texto (outros valores
// sao lidos e ignorados).
struct MiniJson {
    const std::string& s;
    size_t i = 0;
    std::string erro;
    explicit MiniJson(const std::string& t) : s(t) {}
    void ws() { while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i; }
    bool falha(const std::string& m) { if (erro.empty()) erro = m + " (posicao " + std::to_string(i) + ")"; return false; }
    bool str(std::string& out) {
        ws();
        if (i >= s.size() || s[i] != '"') return falha("esperava texto");
        ++i;
        out.clear();
        while (i < s.size() && s[i] != '"') {
            char c = s[i++];
            if (c == '\\' && i < s.size()) {
                char e = s[i++];
                switch (e) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case 'u': out += '?'; i = std::min(s.size(), i + 4); break;
                    default: out += e;
                }
            } else {
                out += c;
            }
        }
        if (i >= s.size()) return falha("texto sem fim");
        ++i;
        return true;
    }
    bool skip() {  // pula um valor qualquer
        ws();
        if (i >= s.size()) return falha("fim inesperado");
        if (s[i] == '"') { std::string t; return str(t); }
        if (s[i] == '{' || s[i] == '[') {
            const char fecha = s[i] == '{' ? '}' : ']';
            const bool obj = s[i] == '{';
            ++i; ws();
            if (i < s.size() && s[i] == fecha) { ++i; return true; }
            while (true) {
                if (obj) { std::string k; if (!str(k)) return false; ws(); if (i >= s.size() || s[i] != ':') return falha("esperava ':'"); ++i; }
                if (!skip()) return false;
                ws();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == fecha) { ++i; return true; }
                return falha("esperava ',' ou fechamento");
            }
        }
        while (i < s.size() && (std::isalnum(static_cast<unsigned char>(s[i])) || s[i] == '.' || s[i] == '-' || s[i] == '+')) ++i;
        return true;
    }
    // objeto de texto -> valor (texto, ou objeto de campos texto)
    bool objeto(std::map<std::string, std::map<std::string, std::string>>& out) {
        ws();
        if (i >= s.size() || s[i] != '{') return falha("o indice deve ser um objeto JSON");
        ++i; ws();
        if (i < s.size() && s[i] == '}') { ++i; return true; }
        while (true) {
            std::string nome;
            if (!str(nome)) return false;
            ws();
            if (i >= s.size() || s[i] != ':') return falha("esperava ':'");
            ++i; ws();
            if (i < s.size() && s[i] == '{') {
                ++i; ws();
                auto& campos = out[nome];
                if (i < s.size() && s[i] == '}') { ++i; }
                else while (true) {
                    std::string k;
                    if (!str(k)) return false;
                    ws();
                    if (i >= s.size() || s[i] != ':') return falha("esperava ':'");
                    ++i; ws();
                    if (i < s.size() && s[i] == '"') { std::string v; if (!str(v)) return false; campos[k] = v; }
                    else if (!skip()) return false;
                    ws();
                    if (i < s.size() && s[i] == ',') { ++i; continue; }
                    if (i < s.size() && s[i] == '}') { ++i; break; }
                    return falha("esperava ',' ou '}'");
                }
            } else if (!skip()) {
                return false;
            }
            ws();
            if (i < s.size() && s[i] == ',') { ++i; continue; }
            if (i < s.size() && s[i] == '}') { ++i; return true; }
            return falha("esperava ',' ou '}'");
        }
    }
};
} // namespace

std::string IndexEntry::spec() const {
    if (!embutido.empty()) return "embutido:" + embutido;
    return "git:" + git + (ref.empty() ? "" : "#" + ref);
}

void PackageManager::setRoot(const std::string& root) { g_root = root; }

std::map<std::string, IndexEntry> PackageManager::loadIndex(std::string* origem, std::string* erro) {
    std::map<std::string, IndexEntry> idx;
    fs::path p;
    if (const char* env = std::getenv("GHOST_PM_INDICE"); env && *env) p = env;
    else p = fs::path(g_root.empty() ? "." : g_root) / "pacotes" / "indice.json";
    if (origem) *origem = p.generic_string();
    std::ifstream f(p, std::ios::binary);
    if (!f) { if (erro) *erro = "indice " + p.generic_string() + " nao encontrado"; return idx; }
    std::stringstream ss;
    ss << f.rdbuf();
    const std::string texto = ss.str();
    MiniJson js(texto);
    std::map<std::string, std::map<std::string, std::string>> bruto;
    if (!js.objeto(bruto)) { if (erro) *erro = "indice " + p.generic_string() + " invalido: " + js.erro; return {}; }
    for (auto& [nome, c] : bruto) {
        if (nome.empty() || nome[0] == '_') continue;
        IndexEntry e;
        e.name = nome;
        e.embutido = c["embutido"];
        e.git = c["git"];
        e.ref = c["ref"];
        e.versao = c["versao"];
        e.descricao = c["descricao"];
        if (e.embutido.empty() && e.git.empty()) continue;
        idx[nome] = e;
    }
    return idx;
}

int PackageManager::cmdSearch(const std::string& termoIn) {
    std::string origem, erro;
    const auto idx = loadIndex(&origem, &erro);
    if (!erro.empty()) { std::cerr << "[GhostPM] " << erro << std::endl; return 1; }
    auto baixa = [](std::string t) { for (auto& c : t) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return t; };
    const std::string termo = baixa(termoIn);
    size_t n = 0;
    for (const auto& [nome, e] : idx) {
        if (!termo.empty() && baixa(nome).find(termo) == std::string::npos && baixa(e.descricao).find(termo) == std::string::npos) continue;
        std::string nomeCol = nome;
        if (nomeCol.size() < 16) nomeCol.resize(16, ' ');
        std::cout << "  " << nomeCol << " " << (e.embutido.empty() ? "git     " : "embutido") << "  " << e.descricao << std::endl;
        ++n;
    }
    std::cout << "[GhostPM] " << n << " pacote(s)" << (termo.empty() ? "" : " para '" + termoIn + "'") << " no indice " << origem << std::endl;
    return n ? 0 : 1;
}

int PackageManager::cmdInstallNames(const std::vector<std::string>& names) {
    std::string erro;
    const auto idx = loadIndex(nullptr, &erro);
    // resolve tudo antes de mexer no projeto
    std::vector<std::pair<std::string, std::string>> deps;
    for (const auto& n : names) {
        auto it = idx.find(n);
        if (it == idx.end()) {
            std::cerr << "[GhostPM] Pacote '" << n << "' nao esta no indice" << (erro.empty() ? "" : " (" + erro + ")")
                      << ". Veja 'ghost pm search' ou use 'ghost pm add " << n << " --path <pasta>' / '--git <url>'." << std::endl;
            return 1;
        }
        deps.emplace_back(n, it->second.spec());
    }
    if (!fs::exists("ghost.toml")) {
        std::cout << "[GhostPM] Sem ghost.toml aqui: criando o projeto." << std::endl;
        if (cmdInit() != 0) return 1;
    }
    for (const auto& [n, spec] : deps)
        if (cmdAdd(n, spec) != 0) return 1;
    return cmdInstall();
}

bool PackageManager::versionSatisfies(const std::string& version, const std::string& specIn) {
    std::string spec = trim(specIn);
    if (spec.empty() || spec == "*") return true;
    if (version.empty()) return false;
    const auto v = versionParts(version);
    auto cmp = [&](const std::vector<int>& w) {
        for (int i = 0; i < 3; ++i) if (v[i] != w[i]) return v[i] < w[i] ? -1 : 1;
        return 0;
    };
    if (spec.rfind(">=", 0) == 0) return cmp(versionParts(spec.substr(2))) >= 0;
    if (spec[0] == '^') {
        const auto w = versionParts(spec.substr(1));
        return v[0] == w[0] && cmp(w) >= 0;
    }
    if (spec[0] == '~') {
        const auto w = versionParts(spec.substr(1));
        return v[0] == w[0] && v[1] == w[1] && cmp(w) >= 0;
    }
    return cmp(versionParts(spec)) == 0;
}

int PackageManager::cmdInstall() {
    if (!fs::exists("ghost.toml")) {
        std::cerr << "[GhostPM] Erro: ghost.toml nao encontrado para instalacao." << std::endl;
        return 1;
    }
    PackageInfo info;
    if (!loadToml("ghost.toml", info)) {
        std::cerr << "[GhostPM] Erro ao carregar ghost.toml." << std::endl;
        return 1;
    }
    std::error_code ec;
    fs::create_directories("ghost_packages", ec);

    // Sem registro remoto: cada dependencia vem de uma pasta (path:), de um repositorio
    // git (git:url#ref) ou ja precisa estar em ghost_packages/<nome>/.
    std::cout << "[GhostPM] Instalando dependencias de '" << info.name << "'..." << std::endl;
    std::vector<ResolvedDep> resolved;
    size_t falhas = 0;
    for (const auto& pair : info.dependencies) {
        const std::string& name = pair.first;
        const std::string& spec = pair.second;
        const fs::path dest = fs::path("ghost_packages") / name;
        ResolvedDep r{name, spec, "", "", ""};
        auto falha = [&](const std::string& msg) {
            ++falhas;
            std::cerr << "  [falha] " << name << ": " << msg << std::endl;
        };
        if (!safeForCommand(name, false)) { falha("nome de pacote invalido"); continue; }

        if (spec.rfind("path:", 0) == 0) {
            const fs::path src = spec.substr(5);
            if (!fs::is_directory(src, ec)) { falha("pasta " + src.generic_string() + " nao existe"); continue; }
            fs::remove_all(dest, ec);
            fs::create_directories(dest, ec);
            fs::copy(src, dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
            if (ec) { falha("copia falhou: " + ec.message()); continue; }
            r.source = spec;
        } else if (spec.rfind("git:", 0) == 0) {
            std::string url = spec.substr(4), ref;
            const size_t hash = url.find('#');
            if (hash != std::string::npos) { ref = url.substr(hash + 1); url = url.substr(0, hash); }
            if (!safeForCommand(url, true) || (!ref.empty() && !safeForCommand(ref, false))) {
                falha("URL ou ref git com caracteres nao permitidos");
                continue;
            }
            fs::remove_all(dest, ec);
            std::string cmd = "git clone --quiet --depth 1 " + std::string(ref.empty() ? "" : "--branch " + ref + " ") +
                              "\"" + url + "\" \"" + dest.generic_string() + "\"";
            if (std::system(cmd.c_str()) != 0 || !fs::is_directory(dest, ec)) { falha("git clone falhou (" + url + ")"); continue; }
            r.commit = runCapture("git -C \"" + dest.generic_string() + "\" rev-parse HEAD");
            fs::remove_all(dest / ".git", ec);
            r.source = "git:" + url + (ref.empty() ? "" : "#" + ref);
        } else if (spec.rfind("embutido:", 0) == 0) {
            // biblioteca que ja vem com a GhostLang: nada a copiar, o import acha <raiz>/libs/x.g
            const std::string rel = spec.substr(9);
            const fs::path arq = fs::path(g_root.empty() ? "." : g_root) / rel;
            if (rel.find("..") != std::string::npos || !fs::is_regular_file(arq, ec)) {
                falha("biblioteca embutida " + rel + " nao encontrada na instalacao da GhostLang");
                continue;
            }
            const auto idx = loadIndex();
            auto it = idx.find(name);
            r.source = spec;
            r.version = it != idx.end() ? it->second.versao : "";
            std::cout << "  [ok] " << name << " " << (r.version.empty() ? "(sem versao)" : r.version)
                      << " <- embutido (ja vem com a GhostLang); use: import \"" << rel << "\"" << std::endl;
            resolved.push_back(r);
            continue;
        } else {
            if (!fs::is_directory(dest, ec)) {
                const auto idx = loadIndex();
                if (idx.count(name))
                    falha("nao ha registro remoto da versao '" + spec + "'; '" + name + "' esta no indice: use 'ghost install " + name + "' (ou 'ghost pm add " + name + "' sem versao)");
                else
                    falha("nao ha registro remoto; use path:<pasta> ou git:<url>, ou copie o pacote para " + dest.generic_string() + "/");
                continue;
            }
            r.source = "local";
        }
        r.version = readVersion(dest);
        // versao exigida: a da especificacao simples, ou "#versao" nao se aplica a path/git
        const bool specIsVersion = spec.rfind("path:", 0) != 0 && spec.rfind("git:", 0) != 0 && spec.rfind("embutido:", 0) != 0;
        if (specIsVersion && !versionSatisfies(r.version, spec)) {
            falha("versao instalada '" + (r.version.empty() ? std::string("desconhecida") : r.version) + "' nao atende '" + spec + "'");
            continue;
        }
        std::cout << "  [ok] " << name << " " << (r.version.empty() ? "(sem versao)" : r.version) << " <- " << r.source
                  << (r.commit.empty() ? "" : " @ " + r.commit.substr(0, 12)) << std::endl;
        resolved.push_back(r);
    }
    if (falhas) {
        std::cerr << "[GhostPM] " << falhas << " dependencia(s) com problema; ghost.lock nao foi atualizado." << std::endl;
        return 1;
    }
    saveLock("ghost.lock", info, resolved);
    std::cout << "[GhostPM] " << resolved.size() << " dependencia(s) instalada(s) em ghost_packages/ e travada(s) em ghost.lock." << std::endl;
    return 0;
}

int PackageManager::dispatch(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "GhostPM - Gerenciador de Pacotes GhostLang:" << std::endl;
        std::cout << "  ghost install <pacote>...      Instala pelo indice (ex.: ghost install sqlite); cria o ghost.toml se faltar" << std::endl;
        std::cout << "  ghost pm search [termo]        Lista os pacotes do indice (pacotes/indice.json ou GHOST_PM_INDICE)" << std::endl;
        std::cout << "  ghost pm init [nome]           Inicializa um novo pacote ghost.toml" << std::endl;
        std::cout << "  ghost pm add <pacote>          Pacote do indice (embutido ou git)" << std::endl;
        std::cout << "  ghost pm add <pacote> <versao>  Dependencia ja em ghost_packages/ (versao: 1.2.0, ^1.2, ~1.2.3, >=1.0)" << std::endl;
        std::cout << "  ghost pm add <pacote> --path <pasta> | --git <url> [--ref <tag>]" << std::endl;
        std::cout << "  ghost pm remove <pacote>       Remove uma dependencia do projeto" << std::endl;
        std::cout << "  ghost pm install [pacote...]   Copia (path:) ou clona (git:) as dependencias; com nomes, igual a ghost install" << std::endl;
        std::cout << "  ghost pm list                  Exibe metadados do pacote e dependencias" << std::endl;
        return 0;
    }

    std::string sub = argv[2];
    if (sub == "init") {
        std::string name = (argc >= 4) ? argv[3] : "";
        return cmdInit(name);
    } else if (sub == "add") {
        if (argc < 4) {
            std::cerr << "Uso: ghost pm add <pacote> [versao | --path <pasta> | --git <url> [--ref <tag>]]" << std::endl;
            return 1;
        }
        std::string pkg = argv[3];
        std::string ver = "*";
        if (argc >= 5) {
            ver = argv[4];
        } else {
            const auto idx = loadIndex();
            auto it = idx.find(pkg);
            if (it != idx.end()) ver = it->second.spec();
        }
        if (argc >= 6 && ver == "--path") ver = std::string("path:") + argv[5];
        else if (argc >= 6 && ver == "--git") ver = std::string("git:") + argv[5] + (argc >= 8 && std::string(argv[6]) == "--ref" ? std::string("#") + argv[7] : std::string());
        return cmdAdd(pkg, ver);
    } else if (sub == "remove" || sub == "rm") {
        if (argc < 4) {
            std::cerr << "Uso: ghost pm remove <pacote>" << std::endl;
            return 1;
        }
        return cmdRemove(argv[3]);
    } else if (sub == "install" || sub == "i") {
        if (argc >= 4) return cmdInstallNames(std::vector<std::string>(argv + 3, argv + argc));
        return cmdInstall();
    } else if (sub == "search" || sub == "s") {
        return cmdSearch(argc >= 4 ? argv[3] : "");
    } else if (sub == "list" || sub == "ls") {
        return cmdList();
    } else {
        std::cerr << "[GhostPM] Subcomando desconhecido: " << sub << std::endl;
        return 1;
    }
}

} // namespace GhostPM
