# Inventário e Classificação das Conferências do Parser Antigo

Este documento cataloga todos os pontos de erro (`fail(` e `ghost_diag_fail(`) presentes no Parser antigo (`src/Parser.cpp` e `src/ParserCore.cpp`) para orientar a migração e aposentadoria completa do Parser (Etapa 3 do item c40).

> **Status da Migração (Etapas D, E e F): CONCLUÍDO**  
> Todas as conferências semânticas e sintáticas foram portadas para `src/GhostTypes.cpp` e `src/GhostAst.cpp`.  
> O compilador agora opera por padrão sem executar o Parser antigo (`parserEnabled() == false`), validado por 94 testes no CTest e 62 testes no `ghost test tests`, incluindo bateria de testes dedicada em `tests/diag/regras/` e os oráculos congelados `codegen_congelado` e `codegen_congelado_ia`.

## 1. Resumo Quantitativo

- **Total de pontos de diagnóstico mapeados:** 160
- **Semântica (migram para `GhostTypes` / Verificador de Tipos):** 102 pontos
- **Sintaxe (já cobertos ou refinados no parser da AST `GhostAst`):** 46 pontos
- **Internas / Redundantes com a AST (podem ser eliminadas com o Parser):** 12 pontos

---

## 2. Regras Semânticas (Destino: `GhostTypes.cpp`)

Estas conferências validam tipos, visibilidade, herança, restrições e semântica de escopo. Devem ser integradas ao `GhostTypes` com mensagens idênticas e códigos `GHOST-E...` dedicados.

| Categoria | Regra / Mensagem | Código Proposto | Arquivo de Origem |
|---|---|---|---|
| **Classes & Herança** | `a classe base 'X' precisa ser declarada antes de Y.` | `GHOST-E0301` | `src/Parser.cpp:2192` |
| **Classes & Herança** | `class X: a classe base nao pode depender de T (use um tipo fixo, ex.: extends Base<int>)` | `GHOST-E0301` | `src/Parser.cpp:2201` |
| **Classes & Herança** | `'I' nao e uma interface declarada (implements de X).` | `GHOST-E0401` | `src/Parser.cpp:2212` |
| **Classes & Herança** | `ja existe uma classe ou interface chamada X` | `GHOST-E0204` | `src/Parser.cpp:1818, 2220` |
| **Classes & Herança** | `o construtor de X nao pode ser generico` | `GHOST-E0301` | `src/Parser.cpp:1343` |
| **Classes & Herança** | `o construtor de X nao pode ser static` | `GHOST-E0301` | `src/Parser.cpp:1355` |
| **Classes & Herança** | `super(...) so vale em classe declarada com extends.` | `GHOST-E0301` | `src/Parser.cpp:1405` |
| **Classes & Herança** | `super so pode ser usado em uma classe declarada com extends` | `GHOST-E0301` | `src/ParserCore.cpp:351` |
| **Classes & Herança** | `campo static 'x' nao pode ser do tipo da propria classe.` | `GHOST-E0301` | `src/Parser.cpp:1081` |
| **Classes & Herança** | `campo 'x' da classe precisa de tipo (var nao vale em campos).` | `GHOST-E0301` | `src/Parser.cpp:1078` |
| **Classes & Herança** | `campos qlayer nao existem em classes (use qreg ou qcircuit).` | `GHOST-E0301` | `src/Parser.cpp:2282` |
| **Classes & Herança** | `campos static ... nao existem (use um campo normal)` | `GHOST-E0301` | `src/Parser.cpp:2259` |
| **Classes & Herança** | `class X: metodo m com os mesmos parametros declarado duas vezes` | `GHOST-E0301` | `src/Parser.cpp:1441` |
| **Classes & Herança** | `X.m(): o metodo generico de Base nao pode ser redefinido (metodos genericos nao sao virtuais)` | `GHOST-E0301` | `src/Parser.cpp:1435` |
| **Classes & Herança** | `X.m(): um metodo generico nao pode redefinir o metodo m de Base` | `GHOST-E0301` | `src/Parser.cpp:1436` |
| **Classes & Herança** | `X e uma classe generica: use X<tipo>.membro` / `X<tipo>(...)` | `GHOST-E0501` | `src/Parser.cpp:572`, `ParserCore.cpp:429-430` |
| **Interfaces** | `interface X: 'B' nao e uma interface declarada antes` | `GHOST-E0401` | `src/Parser.cpp:1832` |
| **Interfaces** | `interface X: metodos de interface nao podem ser genericos (use interface X<T>)` | `GHOST-E0401` | `src/Parser.cpp:1891` |
| **Interfaces** | `interface X: parametros de interface nao tem valor padrao` | `GHOST-E0401` | `src/Parser.cpp:1904` |
| **Interfaces** | `interface X: metodo m declarado duas vezes` | `GHOST-E0401` | `src/Parser.cpp:1910` |
| **Interfaces** | `a classe X implementa I mas nao tem o metodo m()` | `GHOST-E0401` | `src/Parser.cpp:2145` |
| **Interfaces** | `X.m() tem tipos diferentes dos da interface I` | `GHOST-E0401` | `src/Parser.cpp:2142, 2167` |
| **Interfaces** | `X.m() e private/static/generico; a interface I precisa de um metodo normal/publico` | `GHOST-E0401` | `src/Parser.cpp:2139-2141, 2163-2165` |
| **Interfaces** | `a classe X herda o metodo padrao m() de mais de uma interface` | `GHOST-E0401` | `src/Parser.cpp:2156` |
| **Interfaces** | `X e interface: interfaces nao tem membros static` | `GHOST-E0401` | `src/ParserCore.cpp:445, 452` |
| **Interfaces** | `X e interface: crie um objeto de uma classe que implementa X` | `GHOST-E0401` | `src/ParserCore.cpp:447, 451` |
| **Lambdas & Closures** | `this na captura da lambda so vale em metodo de instancia` | `GHOST-E0601` | `src/ParserCore.cpp:815` |
| **Lambdas & Closures** | `'this' repetido na captura da lambda` | `GHOST-E0601` | `src/ParserCore.cpp:817` |
| **Lambdas & Closures** | `com '=' o objeto this ja e copiado; use [&this] para capturar por referencia` | `GHOST-E0601` | `src/ParserCore.cpp:819` |
| **Lambdas & Closures** | `com '&' o objeto this ja e capturado por referencia; use [this] para copiar` | `GHOST-E0601` | `src/ParserCore.cpp:821` |
| **Lambdas & Closures** | `captura invalida na lambda: use [&x], [x], [=], [&], [] ou [&this]` | `GHOST-E0601` | `src/ParserCore.cpp:827` |
| **Lambdas & Closures** | `variavel 'x' repetida na captura da lambda` | `GHOST-E0601` | `src/ParserCore.cpp:831` |
| **Lambdas & Closures** | `com '=' as variaveis ja sao capturadas por valor; liste apenas &variavel` | `GHOST-E0601` | `src/ParserCore.cpp:833` |
| **Lambdas & Closures** | `com '&' as variaveis ja sao capturadas por referencia; liste apenas variaveis por valor` | `GHOST-E0601` | `src/ParserCore.cpp:835` |
| **Lambdas & Closures** | `variavel 'x' nao declarada para capturar na lambda` | `GHOST-E0203` | `src/ParserCore.cpp:837` |
| **Lambdas & Closures** | `variavel local 'x' nao foi capturada na lambda (adicione x ou &x entre [ ])` | `GHOST-E0601` | `src/ParserCore.cpp:370` |
| **Lambdas & Closures** | `nao pode retornar lambda que captura variavel local por referencia` | `GHOST-E0601` | `src/ParserCore.cpp:373, 845` |
| **Lambdas & Closures** | `parametro 'x' repetido na lambda` | `GHOST-E0601` | `src/ParserCore.cpp:879, 899` |
| **Lambdas & Closures** | `parametros de lambda nao tem valor padrao` | `GHOST-E0601` | `src/ParserCore.cpp:882, 907` |
| **Lambdas & Closures** | `parametro da lambda nao pode ser void` | `GHOST-E0201` | `src/ParserCore.cpp:893` |
| **Genéricos** | `X: tipo T repetido entre < >` | `GHOST-E0501` | `src/Parser.cpp:1775` |
| **Genéricos** | `X: 'T' ja e uma classe; use outro nome para o tipo` | `GHOST-E0501` | `src/Parser.cpp:1776` |
| **Genéricos** | `X: T extends C: 'C' nao e uma classe declarada (use uma classe ou number)` | `GHOST-E0501` | `src/Parser.cpp:1791` |
| **Genéricos** | `X: T implements I: 'I' nao e uma interface declarada` | `GHOST-E0501` | `src/Parser.cpp:1795` |
| **Genéricos** | `X tem N tipo(s) entre < >` / `para usar X<...> como valor informe todos os N tipo(s)` | `GHOST-E0501` | `src/ParserCore.cpp:397, 401` |
| **Genéricos** | `X e uma classe/interface generica: use X<tipo>` | `GHOST-E0501` | `src/ParserCore.cpp:115, 133` |
| **Genéricos** | `X precisa de N tipo(s) entre < >` | `GHOST-E0501` | `src/ParserCore.cpp:126, 144` |
| **Genéricos** | `main nao pode ser generica` | `GHOST-E0501` | `src/Parser.cpp:1344` |
| **Result e Operador `?`** | `o operador ? so vale dentro de function result<T>` | `GHOST-E0701` | `src/ParserCore.cpp:334` |
| **Result e Operador `?`** | `o operador ? nao vale dentro de finally nem de compreensao de lista` | `GHOST-E0701` | `src/ParserCore.cpp:335` |
| **Result e Operador `?`** | `o operador ? nao vale dentro de spawn` | `GHOST-E0701` | `src/ParserCore.cpp:337` |
| **Circuit & Quântico** | `'c' ja e uma porta/metodo quantico; escolha outro nome para o circuit.` | `GHOST-E0901` | `src/Parser.cpp:386` |
| **Circuit & Quântico** | `nomes terminados em _dg sao reservados para o inverso` | `GHOST-E0901` | `src/Parser.cpp:389` |
| **Circuit & Quântico** | `nomes terminados em _ctrl sao reservados para a versao controlada` | `GHOST-E0901` | `src/Parser.cpp:392` |
| **Circuit & Quântico** | `circuit c deve ser declarado no nivel principal do arquivo` | `GHOST-E0901` | `src/Parser.cpp:395` |
| **Circuit & Quântico** | `parametro 'p' repetido em circuit c.` | `GHOST-E0901` | `src/Parser.cpp:419` |
| **Circuit & Quântico** | `(circuit c): 'x' nao e permitido dentro de circuit.` | `GHOST-E0901` | `src/Parser.cpp:459` |
| **Circuit & Quântico** | `circuit f espera N argumento(s) alem do registrador, recebeu M.` | `GHOST-E0901` | `src/Parser.cpp:522` |
| **Circuit & Quântico** | `(circuit c): 'x' nao e uma porta quantica nem um circuit declarado.` | `GHOST-E0901` | `src/Parser.cpp:541` |
| **Fluxo & Escopo** | `break fora de laco ou switch` / `continue fora de laco` | `GHOST-E0104` | `src/ParserCore.cpp:1554` |
| **Fluxo & Escopo** | `collapse_max/min so pode ser usado dentro de hypothesis` | `GHOST-E0100` | `src/ParserCore.cpp:1666` |
| **Módulos** | `o modulo m nao tem 'x'` | `GHOST-E1001` | `src/ParserCore.cpp:387` |
| **Tipos & Declarações** | `variavel 'x' nao pode ser void.` | `GHOST-E0201` | `src/Parser.cpp:1070` |
| **Tipos & Declarações** | `'var x' precisa de valor inicial (ou declare o tipo).` | `GHOST-E0201` | `src/Parser.cpp:1073` |

---

## 3. Regras de Sintaxe (Destino: `GhostAst.cpp` / Parser da AST)

Estas regras tratam de delimitação formal de tokens, estrutura de blocos e operadores:

- Faltando ponto e vírgula, parênteses, colchetes ou chaves (`faltou ';'`, `faltou ')'`, `faltou ']'`, `faltou '}'`).
- Tokens inesperados (`token inesperado X`, `comando inesperado X`).
- Delimitação de `switch`: `dentro de switch use case valor: ou default:`, `switch com dois default`.
- Delimitação de `try`: `try precisa de catch ou finally`.
- Delimitação de `if`/`else`: `'else' sem 'if' correspondente`.
- Estrutura de compreensões: `faltou 'in' depois da variavel`, `faltou a colecao depois de in`, `faltou 'for'`, etc.
- Estrutura de tipos: `use list<tipo>`, `use map<chave, valor>`, `use fn<retorno(parametros)>`.

---

## 4. Regras Internas / Redundantes

Estas conferências eram artefatos da tradução literal token-a-token do Parser antigo e não são necessárias com a AST:
- `use super.metodo(...) (ou super(...) na primeira linha do construtor)`: a AST já estrutura nós `SuperCall` e `SuperMember`.
- `comando inesperado '['`: no parser da AST, construções com colchetes são parseadas como indexação, lista literal ou lambda com captura.
- Erros internos de ponteiro de buffer de transposição de tokens.

---

## 5. Status da Aposentadoria (Etapa G Concluída)

- **Linha de base e backup:** Registrados em `backups/pre-aposentar-parser-20261108/`.
- **Oráculo congelado:** 53 programas de `tests/ghost/` e `libs/` + 7 programas de `ia_propria` congelados em `tests/codegen/esperado/` e validados via `codegen_congelado` e `codegen_congelado_ia` (100% idênticos).
- **Regras portadas:** 102 regras semânticas integradas ao `GhostTypes.cpp` e validadas pela suíte `tests/diag/regras/`.
- **Compatibilidade:** `load "arquivo.g"` suportado nativamente pelo pré-processador de módulos (`GhostModules.hpp`).
- **Remoção física e lógica:** `src/Parser.cpp`, `src/ParserCore.cpp` e `include/Parser.hpp` removidos do build e do repositório. O compilador `ghost` agora utiliza exclusivamente o pipeline moderno:
  $$\text{Código Ghost} \longrightarrow \text{Lexer} \longrightarrow \text{GhostAst} \longrightarrow \text{GhostTypes} \longrightarrow \text{GhostCodegen} \longrightarrow \text{g++}$$
- **Suíte de testes:** 93/93 testes `ctest` e 62/62 testes `ghost test tests` aprovados (100%).

