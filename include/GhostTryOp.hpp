// GhostTryOp.hpp - regra única (compilador e REPL) para o operador ? de result<T>.
//
//   double v = dividir(a, b)?;      // err: a função atual devolve o mesmo err; ok: v recebe o valor
//
// O '?' é o operador de result quando o token seguinte não pode começar uma expressão:
// ; ) , ] } . : ou um operador binário (* / % == != < > <= >= && || & | ^ << >>).
// Depois de '+' ou '-' ele também é o operador de result se não houver um ':' para fechar um ?: ternário
// (r? + 1). Em qualquer outro caso é o ?: de sempre. Dentro de um ?: use parênteses: c ? (f()? + 1) : 0.
#ifndef GHOST_TRY_OP_HPP
#define GHOST_TRY_OP_HPP

#include <vector>

#include "Token.hpp"

// rest: os tokens depois do '?' (até o fim da expressão ou do comando)
inline bool ghost_is_try_question(const std::vector<Token>& rest) {
    if (rest.empty()) return true;
    const Token& n = rest[0];
    switch (n.type) {
        case GTokenType::Semicolon: case GTokenType::RParen: case GTokenType::Comma: case GTokenType::RBracket:
        case GTokenType::RBrace: case GTokenType::Dot: case GTokenType::Colon: case GTokenType::EndOfFile:
        case GTokenType::Star: case GTokenType::Slash: case GTokenType::Percent: case GTokenType::Eq: case GTokenType::Neq:
        case GTokenType::Lt: case GTokenType::Gt: case GTokenType::Leq: case GTokenType::Geq:
        case GTokenType::LogicalAnd: case GTokenType::LogicalOr: case GTokenType::BitAnd: case GTokenType::BitOr:
        case GTokenType::Caret: case GTokenType::ShiftLeft: case GTokenType::Chain:
            return true;
        case GTokenType::Plus: case GTokenType::Minus: break;
        case GTokenType::Number: // "x -1" pode vir como número negativo
            if (!n.value.empty() && n.value[0] == '-') break;
            return false;
        default:
            return false;
    }
    // r? + 1  ou  c ? +1 : 0 : procura o ':' que fecharia um ternário
    int depth = 0, pending = 0;
    for (const Token& k : rest) {
        const GTokenType ty = k.type;
        if (ty == GTokenType::LParen || ty == GTokenType::LBracket || ty == GTokenType::LBrace) depth++;
        else if (ty == GTokenType::RParen || ty == GTokenType::RBracket || ty == GTokenType::RBrace) {
            if (--depth < 0) return true;
        } else if (depth == 0) {
            if (ty == GTokenType::Semicolon || ty == GTokenType::Comma) return true;
            if (ty == GTokenType::Question) pending++;
            else if (ty == GTokenType::Colon) {
                if (pending == 0) return false;
                pending--;
            }
        }
    }
    return true;
}

#endif
