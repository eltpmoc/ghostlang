#ifndef GHOST_PM_HPP
#define GHOST_PM_HPP

#include <string>
#include <vector>
#include <map>

namespace GhostPM {

struct PackageInfo {
    std::string name = "app";
    std::string version = "0.1.0";
    std::string description = "Projeto GhostLang";
    std::string author = "GhostLang Dev";
    std::string entry = "src/main.g";
    // nome -> especificação: "1.2.0", "^1.2", "~1.2.3", ">=1.0", "*", "path:<pasta>" ou "git:<url>[#ref]"
    std::map<std::string, std::string> dependencies;
};

// Entrada do indice de pacotes (pacotes/indice.json): biblioteca embutida (libs/x.g)
// ou repositorio git de terceiros
struct IndexEntry {
    std::string name, embutido, git, ref, versao, descricao;
    std::string spec() const;  // "embutido:libs/db.g" ou "git:<url>[#ref]"
};

// Resultado da instalação de uma dependência (vai para o ghost.lock)
struct ResolvedDep {
    std::string name, spec, source, version, commit;
};

class PackageManager {
public:
    static int cmdInit(const std::string& projectName = "");
    static int cmdAdd(const std::string& pkgName, const std::string& version = "1.0.0");
    static int cmdRemove(const std::string& pkgName);
    static int cmdInstall();
    static int cmdList();
    static int cmdSearch(const std::string& termo = "");
    // ghost install <nome>...: cria o ghost.toml se faltar, resolve cada nome pelo indice e instala
    static int cmdInstallNames(const std::vector<std::string>& names);
    static int dispatch(int argc, char* argv[]);

    // Raiz da GhostLang (onde ficam libs/ e pacotes/); o indice vem de GHOST_PM_INDICE ou
    // de <raiz>/pacotes/indice.json
    static void setRoot(const std::string& root);
    static std::map<std::string, IndexEntry> loadIndex(std::string* origem = nullptr, std::string* erro = nullptr);

    static bool loadToml(const std::string& path, PackageInfo& info);
    static bool saveToml(const std::string& path, const PackageInfo& info);
    static bool saveLock(const std::string& path, const PackageInfo& info,
                         const std::vector<ResolvedDep>& resolved = {});
    // Semver: "1.2.0" exato, "^1.2" (mesmo maior), "~1.2.3" (mesmo menor), ">=1.0", "*"
    static bool versionSatisfies(const std::string& version, const std::string& spec);
};

} // namespace GhostPM

#endif // GHOST_PM_HPP
