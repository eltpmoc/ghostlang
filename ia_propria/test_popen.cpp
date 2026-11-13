#include <iostream>
#include <string>
#include <stdio.h>

int main() {
    std::string url = "https://pt.wikipedia.org/w/api.php?action=query&list=search&srlimit=1&format=xml&srsearch=napoleao%20bonaparte";
    std::string cmd = "echo_args.exe -q --silent --fail --proto =https --connect-timeout 4 --max-time 10 --max-filesize 2097152 --user-agent \"GhostLangLocalAI/3.3\" --write-out \"\\nGHOST_HTTP_CODE:%{http_code}\" \"" + url + "\"";
    std::cout << "Running: " << cmd << std::endl;
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to popen" << std::endl;
        return 1;
    }
    char chunk[8192];
    std::string body;
    while (fgets(chunk, sizeof(chunk), pipe) != nullptr) {
        body += chunk;
    }
    int status = _pclose(pipe);
    std::cout << "Status: " << status << std::endl;
    std::cout << "Body: " << body << std::endl;
    return 0;
}
