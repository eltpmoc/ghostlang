// GhostTypes.hpp - tabela de símbolos e verificador de tipos da GhostLang (c41 do dashboard).
//
// Roda sobre a AST (include/GhostAst.hpp) quando não há erro de sintaxe. É conservador: cada
// expressão tem um tipo conhecido (int, double, string, list<int>, uma classe, ...) ou "desconhecido",
// e só aponta erro quando tem certeza, nos casos que o g++ recusaria de qualquer jeito, mas com uma
// mensagem em português, linha e coluna:
//
//   GHOST-E0202  tipo incompatível (declaração, atribuição, argumento, return, condição, comparação)
//   GHOST-E0203  nome não declarado (variável ou função)
//   GHOST-E0204  declaração repetida no mesmo bloco (ou parâmetro repetido)
//   GHOST-E0302  membro inexistente numa classe (campo ou método)
//   GHOST-E0602  número de argumentos errado (função, método ou construtor; considera sobrecarga e padrão)
//   GHOST-E0603  return com valor em função void, ou sem valor em função que devolve algo
//   GHOST-E0104  break/continue fora de laço (ou de switch)
//
// Funções do runtime e da biblioteca de matemática: include/GhostBuiltins.hpp (gerado).
#ifndef GHOST_TYPES_HPP
#define GHOST_TYPES_HPP

#include <vector>

#include "GhostAst.hpp"

std::vector<AstDiagnostic> ghost_typecheck(const AstNode& program);

#endif
