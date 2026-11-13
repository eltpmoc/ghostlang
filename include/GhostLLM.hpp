#ifndef GHOST_LLM_HPP
#define GHOST_LLM_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <memory>

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
#else
    #include <unistd.h>
    #include <limits.h>
#endif

// Classe que encapsula o modelo LLAMA GGUF
class GhostLLM {
private:
    std::string modelPath;

    std::string getCompilerDir() {
#ifdef _WIN32
        char path[MAX_PATH];
        HMODULE hModule = GetModuleHandle(NULL);
        if (hModule != NULL) {
            GetModuleFileNameA(hModule, path, sizeof(path));
            std::string fullPath(path);
            size_t lastSlash = fullPath.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                return fullPath.substr(0, lastSlash);
            }
        }
#else
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        if (count != -1) {
            std::string fullPath(result, count);
            size_t lastSlash = fullPath.find_last_of('/');
            if (lastSlash != std::string::npos) {
                return fullPath.substr(0, lastSlash);
            }
        }
#endif
        return ".";
    }

public:
    GhostLLM() : modelPath("") {}
    GhostLLM(const std::string& path) : modelPath(path) {}

    std::string prompt(const std::string& inputPrompt) {
        if (modelPath.empty()) {
            std::cerr << "[GhostLLM] Erro: Modelo GGUF nao foi carregado." << std::endl;
            return "";
        }

        std::string exeDir = getCompilerDir();
        std::string llamaCmd;
        std::string promptFile = ".p_prompt.tmp";
        FILE* outfile = fopen(promptFile.c_str(), "w");
        if (outfile) {
            fwrite(inputPrompt.c_str(), 1, inputPrompt.length(), outfile);
            fclose(outfile);
        }

#ifdef _WIN32
        llamaCmd = "\"" + exeDir + "\\compiler\\llama-cli.exe\"";
        // Check if exists, if not try parent dir
        if (GetFileAttributesA((exeDir + "\\compiler\\llama-cli.exe").c_str()) == INVALID_FILE_ATTRIBUTES) {
            llamaCmd = "\"" + exeDir + "\\..\\compiler\\llama-cli.exe\"";
        }
        std::string fullCmd = "\"\"" + llamaCmd + "\" -m \"" + modelPath + "\" -f \"" + promptFile + "\" -c 4096 -n 512 --log-disable --single-turn 2>nul\"";
#else
        llamaCmd = "\"" + exeDir + "/compiler/llama-cli\"";
        if (access((exeDir + "/compiler/llama-cli").c_str(), F_OK) != 0) {
            llamaCmd = "\"" + exeDir + "/../compiler/llama-cli\"";
        }
        std::string fullCmd = llamaCmd + " -m \"" + modelPath + "\" -f \"" + promptFile + "\" -c 4096 -n 512 --log-disable --single-turn 2>/dev/null";
#endif

        std::string output;
        
#ifdef _WIN32
        FILE* rawPipe = _popen(fullCmd.c_str(), "r");
        std::unique_ptr<FILE, decltype(&_pclose)> pipe(rawPipe, _pclose);
#else
        FILE* rawPipe = popen(fullCmd.c_str(), "r");
        std::unique_ptr<FILE, decltype(&pclose)> pipe(rawPipe, pclose);
#endif
        
        if (!pipe) {
            std::cerr << "[GhostLLM] Erro: Falha ao iniciar llama-cli. Verifique se ele esta instalado em compiler/." << std::endl;
            return "";
        }
        
        int c;
        while ((c = fgetc(pipe.get())) != EOF) {
            char ch = static_cast<char>(c);
            output += ch;
        }
        
        // Extrai apenas a resposta da IA do output interativo do llama-cli
        // O output contem o prompt precedido por "> " e a resposta logo abaixo
        std::string marker = "> " + inputPrompt;
        size_t pos = output.find(marker);
        if (pos != std::string::npos) {
            output = output.substr(pos + marker.length());
            // Remove quebras de linha iniciais
            while (!output.empty() && (output.front() == '\n' || output.front() == '\r' || output.front() == ' ')) {
                output.erase(0, 1);
            }
        } else {
            // Se nao encontrar a string exata (ex: multi-line), tenta encontrar apenas "> " e pega a ultima ocorrencia
            size_t last_marker = output.rfind("> ");
            if (last_marker != std::string::npos) {
                size_t end_of_line = output.find('\n', last_marker);
                if (end_of_line != std::string::npos) {
                    output = output.substr(end_of_line + 1);
                }
            }
        }
        
        // Remove a metrica de performance final e "Exiting..."
        size_t end_pos = output.find("[ Prompt:");
        if (end_pos != std::string::npos) {
            output = output.substr(0, end_pos);
        }
        
        // Remove trailing newlines/spaces se tiver
        while(!output.empty() && (output.back() == '\n' || output.back() == '\r' || output.back() == ' ')) {
            output.pop_back();
        }

        // Sanitize return string to be JSON-safe since it's commonly used as memory
        std::string sanitized_output = "";
        for (char c : output) {
            if (c == '"') sanitized_output += "\\\"";
            else if (c == '\n') sanitized_output += "\\n";
            else if (c == '\r') sanitized_output += ""; // drop CR
            else sanitized_output += c;
        }

        if (sanitized_output.empty()) {
            return "[Erro do GhostLLM] Nenhuma resposta recebida. Verifique se o caminho do modelo GGUF esta correto e se o executavel 'llama-cli' compativel com seu SO existe na pasta compiler/.";
        }

        return sanitized_output;
    }
};

// Função helper mapeada pela sintaxe do GhostLang
inline GhostLLM load_gguf(const std::string& path) {
    std::cout << "[Ghost OS] Modelo LLM Quântico/GGUF (" << path << ") instanciado com sucesso." << std::endl;
    return GhostLLM(path);
}

#endif // GHOST_LLM_HPP
