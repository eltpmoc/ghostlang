// GhostCppEmit.hpp - partes fixas do C++ gerado, comuns ao Parser (src/Parser.cpp) e ao gerador pela
// AST (src/GhostCodegen.cpp): cabeçalho, começo e fim do main() e a montagem final do arquivo.
#ifndef GHOST_CPP_EMIT_HPP
#define GHOST_CPP_EMIT_HPP

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <sstream>
#include <string>

inline std::string ghost_cpp_header(bool wasm) {
    std::string h = "/* Transpilado pelo Ghost Compiler v1.0 */\n";
    h += "#include <iostream>\n";
    h += "#include <cstdio>\n";
    h += "#include <cstdlib>\n";
    h += "#include <string>\n";
    h += "#include <complex>\n";
    h += "#include <type_traits>\n";
    h += "#include <fstream>\n";
    h += "#include <sstream>\n";
    h += "#include <thread>\n";
    h += "#include <chrono>\n";
    h += "#include <mutex>\n";
    h += "#include \"include/GhostNumberG.hpp\"\n";
    h += "#include \"include/GhostMatrix.hpp\"\n";
    h += "#include \"include/GhostLLM.hpp\"\n";
    h += "#include \"include/GhostLib.hpp\"\n";
    h += "#include \"include/GhostDevice.hpp\"\n#include \"include/GhostTextModel.hpp\"\n#include \"include/GhostWeb.hpp\"\n#include \"include/GhostKnowledge.hpp\"\n#include \"include/GhostTemporal.hpp\"\n#include \"include/GhostTransformer.hpp\"\n#include \"include/GhostTransformerG.hpp\"\n#include \"include/GhostMatrixG.hpp\"\n#include \"include/GhostQuantum.hpp\"\n#include \"include/GhostQML.hpp\"\n#include \"include/GhostHybrid.hpp\"\n#include \"include/GhostQTape.hpp\"\n#include \"include/GhostCore.hpp\"\n";
    h += "#ifdef _WIN32\n";
    h += "#include <windows.h>\n";
    h += "#endif\n\n";
    if (wasm) h += "#include <emscripten.h>\n\n";
    return h;
}

inline std::string ghost_cpp_main_prologue() {
    std::string m = "int main(int argc, char** argv) {\n    try {\n";
    m += "#ifdef _WIN32\n";
    m += "    SetConsoleOutputCP(CP_UTF8);\n";
    m += "    SetConsoleCP(CP_UTF8);\n";
    m += "#endif\n";
    m += "    std::cout << std::boolalpha;\n"; // bool imprime true/false
    m += "    std::string ghost_args = \"\";\n";
    m += "    std::vector<std::string> ghost_args_vec;\n";
    m += "    for (int i = 1; i < argc; ++i) {\n";
    m += "        ghost_args += argv[i];\n";
    m += "        ghost_args_vec.push_back(argv[i]);\n";
    m += "        if (i < argc - 1) ghost_args += \" \";\n";
    m += "    }\n";
    return m;
}

// Chamada de function main (0, 1 ou 2 parâmetros) e fim do main() do C++
inline std::string ghost_cpp_main_epilogue(bool hasMain, int paramCount, bool takesArray) {
    std::string m;
    if (hasMain) {
        if (paramCount == 0) m += "    ghost_main();\n";
        else if (paramCount == 1) m += takesArray ? "    ghost_main(ghost_args_vec);\n" : "    ghost_main(ghost_args);\n";
        else if (paramCount == 2) m += "    ghost_main(argc, ghost_args);\n";
    }
    m += "    return 0;\n    } catch (const std::exception& e) { std::cerr << e.what() << std::endl; return 1; }\n";
    m += "}\n";
    return m;
}

// Variáveis globais declaradas antes das classes (extern): métodos de classe podem usá-las,
// inclusive as de módulos importados (as definições continuam em globalsCode, depois das classes).
inline std::string ghost_cpp_global_externs(const std::string& globalsCode) {
    std::string globalExterns;
    std::istringstream gl(globalsCode);
    for (std::string ln; std::getline(gl, ln);) {
        if (ln.empty() || ln.back() != ';' || ln.find('(') != std::string::npos || ln.find('=') != std::string::npos ||
            ln.find('{') != std::string::npos || ln.rfind("#", 0) == 0)
            continue;
        size_t e = ln.size() - 1, b = e;
        while (b > 0 && (std::isalnum(static_cast<unsigned char>(ln[b - 1])) || ln[b - 1] == '_')) --b;
        if (b == e || b == 0 || !(ln[b - 1] == ' ' || ln[b - 1] == '*' || ln[b - 1] == '&')) continue;
        globalExterns += "extern " + ln + "\n";
    }
    return globalExterns;
}

// Junta as partes na ordem: cabeçalho, declarações das classes, protótipos, externs das globais,
// classes, globais, funções e main. Depois de cada bloco com #line do .g, volta a numeração para o
// próprio .cpp (cppName vazio: sem essas voltas), para o código auxiliar entre os blocos não aparecer
// como se fosse do .g nos erros do g++.
inline std::string ghost_cpp_assemble(const std::string& header, const std::string& forwardClasses,
                                      const std::string& prototypes, const std::string& classes,
                                      const std::string& globals, const std::string& functions,
                                      const std::string& mainCode, const std::string& cppName) {
    auto join = [&](std::string& all, const std::string& part) {
        all += part;
        if (!cppName.empty() && part.find("#line ") != std::string::npos) {
            if (!all.empty() && all.back() != '\n') all += '\n';
            const size_t n = static_cast<size_t>(std::count(all.begin(), all.end(), '\n'));
            std::string f;
            for (char c : cppName) f += (c == '\\') ? '/' : c;
            all += "#line " + std::to_string(n + 2) + " \"" + f + "\"\n";
        }
    };
    const std::string externs = ghost_cpp_global_externs(globals);
    std::string all;
    for (const std::string* part : {&header, &forwardClasses, &prototypes, &externs, &classes, &globals, &functions, &mainCode})
        join(all, *part);
    return all;
}

#endif
