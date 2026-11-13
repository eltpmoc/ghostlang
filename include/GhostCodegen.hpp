// GhostCodegen.hpp - geração de C++ a partir da AST (c40, etapa 2).
//
// O Parser antigo (src/Parser.cpp) traduz direto dos tokens. Este gerador faz o mesmo trabalho a
// partir da árvore, construção por construção. Enquanto não cobre a linguagem toda, ele recusa o
// programa (devolve false e o motivo) ao encontrar algo que ainda não sabe gerar, e o compilador usa
// o Parser antigo. Para os programas que ele cobre, o C++ precisa ser o mesmo do Parser antigo:
// mesmos tokens e mesma sequência de #line (CTest codegen_paridade); só a formatação muda.
#ifndef GHOST_CODEGEN_HPP
#define GHOST_CODEGEN_HPP

#include <string>

#include "GhostAst.hpp"

struct GhostCodegenResult {
    bool ok = false;
    std::string cpp;     // C++ completo (ok)
    std::string reason;  // o que ainda não é coberto (não ok), ex.: "class (linha 3)"
};

// cppFile: nome do .cpp gerado (para as diretivas #line de volta ao .cpp; vazio = sem elas)
GhostCodegenResult ghost_codegen(const AstNode& program, bool wasm, const std::string& cppFile);

// Mesmo C++? Compara os tokens (sem espaços e quebras de linha fora de textos) e a sequência de #line.
// Em diferença, diff recebe a primeira linha diferente de cada lado.
bool ghost_cpp_equivalent(const std::string& a, const std::string& b, std::string* diff);

// Forma normalizada do C++ (a mesma régua de ghost_cpp_equivalent), em texto para gravar em arquivo:
// os #line (sem os de volta ao .cpp), uma linha "----", e os tokens sem espaços fora de textos, com
// uma quebra de linha depois de ';', '{' e '}' para o arquivo ficar legível. Dois C++ são
// equivalentes exatamente quando as formas normalizadas são iguais. É o formato dos arquivos
// congelados de tests/codegen/esperado/ (o oráculo que substitui o Parser antigo).
std::string ghost_cpp_normalized(const std::string& code);

#endif
