#include "../include/GhostPM.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

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

bool PackageManager::saveLock(const std::string& path, const PackageInfo& info) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "# Gerado automaticamente por GhostPM (GhostLang Package Manager).\n";
    file << "# NAO edite manualmente este arquivo.\n";
    file << "lock_version = 1\n\n";

    file << "[[package]]\n";
    file << "name = \"" << info.name << "\"\n";
    file << "version = \"" << info.version << "\"\n\n";

    for (const auto& pair : info.dependencies) {
        file << "[[package]]\n";
        file << "name = \"" << pair.first << "\"\n";
        file << "version = \"" << pair.second << "\"\n";
        file << "source = \"ghost-registry\"\n\n";
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
            mainFile << "fun main() {\n";
            mainFile << "    escrevaln(\"Ola do projeto " << name << " em GhostLang!\");\n";
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

    // Ainda nao ha registro remoto de pacotes: ate 12/11/2026 este comando dizia
    // "[OK] Resolvido" para qualquer nome sem baixar nada. Agora so aceita
    // dependencias que ja estao em ghost_packages/<nome>/ e avisa das que faltam.
    std::cout << "[GhostPM] Conferindo dependencias de '" << info.name << "' em ghost_packages/ (nao ha registro remoto)..." << std::endl;
    size_t missing = 0;
    for (const auto& pair : info.dependencies) {
        const fs::path dir = fs::path("ghost_packages") / pair.first;
        if (fs::is_directory(dir, ec)) {
            std::cout << "  [local] " << pair.first << "@" << pair.second << " em " << dir.generic_string() << std::endl;
        } else {
            ++missing;
            std::cerr << "  [falta] " << pair.first << "@" << pair.second
                      << ": copie o pacote para " << dir.generic_string() << "/" << std::endl;
        }
    }
    if (missing) {
        std::cerr << "[GhostPM] " << missing << " dependencia(s) faltando; ghost.lock nao foi atualizado." << std::endl;
        return 1;
    }
    saveLock("ghost.lock", info);
    std::cout << "[GhostPM] " << info.dependencies.size() << " dependencia(s) encontrada(s) em ghost_packages/ e travada(s) em ghost.lock." << std::endl;
    return 0;
}

int PackageManager::dispatch(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "GhostPM - Gerenciador de Pacotes GhostLang:" << std::endl;
        std::cout << "  ghost pm init [nome]           Inicializa um novo pacote ghost.toml" << std::endl;
        std::cout << "  ghost pm add <pacote> [versao] Adiciona uma dependencia e atualiza ghost.lock" << std::endl;
        std::cout << "  ghost pm remove <pacote>       Remove uma dependencia do projeto" << std::endl;
        std::cout << "  ghost pm install               Confere as dependencias em ghost_packages/ (sem registro remoto)" << std::endl;
        std::cout << "  ghost pm list                  Exibe metadados do pacote e dependencias" << std::endl;
        return 0;
    }

    std::string sub = argv[2];
    if (sub == "init") {
        std::string name = (argc >= 4) ? argv[3] : "";
        return cmdInit(name);
    } else if (sub == "add") {
        if (argc < 4) {
            std::cerr << "Uso: ghost pm add <pacote> [versao]" << std::endl;
            return 1;
        }
        std::string pkg = argv[3];
        std::string ver = (argc >= 5) ? argv[4] : "1.0.0";
        return cmdAdd(pkg, ver);
    } else if (sub == "remove" || sub == "rm") {
        if (argc < 4) {
            std::cerr << "Uso: ghost pm remove <pacote>" << std::endl;
            return 1;
        }
        return cmdRemove(argv[3]);
    } else if (sub == "install" || sub == "i") {
        return cmdInstall();
    } else if (sub == "list" || sub == "ls") {
        return cmdList();
    } else {
        std::cerr << "[GhostPM] Subcomando desconhecido: " << sub << std::endl;
        return 1;
    }
}

} // namespace GhostPM
