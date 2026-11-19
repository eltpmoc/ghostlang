# Especificação Formal da GhostLang (v0.5)

Este documento estabelece a especificação técnica formal da linguagem GhostLang: sintaxe, semântica, sistema de tipos, computação quântica, inteligência artificial e bibliotecas de tempo de execução.

---

## 1. Visão Geral da Arquitetura

A **GhostLang** é uma linguagem de programação estaticamente tipada, com inferência forte, voltada para:
1. **Computação Quântica Nativa:** Registradores quânticos (`qreg`), circuitos reutilizáveis (`circuit`), portas controladas (`mcx`, `ccx`), ansatz variacionais (`qcircuit`) e exportação/importação OpenQASM 3.0.
2. **Inteligência Artificial e Deep Learning:** Redes neurais clássicas e híbridas (`network >>`), camadas quânticas (`qlayer`), ativações analíticas (`activate(m, f)`), modelos de linguagem (`textmodel`, `llm GGUF`) e algoritmos de otimização (`Adam`, `SGD`).
3. **Álgebra Hipercomplexa e Numérica:** Números Fantasmas $G = x + y i + p g + \theta^2$, tensores e matrizes (`GhostMatrix`) com suporte multi-thread OpenMP e aceleração opcional em GPU CUDA.
4. **Execução dupla com paridade conferida por testes:** compilação transpilada para C++17 (`ghost build`) e interpretação direta no REPL ou por scripts (`ghost`). Os pares `golden_*`/`repl_*` do CTest conferem que os dois caminhos dão a mesma saída; as diferenças conhecidas estão em `docs/NUCLEO.md`.

---

## 2. Léxico e Tokens

### 2.1 Identificadores
Iniciados por letra ASCII `[a-zA-Z]` ou sublinhado `_`, seguidos por letras, dígitos `[0-9]` ou `_`.

### 2.2 Palavras-Chave Reservadas
- **Declaração e Tipos:** `int`, `double`, `float`, `string`, `bool`, `void`, `var`, `complex`, `ghost`, `date`, `time`, `datetime`, `json`, `list`, `map`, `set`, `result`, `fn`.
- **Controle de Fluxo:** `if`, `else`, `for`, `while`, `do`, `foreach`, `in`, `switch`, `case`, `default`, `break`, `continue`, `return`.
- **Tratamento de Exceções:** `try`, `catch`, `finally`, `throw`, `assert`, `ok`, `err`.
- **Orientação a Objetos:** `class`, `interface`, `extends`, `implements`, `public`, `private`, `protected`, `this`, `super`, `new`, `static`.
- **Quântico e IA:** `state`, `gate`, `qreg`, `qcircuit`, `qlayer`, `circuit`, `measure`, `network`, `activate`, `apply`, `show`, `load`, `pragma`, `hypothesis`.
- **Módulos:** `import`, `as`.

### 2.3 Literais
- **Inteiros:** Dígitos decimais (ex.: `42`, `-10`).
- **Ponto Flutuante:** Padrão decimal com ponto (ex.: `3.14159`).
- **Complexos e Imaginários:** Literais terminados no sufixo `i` (ex.: `4.0i`, `1.5 - 2.5i`, constante `i`).
- **Hipercomplexos (G):** Literais terminados no sufixo `g` ou `theta` (ex.: `3.5g`, `5.0theta`).
- **Booleanos:** `true` e `false`.
- **Cadeias de Caracteres:** Delimitadas por aspas duplas `"..."`. Suporta sequências de escape (`\n`, `\t`, `\"`, `\\`).
- **F-Strings:** Prefixo `f"..."` com interpolação de expressões `${expr}` e formatação `${valor:.2f}`.
- **Coleções Literais:**
  - Lista: `[1, 2, 3]`
  - Conjunto: `{1, 2, 3}`
  - Mapa: `{"chave": "valor"}`

---

## 3. Sistema de Tipos

### 3.1 Tipos Primitivos
- `int`: Inteiro de 64 bits (`long long`).
- `double` / `float`: Ponto flutuante IEEE 754 de 64 bits.
- `bool`: Booleano nativo (`true`, `false`). Conversão explícita com `bool(expr)`.
- `string`: Texto codificado em UTF-8.
- `void`: Indica ausência de retorno funcional.
- `complex`: Número complexo `std::complex<double>`.
- `ghost`: Elemento da álgebra quatérnica/nilpotente $G$.

### 3.2 Tipos de Dados Especializados
- `date`: Data gregoriana no formato `YYYY-MM-DD`.
- `time`: Horário no formato `HH:MM:SS`.
- `datetime`: Carimbo completo `YYYY-MM-DD HH:MM:SS`.
- `json`: Tipo estruturado para serialização e manipulação com `json_parse`, `json_get` e `json_stringify`.

### 3.3 Coleções e Contêineres Genéricos
- `list<T>`: Lista dinâmica sequencial indexada por zero. Suporta fatiamento `[start:end:step]` e compreensão `[x * 2 for x in xs if x > 0]`.
- `set<T>`: Conjunto com garantia de unicidade de elementos e operações de pertinência, união, interseção e diferença.
- `map<K, V>`: Tabela associativa chave-valor. Compreensão suportada via `{k: v for ...}`.
- `result<T>`: Tipo monádico de tratamento robusto de erros com variantes `ok(valor)` e `err(mensagem)`.

---

## 4. Estruturas de Controle

### 4.1 Condicional `if` / `else if` / `else`
```ghost
bool cond1 = false;
bool cond2 = true;
if (cond1) {
    // ramo 1
} else if (cond2) {
    // ramo 2
} else {
    // alternativa padrao
}
```

### 4.2 Repetição `for`, `while`, `do-while` e `foreach`
```text
for (var i = 0; i < 10; i = i + 1) { ... }
while (ativo) { ... }
do { ... } while (executando);
foreach (item in colecao) { ... }
foreach (qubit in registrador_qreg) { ... }
for (x in colecao) { ... }            // igual a foreach
for x in range(inicio, fim, passo) { ... }   // forma do Python, sem parenteses
for chave, valor in mapa { ... }
```
`range(fim)`, `range(inicio, fim)` e `range(inicio, fim, passo)` geram inteiros sem incluir o `fim` (passo negativo conta para baixo). O `for ... in` percorre `range`, textos (caractere a caractere), listas, conjuntos, mapas (chave e valor), matrizes `gate`/`matrix`/`GhostMatrix` (elemento a elemento, linha por linha) e `qreg` (índices dos qubits).

### 4.3 Seleção Múltipla `switch`
Suporta correspondência exata sobre inteiros, caracteres e strings:
```ghost
string comando = "iniciar";
switch (comando) {
    case "iniciar": print("start"); break;
    case "parar": print("stop"); break;
    default: print("desconhecido"); break;
}
```

---

## 5. Orientação a Objetos e Abstrações

### 5.1 Classes e Herança
- Modificadores de visibilidade: `public:`, `private:` e `protected:`.
- Subclasses herdam membros públicos e protegidos via `extends`. Chamada do construtor ancestral via `super(...)`.
- Membros estáticos (`static int contagem`) com escopo da classe e especializações parametrizadas em genéricos (`Classe<T>.staticCampo`).

### 5.2 Interfaces e Métodos Padrão
- Interfaces declaradas com `interface Nome<T>`.
- Múltipla implementação: `class Servico implements A, B`.
- Suporte a métodos padrão (default methods) fornecendo implementação base para as classes cumpridoras.

---

## 6. Funções, Lambdas e Closures

### 6.1 Funções Nomeadas e Sobrecarga
```ghost
function int somar(int a, int b) { return a + b; }
function double somar(double a, double b) { return a + b; }
```

### 6.2 Lambdas e Tipo Funcional `fn`
- Tipagem de primeira classe: `fn<int(int, int)> operacao = (a, b) => a + b;`.
- Inferência de tipos de parâmetros em expressões de seta: `(x) => x + 1`.
- Captura explícita por referência: `[&x](y) => x = x + y;`.

---

## 7. Computação Quântica (GhostQuantum)

### 7.1 Registrador Quântico `qreg`
- `qreg q(num_qubits);`: aloca registrador statevector.
- **Portas Fixas:** `h(q)`, `x(q)`, `y(q)`, `z(q)`, `s(q)`, `sdg(q)`, `t(q)`, `tdg(q)`, `sx(q)`.
- **Portas Controladas:** `cx(c, t)`, `cy(c, t)`, `cz(c, t)`, `ch(c, t)`, `swap(a, b)`, `ccx(c1, c2, t)`, `cswap(c, a, b)`.
- **Multi-Controladas (`mcx`):** `q.mcx([c0, c1, c2], target)` ou `q.mcx(c0, c1, c2, target)`.
- **Rotações Parametrizadas:** `rx(q, th)`, `ry(q, th)`, `rz(q, th)`, `p(q, phi)`, `u(q, th, phi, lam)`, `rzz(a, b, th)`.
- **Medição e Observáveis:** `measure(q)`, `measure_all()`, `histogram(shots)`, `counts(shots)` (retorna `map<string, int>`), `prob(q)`, `expect_z(q)`, `expect("ZZ")`.
- **Interoperabilidade:** Exportação e importação nativa OpenQASM 3.0 (`to_qasm()`, `from_qasm()`).

### 7.2 Sub-Circuitos Reutilizáveis `circuit`
```ghost
circuit bell(a, b) {
    h(a);
    cx(a, b);
}
```
Suporte nativo a versões inversas automáticas (`bell_dg`) e controladas (`bell_ctrl`).

---

## 8. Inteligência Artificial e Ativações

### 8.1 Redes Híbridas e Topologias
- Operador de encadeamento sequencial: `network rede = W1 >> Q >> W2;`.
- Camadas quânticas variacionais: `qlayer Q(n_qubits, depth);`.
- Otimização automática de gradientes: `rede => optimize(X, Y, epochs, lr, opt);`.

### 8.2 Funções de Ativação (`activate`)
Suporte a colapso e funções de ativação em matrizes e tensores:
- `relu`, `sigmoid`, `sin`, `cos`, `tan`
- `tanh`, `leaky_relu`, `gelu`, `softmax`

---

## 9. Gramática Formal EBNF (Resumo Estrutural)

```ebnf
Programa        ::= ( Declaracao | Importacao | Funcao | Classe | Interface | Circuit )*
Importacao      ::= "import" String ( "as" Identificador )? ";"
Funcao          ::= "function" Tipo Identificador ( "<" TypeParams ">" )? "(" Parametros? ")" Bloco
Classe          ::= "class" Identificador ( "<" TypeParams ">" )? ( "extends" Tipo )? ( "implements" TipoList )? "{" CorpoClasse "}"
Comando         ::= ComandoSe | ComandoPara | ComandoEnquanto | ComandoFazer | ComandoParaCada
                  | ComandoEscolha | ComandoRetorno | ComandoLancar | ComandoTentar | ComandoExpressao
ComandoSe       ::= "if" "(" Expressao ")" Bloco ( "else" ( ComandoSe | Bloco ) )?
ComandoExpressao::= Expressao ";"
Expressao       ::= Atribuicao | Ternario | Binario | Unario | Chamada | Literal | Primario
```

---

## 10. Álgebra Linear e Tensores N-Dimensionais

### 10.1 Tensores N-D e Regras de Broadcasting
O GhostLang implementa tensores multidimensionais com regras de alinhamento de formas estilo NumPy (da direita para a esquerda):
- Fatiamento unidimensional ou multidimensional (`slice`).
- Transposição arbitrária de eixos (`transpose(t, perm)`).
- Mudança de dimensões sem cópia (`reshape(t, new_shape)`).
- Operações de ponto a ponto: `tensor_add`, `tensor_sub`, `tensor_mul`, `tensor_div`.

### 10.2 Aceleração Matricial (GhostBLAS)
- `tensor_matmul` usa `cblas_dgemm` do OpenBLAS quando o runtime é compilado com `GHOST_USE_BLAS` (ligado sozinho quando `third_party/openblas/lib/libopenblas.a` existe; gerado por `scripts/build-openblas.ps1`). O OpenBLAS fica embutido na `libghost_runtime.a`. Intel MKL e cuBLAS não são suportados.
- `blas_backend()` devolve `"OpenBLAS"` ou `"Nativo (cache-blocked OpenMP)"`; `has_blas_hardware()` diz se o BLAS está ligado.
- Fallback nativo multithreaded OpenMP com cache-blocking em blocos L1 $32 \times 32$ para arquiteturas sem aceleração dedicada.

---

## 11. Computação Quântica Avançada e Hardware Real

### 11.1 Matrizes de Densidade e Ruído de Kraus
- Suporte a estados puros e mistos via `GhostDensityMatrix`:
  - $\rho = \sum_i p_i |\psi_i\rangle \langle\psi_i|$, com $\text{Tr}(\rho) = 1$.
  - Pureza do estado: $\gamma = \text{Tr}(\rho^2)$.
- Aplicação de canais quânticos através de conjuntos de operadores de Kraus $\{K_k\}$ satisfazendo $\sum_k K_k^\dagger K_k = I$:
  - Canal despolarizante, bit-flip, phase-flip, amortecimento de amplitude ($T_1$) e fase ($T_2$).

### 11.2 Integração com Hardware Físico e OpenQASM 3.0
- Interface de submissão no estilo dos provedores (IBM Quantum, AWS Braket, IonQ) via `libs/hardware.g`; hoje ela só emula localmente a distribuição de resultados, sem enviar jobs a hardware real.
- Exportação e importação nativa de programas no formato OpenQASM 3.0 (`to_qasm()`, `from_qasm()`).

---

## 12. Ferramental do Ecossistema e Protocolos

### 12.1 Language Server Protocol (LSP)
- Implementação nativa sobre JSON-RPC 2.0 (`ghost lsp`):
  - Diagnósticos semânticos e sintáticos integrados com o parser.
  - Hover informativo com documentação de assinaturas de funções e tipos.
  - Autocompletion consciente de contexto de símbolos locais, globais e bibliotecas.

### 12.2 Depuração (DAP) e Gerenciamento de Pacotes
- Compilação para depuração (`ghost debug`) com injeção de diretivas `#line` mapeando o código transpilado para o arquivo de origem `.g`.
- Gerenciador de pacotes GhostPM (`ghost pm`) com manifesto `ghost.toml` e travamento de versões determinístico via `ghost.lock`.

