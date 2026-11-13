#include <iostream>
#include <string>
#include <stdio.h>

int main() {
    std::string exe = "test_err.exe";
    std::string cmd = "\"\"" + exe + "\" 2>nul\"";
    std::cout << "Running: " << cmd << std::endl;
    FILE* fp = _popen(cmd.c_str(), "r");
    if (!fp) {
        std::cout << "Failed to run." << std::endl;
        return 1;
    }
    char buf[128];
    while (fgets(buf, 128, fp) != NULL) {
        std::cout << "OUTPUT: " << buf;
    }
    _pclose(fp);
    return 0;
}
