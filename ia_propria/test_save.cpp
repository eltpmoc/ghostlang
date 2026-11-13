#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

int main() {
    std::string dir = "v3/memoria";
    try {
        fs::create_directories(dir);
        auto temp = (fs::path(dir) / (".record-temp-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))).string();
        std::cout << "Path: " << temp << std::endl;
        std::ofstream out(temp, std::ios::binary);
        if (!out) {
            std::cerr << "Failed to create file!" << std::endl;
            return 1;
        }
        std::cout << "Success!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
