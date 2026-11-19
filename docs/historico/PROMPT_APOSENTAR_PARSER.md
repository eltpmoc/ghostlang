# Prompt de continuação — GhostLang: aposentar o Parser antigo (c40, etapa 3)

> Copie este arquivo inteiro como primeira mensagem para a IA que vai continuar o trabalho.
> Escrito em 08/11/2026, ao fim da etapa 2 do c40 (gerador de C++ pela AST completo e padrão).

---

## 1. Quem você é e como trabalhar

Você vai continuar o desenvolvimento da **GhostLang**, uma linguagem transpilada para C++17 (com REPL interpretado), voltada a computação quântica e IA quântica. A ordem de plataformas é Windows, depois Linux, depois macOS. O dono do projeto escreve em **português do Brasil**: responda sempre em português.

Regras de trabalho, todas obrigatórias:

1. **Segurança antes de velocidade.** Nenhuma mudança pode alterar o comportamento de programas que já funcionam sem que isso seja intencional, testado e documentado. Trabalhe em etapas pequenas, cada uma verificada antes da seguinte.
2. **Backup antes de mexer.** No início de cada etapa grande, copie os arquivos que vai alterar para `backups/pre-<tema>-<AAAAMMDD>/`. Exemplo: `backups/pre-aposentar-parser-20261110/`.
3. **Testes sempre.** Ao fim de cada etapa, rode o CTest completo e o `ghost test tests`, e só siga com 100% passando. Hoje são **88 testes** no CTest e **44** no `ghost test tests`: 35 goldens e 9 de diagnóstico. Se o número cair, explique por quê.
4. **SEMPRE atualize a documentação e o dashboard** ao fim de cada etapa, no mesmo turno, sem esperar pedido (seção 7). Isso faz parte do "feito".
5. **Git:** o repositório tem `git init` e `.gitignore`, mas **nenhum commit ainda**. Só faça commit, branch ou push quando o dono pedir explicitamente.
6. **Não invente resultado.** Se um teste falhar, mostre a saída. Se algo não foi verificado, diga isso.
7. **Mensagens do compilador sem acento**, por exemplo `erro GHOST-E0202: tipo incompativel: ...`, seguindo o padrão existente. Documentação e comentários de código levam acento normalmente.
8. **Não use ou altere comandos destrutivos** (apagar pastas, sobrescrever arquivos sem ler) sem necessidade clara. Antes de apagar o Parser (etapa final), confirme com o dono.

---

## 2. Ambiente (Windows 11)

- **Raiz do projeto:** `H:\PROJETOS\PROJETOS\ghost-linux` (no bash: `/h/PROJETOS/PROJETOS/ghost-linux`).
- **Compilador:** MSYS2 ucrt64, g++ 15.2 (`C:\msys64\ucrt64\bin`), CMake e Ninja. O diretório de build é `build/` (Debug).
- **Python:** `python` (3.14), usado para scripts de edição e para o dashboard.
- **Binário instalado:** `bin/ghost.exe`. Depois de cada etapa aprovada, copie `build/ghost.exe` para `bin/ghost.exe`. Antes da cópia, `bin/ghost.exe` é a **versão de referência** usada nas comparações.

### Armadilhas conhecidas (importante)

- **O Bash da ferramenta esconde as mensagens de erro do linker.** Aparece só `collect2.exe: error: ld returned 1 (ou 5)`, sem o `undefined reference`. **Compile sempre pelo PowerShell:**
  ```powershell
  Set-Location H:\PROJETOS\PROJETOS\ghost-linux\build; $env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH; cmake .. | Out-Null; ninja 2>&1 | Select-String -Pattern " error|undefined|FAILED"; ctest -j4 2>&1 | Select-Object -Last 6
  ```
- **Heredoc do bash estraga escapes** (`\\n`, `\"`) em código C++ escrito por `cat <<EOF` ou `python - <<EOF`. Para edições com escapes, use a ferramenta de edição de arquivo ou escreva o script Python num arquivo e rode o arquivo. Nunca use `cat > arquivo` sem heredoc: ele fica esperando entrada.
- **`ghost run` compila em modo silencioso.** As mensagens `[Ghost] ...` do compilador só aparecem em `ghost build`.
- **Programas que não entram nas comparações:** `tests/ghost/error.g` tem erro de propósito; `libs/time.g` usa `function void sleep`, que o Parser nunca aceitou porque `sleep` é palavra-chave.
- **`ia_propria/`** depende de funções C++ nativas (`ia_*`, `v2_*`) do backend. O `ghost -x ... -o build/v5/ia.exe` falha no g++ de propósito; o script `ia_propria/scripts/compilar_v5.ps1` usa o `_temp.cpp` gerado. Os imports são relativos, então rode a partir de `ia_propria/`.

---

## 3. Arquitetura atual (o que você precisa saber)

```
fonte .g ──resolveImports──► texto juntado ──┬─► Lexer ─► GhostAst (AST) ─► GhostTypes (verificador) ─► GhostCodegen ─► C++  (PADRÃO)
                                             └─► Lexer ─► Parser (antigo: traduz tokens direto para C++)        ─► C++  (trava/referência)
```

| Arquivo | Papel |
|---|---|
| `src/ghost_compiler.cpp` | CLI (`ghost run/build/check/test/ast/cpp/version/help/repl`), `resolveImports`, `compileProgram`, `reportCppErrors` (erros do g++ → `.g`) |
| `src/Lexer.cpp`, `include/Lexer.hpp`, `include/Token.hpp` | lexer comum (compilador, AST e REPL) |
| `src/Parser.cpp`, `src/ParserCore.cpp`, `include/Parser.hpp` | **Parser antigo** (cerca de 4.000 linhas): gera C++ direto dos tokens e faz conferências semânticas próprias. **É o que você vai aposentar.** |
| `include/GhostAst.hpp`, `src/GhostAst.cpp` | AST: nós uniformes (`AstNode`: kind, text, name, attr, line, col, endLine, parens, kids), parser recursivo com recuperação de erros (vários erros por compilação), `ghost_ast_parse`, `ghost_ast_parse_expr`, `ghost_ast_dump`. O significado de cada nó está no topo de `GhostAst.hpp`. |
| `include/GhostTypes.hpp`, `src/GhostTypes.cpp` | verificador de tipos (c41): tabela de símbolos e tipos conservadores (desconhecido nunca gera erro). Códigos E0104, E0202, E0203, E0204, E0302, E0602, E0603. |
| `include/GhostBuiltins.hpp` | **gerado** por `python scripts/python/gen_builtins.py`: nomes do runtime aceitos sem declaração (funções e constantes) |
| `include/GhostCodegen.hpp`, `src/GhostCodegen.cpp` | gerador de C++ pela AST (c40 etapa 2). Cobre tudo, exceto `load "arquivo"`. `ghost_codegen()` e `ghost_cpp_equivalent()` (compara tokens sem espaços e a sequência de `#line`). |
| `include/GhostCppEmit.hpp` | partes fixas do C++, comuns aos dois geradores: cabeçalho, `main()`, montagem e `extern` das globais |
| `include/GhostDiag.hpp` | diagnósticos no formato `arquivo:linha:col: erro GHOST-Exxxx: msg` + trecho + `^`; mapa de linhas dos imports; `onFatal` (junta os erros do Parser com os da AST) |
| `include/GhostModules.hpp` | `import "x.g" as m` (renomeia para `m__nome`) |
| `src/Interpreter*.cpp` | o **REPL**: interpretador próprio, que não usa o Parser nem a AST |

### Fluxo de `compileProgram` (src/ghost_compiler.cpp), hoje

1. `resolveImports` junta os arquivos e preenche `GhostDiag::lineMap`.
2. `installAstDiagnostics(source)` monta `g_ast = parseAndCheck(source)` (AST + verificador) e instala `onFatal`.
3. `Parser parser(sourceCode, isWasm); cppCode = parser.parse();`: o **Parser ainda roda sempre**. Se ele encerrar com erro (`ghost_diag_fail` → `exit(1)`), `onFatal` imprime esse erro junto com os da AST, em ordem de linha.
4. Bloco "C++ gerado a partir da AST": se `GHOST_CODEGEN != parser` e a AST não tem erros, roda `ghost_codegen`.
   - Se o C++ for equivalente ao do Parser, usa o da AST.
   - Se for diferente, usa o do Parser e avisa `[Ghost] aviso: o C++ gerado pela AST ficou diferente do Parser ...`.
   - `GHOST_CODEGEN=ast` força a AST.
5. Grava o `_temp.cpp`, e `astFoundErrors()` para antes do g++ se a AST/verificador acharem erros. O `.cpp` fica gravado por causa do `ia_propria`.
6. Chama o g++, e `reportCppErrors` mapeia os erros do g++ para o `.g` (E9001).

- **Variáveis de ambiente:**
  - `GHOST_AST=0` desliga AST, verificador e gerador;
  - `GHOST_CODEGEN=parser` usa só o Parser;
  - `GHOST_CODEGEN=ast` força o C++ da AST.
- **Comandos de apoio:**
  - `ghost ast arquivo.g [--erros]` mostra a árvore ou só os erros;
  - `ghost cpp arquivo.g` mostra o C++ da AST;
  - `ghost cpp --comparar a.g b.g ...` imprime `igual` / `DIFERENTE` (com o primeiro trecho diferente) / `nao coberto (motivo)`.

### Detalhes do gerador que reproduzem o Parser (não quebre)

- O primeiro nome de um comando sai como está (`parseAction`): `set(x);` continua `set(x)`, mas em expressão vira `ghost_to_set`.
- A lista literal pega o tipo da declaração (`listLiteralType`), e só a primeira lista do valor.
- Função usada como valor vira `ghost_fn_val(f)`; função genérica como valor vira `ghost_make_poly_fn(...)`.
- Classes, interfaces e genéricos valem a partir da linha da declaração. `forwardClasses` sai em ordem alfabética.
- O contador único (`ghost_sw_N`, `ghost_fin_N`, `ghost_out_N`, `ghost_try_N`, `ghost_hyp_*_N`) avança nos mesmos pontos e na mesma ordem.
- **`#line`:**
  - sai antes de cada comando;
  - no nível principal, também antes de cada `}`;
  - a linha de `function`/`class`/`interface`/`circuit` vai para o `main()`;
  - **nada** dentro de `switch`, `try`, corpo de lambda, `hypothesis`, `circuit` e métodos padrão de interface.

---

## 4. Estado atual (08/11/2026)

- **Paridade do gerador:** em `tests/ghost` + `libs` são **51 programas iguais, 0 diferentes, 0 não cobertos** (CTest `codegen_paridade`, que exige `MIN_IGUAIS=51`). `ia_propria` tem 13 de 13 iguais. Em 68 programas do repositório a trava não disparou nenhuma vez.
- **CTest: 88 testes**, entre eles:
  - golden (`golden_*`, 35);
  - REPL (`repl_*`);
  - diagnóstico (`diag_*`, 9: `ponto_e_virgula parentese tipo_cpp template_cpp import modulo_membro varios_erros pv_antes_comando tipos`);
  - AST (`ast_expressoes`, `ast_cobertura`);
  - `codegen_paridade` e `codegen_ast_*` (9 programas compilados com `GHOST_CODEGEN=ast`);
  - `cli_check`, `cli_test_diag`.
- **Dashboard:** progresso **55,1%**, checklist **79/91**; c40 e c41 feitos. Recurso `cmp-ast` no nível 3: o nível 4 exige que o REPL use a mesma base.
- **O que ainda depende do Parser:**
  1. as **conferências semânticas** que só ele faz: cerca de 200 pontos de erro (`fail(` e `ghost_diag_fail(` em `Parser.cpp`/`ParserCore.cpp`), com cerca de 120 mensagens diferentes;
  2. o **`load "arquivo"`** (import em tempo de compilação com um sub-Parser), que nenhum programa do projeto usa;
  3. o papel de **oráculo** nos testes de paridade.

---

## 5. A tarefa: aposentar o Parser antigo com segurança

Objetivo final: o compilador não executa mais `Parser::parse()`. Todos os erros que o Parser apontava são apontados pela AST ou pelo verificador, com a mesma mensagem e o mesmo código. O C++ vem só de `GhostCodegen`, e a cobertura de testes não cai. Faça **nesta ordem**, verificando cada etapa antes da seguinte.

### Etapa A — Linha de base

1. Leia `docs/NUCLEO.md` (seções "Linha de comando", "Diagnósticos do compilador", "Árvore sintática (AST)", "Geração de C++ pela AST", "Verificador de tipos", "Diferenças entre compilador e REPL"), `docs/MANUAL.md` e `dashboard/ghostlang-status.json`.
2. Compile pelo PowerShell e rode o CTest (88/88) e o `ghost test tests` (44/44). Se algo não passar, pare e informe.
3. Faça o backup `backups/pre-aposentar-parser-<data>/` com `src`, `include`, `CMakeLists.txt` e `tests`.

### Etapa B — Congelar o oráculo (antes de qualquer remoção)

Quando o Parser sair, o teste de paridade perde a referência. Por isso, antes de remover:

1. Crie `tests/codegen/esperado/`. Para cada programa da paridade, gere com o Parser **atual** (`bin/ghost.exe`, `GHOST_CODEGEN=parser`, `cppFile` vazio) o C++ normalizado: sem espaços fora de strings, `#line` separados, do mesmo jeito que `ghost_cpp_equivalent` normaliza. Grave em `tests/codegen/esperado/<caminho_com__>.cpp.txt`.
2. Acrescente a `ghost cpp` uma opção `--comparar-com <pasta>`, que compara o C++ da AST com esses arquivos congelados, e um CTest `codegen_congelado` que use essa opção.
3. Confirme que `codegen_congelado` passa. A partir daqui ele é o oráculo, e qualquer mudança intencional no C++ gerado exige regenerar esses arquivos e explicar na documentação.

### Etapa C — Inventário das conferências do Parser

1. Liste todas as mensagens:
   ```bash
   grep -n 'fail(\|ghost_diag_fail(' src/Parser.cpp src/ParserCore.cpp
   ```
2. Classifique cada uma numa tabela, em `docs/PARSER_REGRAS.md` (arquivo novo):
   - **sintaxe:** a AST já pega, com a mesma mensagem ou equivalente;
   - **semântica:** precisa ir para o verificador `GhostTypes`;
   - **interna:** só fazia sentido na tradução por tokens e pode sumir.
   Exemplos de semântica:
   - capturas de lambda (variável não capturada, `[=, x]`, `this` fora de método, retornar lambda que captura local por referência);
   - `?` fora de `function result<T>`, dentro de `finally`, `spawn` ou compreensão;
   - genéricos (aridade de `<...>`, classe genérica sem `<tipo>`, restrição com classe ou interface inexistente);
   - classes (base declarada antes, campo `var`, campo `static` do próprio tipo, construtor genérico ou `static`, método duplicado, redefinir método genérico, `checkImplements`);
   - interfaces (método genérico, valor padrão, nome repetido);
   - `circuit` (nomes `_dg`/`_ctrl` reservados, aridade nas chamadas, só no nível principal, comandos permitidos dentro);
   - módulos (`m.x` inexistente);
   - `switch` com dois `default`, `try` sem `catch`/`finally`, `var` sem valor, variável `void`, `break`/`continue`.
3. Para **cada** regra semântica, crie um caso em `tests/diag/regras/<nome>.g` com `<nome>.erros` (o trecho esperado da mensagem atual do Parser). Rode com o compilador atual para provar que a mensagem é essa.

### Etapa D — Portar as conferências para o verificador

1. Implemente cada regra em `src/GhostTypes.cpp`, ou na AST se for de sintaxe, **com o mesmo texto de mensagem**, para não quebrar `.erros` nem confundir usuários. Use o código da categoria certa (tabela em `docs/NUCLEO.md`); quando fizer sentido, crie um código próprio (E03xx, E05xx, E06xx, E07xx, E09xx...) e documente.
2. Regra de ouro do verificador: **nunca recusar programa válido**. Depois de cada lote de regras, rode o `ast_cobertura` (todos os programas válidos passam sem erro), os goldens e o `ia_propria` (só podem aparecer as funções nativas `ia_*`/`v2_*`).
3. Crie um modo de teste sem o Parser: por exemplo `GHOST_PARSER=0`, em que `compileProgram` não chama `Parser::parse()` e usa só AST, verificador e gerador. Rode **todos** os `tests/diag` e `tests/diag/regras` nesse modo. Cada `.erros` precisa continuar passando. Registre isso num CTest (`diag_sem_parser_*`).

### Etapa E — `load "arquivo"`

Decida com o dono: portar para o gerador (import em tempo de compilação; ver `Parser::parseLoad`, que junta `classes`, `genericParams` etc. do sub-Parser) **ou** torná-lo um apelido de `import "arquivo"`, com aviso de que está obsoleto. Teste com um caso novo em `tests/ghost/` e documente.

### Etapa F — Trocar o padrão para "sem Parser"

1. Em `compileProgram`, o padrão passa a ser não rodar o Parser. `GHOST_CODEGEN=parser` (ou `GHOST_PARSER=1`) continua disponível por um período de transição, para comparação.
2. `ghost check` passa a usar só AST e verificador.
3. Rode tudo:
   - CTest, `ghost test tests`, `codegen_congelado`, `ast_cobertura`;
   - `ia_propria`: o `_temp.cpp` precisa sair igual ao de antes. Compare com `GHOST_CODEGEN=parser`, normalizando.
4. Meça e anote o tempo de compilação antes e depois (é para ficar mais rápido).

### Etapa G — Remover o Parser (só com confirmação do dono)

1. Peça confirmação explícita ao dono antes de apagar.
2. Remova `src/Parser.cpp`, `src/ParserCore.cpp` e `include/Parser.hpp` do alvo `ghost` no `CMakeLists.txt`; tire os includes e o código de `onFatal` que só servia ao Parser. Mantenha os arquivos no backup.
3. O `codegen_paridade`, que dependia do Parser, vira `codegen_congelado`. Atualize `tests/codegen/run_paridade.cmake` ou remova-o com explicação.
4. O que o Parser registrava e o gerador usa já está no gerador. Confira que nada mais referencia `Parser`:
   ```bash
   grep -rn "Parser" src include CMakeLists.txt
   ```
5. CTest, `ghost test tests` e `ia_propria` de novo. Copie para `bin/ghost.exe`.

### Etapa H (opcional, depois) — REPL pela mesma AST

É o caminho para o nível 4 de `cmp-ast` e para a paridade por construção. Faça só se o dono pedir: é um trabalho grande, separado.

---

## 6. Como verificar (receitas)

**Lista de programas** (exclui backups e builds):
```bash
cd /h/PROJETOS/PROJETOS/ghost-linux
find . -name "*.g" -not -path "./backups/*" -not -path "./build*" -not -path "./tests/diag/*" > "$TEMP/gfiles.txt"
```

**Paridade do gerador** (enquanto o Parser existir):
```bash
./build/ghost.exe cpp --comparar $(ls tests/ghost/*.g libs/*.g | grep -v "error.g\|libs/time.g") | tail -1
# esperado: "51 igual(is), 0 diferente(s), 0 nao coberto(s)"
cd ia_propria && ../build/ghost.exe cpp --comparar v5/src/main.g v4/src/main.g src/main.g v2/src/main.g v3/src/main.g src/modelo.g v3/src/conhecimento.g
```

**C++ gerado igual byte a byte entre duas versões do compilador** (use quando mexer em algo que não deveria mudar o C++). Tire o g++ do PATH, o que faz o build falhar rápido depois de gravar o `.cpp`:
```bash
O="$TEMP/cmp"; rm -rf "$O"; mkdir -p "$O/old" "$O/new"
while read f; do case "$f" in ./tests/ghost/error.g|./libs/time.g) continue;; esac
  nm=$(echo "$f" | sed 's|^\./||; s|/|_|g; s|\.g$||'); dir=$(dirname "$f"); base=$(basename "$f")
  ( cd "$dir" && PATH="/c/Windows/System32" /h/PROJETOS/PROJETOS/ghost-linux/bin/ghost.exe -d "$base" -o "$O/old/$nm" >/dev/null 2>&1
               PATH="/c/Windows/System32" /h/PROJETOS/PROJETOS/ghost-linux/build/ghost.exe -d "$base" -o "$O/new/$nm" >/dev/null 2>&1 )
  [ -f "$O/old/${nm}_temp.cpp" ] || continue
  cmp -s <(sed 's|/cmp/old/|/cmp/X/|' "$O/old/${nm}_temp.cpp") <(sed 's|/cmp/new/|/cmp/X/|' "$O/new/${nm}_temp.cpp") || echo "DIF $f"
done < "$TEMP/gfiles.txt"
```

**A AST e o verificador aceitam todos os programas válidos:** CTest `ast_cobertura`, ou um laço com `./build/ghost.exe ast "$f" --erros`.

**Um programa compilado pelo caminho da AST:** `GHOST_CODEGEN=ast ./build/ghost.exe build x.g -o x.exe`, que imprime `[Ghost] C++ gerado a partir da AST.`.

**Saída igual entre caminhos:** rode `ghost run` com `GHOST_CODEGEN=parser` e sem a variável, e compare com `diff`.

---

## 7. Documentação e dashboard: SEMPRE, ao fim de cada etapa

### Documentação

- **`docs/NUCLEO.md`** é a referência técnica da linguagem. Atualize as seções afetadas:
  - "Linha de comando": comandos e variáveis de ambiente;
  - "Diagnósticos do compilador": tabela de códigos GHOST-Exxxx;
  - "Árvore sintática (AST)" e "Geração de C++ pela AST": o que é coberto, o caminho padrão e os testes;
  - "Verificador de tipos": regras novas;
  - "Diferenças entre compilador e REPL".
  Mantenha os números certos: quantos goldens, quantos testes de diagnóstico, quantos programas na paridade.
- **`docs/MANUAL.md`** é o manual do usuário. Atualize o parágrafo da CLI (por volta da linha 397) e qualquer comportamento visível.
- **`docs/PARSER_REGRAS.md`** (novo, etapa C) é o inventário das regras do Parser e para onde cada uma foi.
- **Estilo:** português com acento, frases curtas, listas e tabelas, exemplos de código entre crases. Não deixe texto desatualizado: procure as menções antigas (por exemplo "o Parser roda sempre") e corrija.

### Dashboard (`dashboard/ghostlang-status.json`)

Edite por **script Python**: carregue o JSON, altere e grave com `json.dumps(d, ensure_ascii=False, indent=2) + "\n"` e `newline="\n"`. Assim a formatação se mantém. Estrutura:

- `projeto.atualizado_em`: data do dia (`AAAA-MM-DD`).
- `recursos[]`: cada recurso tem `id`, `nome`, `categoria`, `nivel` (0 a 5), `alvo`, `fase`, `prioridade`, `desde`, `onde`, `arquivos`, `descricao`, `evidencia` e `proximos_passos`. Recursos ligados a este trabalho: **`cmp-ast`**, **`cmp-typecheck`** e **`cmp-diagnosticos`**.
  - Atualize a `evidencia` com fatos verificados (números de testes, datas) e os `proximos_passos`.
  - Suba o `nivel` só quando o critério de `niveis[]` for cumprido: 3 = funcional no caminho principal; 4 = testado automaticamente, documentado e consistente entre compilador e REPL; 5 = otimizado, multiplataforma, CI.
- `checklist[]`: itens `{id, fase, texto, feito, recurso}`. Ao concluir um item, marque `feito: true` e ajuste o texto com a data e o resumo. Se este trabalho merecer um item próprio (por exemplo `c46` "Aposentar o Parser antigo"), crie-o na fase F2, ligado ao recurso `cmp-ast`.
- `linha_do_tempo[]`: acrescente `{data, titulo, descricao}` a cada etapa concluída.
- `snapshots[]`: acrescente `{data, progresso, recursos, nota}`.
  - `progresso` = arredondamento de `soma(min(nivel, alvo)) / soma(alvo) * 100` sobre todos os recursos. **Recalcule, não chute.**
  - `nota` traz o número de testes do CTest.
- Antes de gravar, compare com uma cópia (`diff`): a mudança deve ser pequena e só nos campos pretendidos.

Exemplo de cálculo do progresso:
```python
import json
d = json.load(open("dashboard/ghostlang-status.json", encoding="utf-8"))
r = d["recursos"]
print(round(sum(min(x["nivel"], x["alvo"]) for x in r) / sum(x["alvo"] for x in r) * 100, 1))
```

---

## 8. Padrões de código do GhostLang

- C++17, 4 espaços, chaves na mesma linha, comentários em português explicando o *porquê*.
- Diagnósticos sempre por `ghost_diag_fail` (Parser) ou `AstDiagnostic` com `code` (AST/verificador), no formato do gcc. Use `ghost_diag_print` para imprimir.
- **Paridade compilador × REPL** é princípio do projeto. Se mudar o comportamento de um recurso no compilador, verifique o REPL (`tests/repl/*.ghost` e `.expected`) e documente as diferenças na tabela de `docs/NUCLEO.md`.
- **Testes novos:**
  - golden: `tests/ghost/X.g` com `tests/golden/X.expected`, registrado na lista `foreach(prog ...)` do `CMakeLists.txt` (por volta da linha 167);
  - diagnóstico: `tests/diag/X.g` com `X.erros` (uma linha esperada por linha), na lista `foreach(diag ...)`;
  - REPL: `tests/repl/X.ghost` com `X.expected`.
- Gere o `.expected` com a versão de referência e confira a saída à mão: ela precisa estar **certa**, não só igual.
- Ao acrescentar funções ao runtime (`include/*.hpp`), rode `python scripts/python/gen_builtins.py`.

---

## 9. Como relatar ao dono

Ao fim de cada etapa, um resumo curto em português:
- o que mudou (arquivos com links);
- como foi verificado (números: CTest X/X, `ghost test` Y/Y, paridade N iguais);
- o que ficou de fora ou não foi verificado;
- dashboard e docs atualizados (progresso e checklist);
- a próxima etapa.

Não faça commit sem pedido. Se o dono pedir commit, use mensagens em português, por exemplo `c40: ...`.

---

## 10. Primeira ação sugerida

1. Rodar a etapa A (linha de base) e relatar os números.
2. Fazer a etapa B (congelar o oráculo) e a C (inventário em `docs/PARSER_REGRAS.md`, com os casos em `tests/diag/regras/`).
3. Parar e mostrar ao dono a tabela de regras, com quantas são de sintaxe, de semântica e internas, antes de começar a portar (etapa D).
