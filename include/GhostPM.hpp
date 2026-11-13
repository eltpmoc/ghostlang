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
    std::map<std::string, std::string> dependencies;
};

class PackageManager {
public:
    static int cmdInit(const std::string& projectName = "");
    static int cmdAdd(const std::string& pkgName, const std::string& version = "1.0.0");
    static int cmdRemove(const std::string& pkgName);
    static int cmdInstall();
    static int cmdList();
    static int dispatch(int argc, char* argv[]);

    static bool loadToml(const std::string& path, PackageInfo& info);
    static bool saveToml(const std::string& path, const PackageInfo& info);
    static bool saveLock(const std::string& path, const PackageInfo& info);
};

} // namespace GhostPM

#endif // GHOST_PM_HPP
