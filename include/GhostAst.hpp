// GhostAst.hpp - árvore sintática (AST) da GhostLang (c40 do dashboard, etapa 1).
//
// O parser da AST (src/GhostAst.cpp) lê o programa inteiro e monta a árvore, sem gerar código.
// Ele se recupera dos erros de sintaxe (pula até o fim do comando) e por isso encontra vários erros
// numa compilação só. Nesta etapa a geração de C++ continua no Parser antigo (src/Parser.cpp):
// a AST serve ao `ghost check`, ao `ghost ast` (mostra a árvore) e à lista de erros; as próximas
// etapas usam a mesma árvore para o verificador de tipos (c41) e para gerar o C++.
//
// Um tipo de nó só (AstNode) com campos genéricos; o significado de cada campo por tipo de nó:
//
//   Program        kids = declarações e comandos do nível principal
//   Block          kids = comandos
//   Import         text = arquivo, name = apelido (import "x.g" as m)
//   VarDecl        name, kids[0] = Type, kids[1] = valor inicial (opcional); attr: static, public/private/protected (campos)
//   FuncDecl       name, kids[0] = Type do retorno, depois TypeParam*, Param*, e Block (sem Block: método de interface)
//                  attr: ctor, static, abstract, public/private/protected
//   Param          name, kids[0] = Type ("var" = sem tipo, em lambda e circuit), kids[1] = valor padrão (opcional)
//   TypeParam      name, attr = extends|implements (restrição), kids = Type das restrições (A & B)
//   ClassDecl      name, kids = TypeParam*, Type[attr=extends]?, Type[attr=implements]*, membros (VarDecl, FuncDecl, Domain)
//                  e Empty[text=section, name=public|private|protected] onde a seção começa
//   InterfaceDecl  name, kids = TypeParam*, Type[attr=extends]*, FuncDecl*
//   CircuitDecl    name, kids = Param*, Block
//   If             kids = condição, Block, else (Block ou If, opcional)
//   While          kids = condição, Block          DoWhile   kids = Block, condição
//   For            kids = início (VarDecl, ExprStmt ou Empty), condição (ou Empty), passo (ou Empty), Block
//   Foreach        name = variável, attr = segunda variável (chave, valor), kids = Type, coleção, Block
//   Switch         kids = valor, Case*          Case   kids = valores..., Block (attr = default: sem valores)
//   Try            kids = Block, Catch?, Finally?   Catch  name = variável (opcional), kids = Block   Finally kids = Block
//   Throw/Return   kids = valor (opcional)      Break, Continue, Empty (';' sozinho; fica no Block)
//   ExprStmt       kids[0] = expressão          Print   kids = argumentos
//   Spawn          kids = Block                 Collapse  text = max|min, kids[0] = pontuação
//   Hypothesis     name = variável, kids = Type, coleção, Block
//   Domain         comandos de IA/quântico/rede (state, gate, qreg, network, load, show, ...):
//                  text = palavra-chave, name = nome declarado (se houver), kids = argumentos / valor,
//                  attr = "=" quando o último filho é o valor depois de '='. "x => load(...)" tem text "=>".
//   Type           text = nome (int, double, string, list, map, set, result, fn, Classe...), kids = argumentos de tipo
//                  (fn: kids[0] = retorno, depois os parâmetros). int[] vira list<int>.
//
//   Expressões:
//   Number/String/FString  text = literal (FString: kids = expressões de ${...}; attr da expressão = formato)
//   Ident (text), This, Super
//   Unary          text = operador (- + ! ~ ++ --), kids[0]
//   Postfix        text = ++ ou --, kids[0]
//   Binary         text = operador, kids = esquerda, direita
//   Assign         text = = += -= *= /= %= **=, kids = alvo, valor
//   Ternary        kids = condição, então, senão
//   Call           kids[0] = função, depois argumentos       GenericRef  kids[0] = nome, depois Type* (f<int>)
//   Member         text = membro, kids[0] = objeto, depois Type* (obj.f<int>)      Index   kids = objeto, índice
//   Slice          kids = objeto, início, fim, passo (Empty quando omitido)
//   ListLit        kids = elementos (attr = set: {a, b})    MapLit  kids = MapEntry*   MapEntry kids = chave, valor
//   Lambda         kids = Capture*, Param*, Type do retorno (attr=ret, opcional), corpo (expressão, Block ou Print)
//   Capture        text = nome (= e & sozinhos: captura padrão), attr = ref quando &nome
//   Comprehension  attr = list|set|map, kids = cabeça (expressão, ou MapEntry em map), depois CompFor/CompIf
//   CompFor        name = variável, attr = segunda variável, kids[0] = coleção      CompIf  kids[0] = condição
//   TryOp          kids[0] (r? de result<T>)    New  kids[0] (new X(...))    TypeExpr  kids[0] = Type (list<int>())
//   Cast           text = tipo, kids[0] ((int)x)
#ifndef GHOST_AST_HPP
#define GHOST_AST_HPP

#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

enum class AstKind {
    Program, Block, Import, VarDecl, FuncDecl, Param, TypeParam, ClassDecl, InterfaceDecl, CircuitDecl,
    If, While, DoWhile, For, Foreach, Switch, Case, Try, Catch, Finally, Throw, Return, Break, Continue,
    Empty, ExprStmt, Print, Spawn, Collapse, Hypothesis, Domain, Type,
    Number, String, FString, Ident, This, Super, Unary, Postfix, Binary, Assign, Ternary, Call, GenericRef,
    Member, Index, Slice, ListLit, MapLit, MapEntry, Lambda, Capture, Comprehension, CompFor, CompIf,
    TryOp, New, TypeExpr, Cast
};

struct AstNode;
using AstPtr = std::unique_ptr<AstNode>;

struct AstNode {
    AstKind kind;
    std::string text;   // operador, literal, palavra-chave ou nome do tipo
    std::string name;   // nome declarado
    std::string attr;   // marcas (static, ctor, set, ...), separadas por espaço
    int line = 0, col = 0;
    int endLine = 0;    // Block: linha do '}' que fecha
    int parens = 0;     // expressão escrita entre parênteses (quantos pares): (a + b) * c
    std::vector<AstPtr> kids;

    AstNode(AstKind k, int l, int c) : kind(k), line(l), col(c) {}
    AstNode* add(AstPtr n) {
        kids.push_back(std::move(n));
        return kids.back().get();
    }
    bool has(const std::string& mark) const; // attr contém a marca
};

struct AstDiagnostic {
    int line = 0, col = 0;  // linha do texto juntado (GhostDiag::locate leva ao arquivo)
    std::string msg;
    std::string code;       // GHOST-Exxxx (vazio: pela categoria da mensagem, ghost_diag_code)
};

struct AstParseResult {
    AstPtr program;
    std::vector<AstDiagnostic> errors;  // em ordem; no máximo um por linha
};

// Monta a AST do texto (já com os imports juntados). Nunca encerra o processo.
AstParseResult ghost_ast_parse(const std::string& source);
// Uma expressão solta (o texto de um ${...}); line = linha da f-string; generics = nomes genéricos do
// arquivo (para ler f<int>(x)). nullptr se houver erro.
AstPtr ghost_ast_parse_expr(const std::string& source, int line, std::vector<AstDiagnostic>* errors,
                            const std::set<std::string>* generics = nullptr);

const char* ghost_ast_kind_name(AstKind k);
std::string ghost_ast_type_str(const AstNode& type);   // list<int>, fn<int(int)>, ...
// Árvore indentada, uma linha por nó: "VarDecl x : int  @5" (linha no arquivo de origem)
void ghost_ast_dump(const AstNode& n, std::ostream& os, int indent = 0);

#endif
