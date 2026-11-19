# Núcleo da linguagem GhostLang

Referência de sintaxe, tipos, controle de fluxo, funções, coleções, OOP, erros e módulos.
Vale para o **compilador** (`ghost arquivo.g -o app`, que gera C++17) e para o **REPL** (`ghost` sem argumentos), com as diferenças indicadas no fim.

Testes: `tests/ghost/nucleo.g` (CTest `golden_nucleo`, saída conferida linha a linha) e `tests/repl/nucleo.ghost` (CTest `repl_nucleo`), com os mesmos resultados nos dois caminhos.

## Tipos

| GhostLang | C++ gerado | Observação |
|---|---|---|
| `int`, `double`, `float` | `int`, `double`, `float` | `7 / 2 == 3` entre inteiros, também no REPL |
| `bool` | `bool` | `true`/`false`; conversão explícita `bool(x)` nos dois caminhos (`bool(0)` é `false`, `bool(42)` é `true`, `bool("")` é `false`, `bool("ok")` é `true`, `bool([])` é `false`, `bool([1])` é `true`, `bool(ok(v))` é `true`); `print` mostra `true`/`false` |
| `string` | `std::string` | `"a" + 1` concatena; métodos `upper`, `lower`, `contains`, `split`, `replace`, `trim`, `substr`... (REPL) |
| `var` | `auto` | precisa de valor inicial |
| `complex` | `std::complex<double>` | literal `2.0i` |
| `ghost` | `GhostNumberG` | construtor nativo explícito `ghost(x, y, p, theta2)`; literal `3.5g` (ou `3.5theta`); no valor de `ghost G = ...`, `i` e `theta` são as unidades: `ghost G = 1.5 - 2.5*i + 3.5*theta;` (uma variável com esse nome visível ali tem prioridade; o `i` de um `for (int i ...)` já fechado não conta). Suporte a formatação vetorial 4D (`g.to_4d()`, `{g:4d}`) e matemática (`g.to_math()`, `{g:math}`), além de parsing universal em `input` e `inputln`. |
| `matrix` | `GhostMatrix` | Álgebra linear numérica clássica de precisão dupla: declaração `matrix A(r, c);`, métodos `set(r, c, v)`, `get(r, c)`, `inverse()` / `inv()`, `det()` / `determinant()`, `show()`, produto matricial `A * B`, soma `+`, subtração `-`, produto e divisão escalar. |
| `GhostMatrix` | `GhostMatrix` | Motor matricial hipercomplexo 4D e Deep Learning: elementos quânticos `ghost(x, y, p, theta2)`, transposição `transpose()`, ativações neurais vetorizadas (`relu()`, `sigmoid()`, `tanh()`, `leaky_relu()`, `gelu()`, `softmax()`), pooling 2D e portas de Pauli/Hadamard. |
| `date`, `time`, `datetime`, `json` | tipos do GhostLib | |
| `list<T>`, `T[]` | `GhostList<T>` | ver Coleções |
| `map<K, V>` | `GhostMap<K, V>` | ver Coleções |
| `set<T>` | `GhostSet<T>` | conjunto ordenado sem repetidos; ver Coleções |
| `result<T>` | `GhostResult<T>` | erros recuperáveis com `ok`/`err`, também no REPL |
| `fn<Ret(P1, ...)>` | `GhostFn<Ret(P1, ...)>` | funções, lambdas e closures como valores (ver Lambdas e closures) |
| `qreg`, `qcircuit`, `qlayer` | runtime quântico | ver `docs/QUANTUM.md` |
| nome de classe | a própria classe | `Neuronio n = new Neuronio(0.5);` |
| nome de interface | valor que guarda qualquer objeto da interface | `Forma f = Quadrado(2);` (ver Interfaces) |
| classe genérica | `template <class T> class ...` | `Pilha<int> p;` (ver Classes genéricas) |

Variável declarada sem valor recebe o valor zero do tipo (`0`, `""`, `false`, lista vazia).

Números viram texto da mesma forma no `print`, nas f-strings, em `"texto" + numero` e dentro de listas e mapas, tanto compilado quanto no REPL. Um real com valor inteiro sai sem casas (`2.0` vira `2`); os outros saem com até 6 algarismos significativos (`2.5`, `0.333333`). Antes, `"x = " + 2.5` no programa compilado dava `x = 2.500000`.

Literais em notação científica valem nos dois caminhos: `1e20`, `2.5e-3`, `6.02E+23`.

## Matrizes de Alta Performance: `matrix` e `GhostMatrix`

O GhostLang disponibiliza duas abordagens nativas para operações matriciais com aceleração OpenMP e CUDA:

### 1. Álgebra Linear Numérica Clássica com `matrix`
Projetado para computação científica, sistemas de equações e operações matemáticas padrão:

```ghost
// Álgebra Linear Clássica e Resolução de Sistemas: matrix
function void main() {
    matrix A(2, 2);
    A.set(0, 0, 4.0); A.set(0, 1, 7.0);
    A.set(1, 0, 2.0); A.set(1, 1, 6.0);

    matrix A_inv = A.inverse(); // Inversão Gauss-Jordan com pivotamento parcial
    matrix I = A * A_inv;       // Multiplicação matricial (A * A_inv = Identidade)

    println("Matriz A original:");
    A.show();

    println("Matriz Inversa A^-1:");
    A_inv.show();

    println("Identidade resultante A * A^-1:");
    I.show();

    double d = A.det(); // Determinante O(n³)
    println(f"Determinante: {d}");
}
```

### 2. Álgebra Hipercomplexa 4D e Redes Neurais com `GhostMatrix`
Projetado para estados quânticos hipercomplexos, números $G$, ativações de inteligência artificial e visão computacional:

```ghost
// Álgebra Hipercomplexa G e Ativações de IA: GhostMatrix
function void main() {
    GhostMatrix G(2, 2);

    // Definindo elementos hipercomplexos 4D (x, y, p, theta2)
    ghost g1 = ghost(2.0, 1.0, 0.5, 0.0);
    ghost g2 = ghost(-3.0, 0.0, 1.2, 0.0);
    ghost g3 = ghost(1.5, -0.5, 0.0, 0.0);
    ghost g4 = ghost(4.0, 2.0, -1.0, 0.0);

    G.set(0, 0, g1);
    G.set(0, 1, g2);
    G.set(1, 0, g3);
    G.set(1, 1, g4);

    println("Matriz Hipercomplexa GhostMatrix G:");
    G.show();

    GhostMatrix G_t = G.transpose();
    println("Matriz Transposta G^T:");
    G_t.show();

    GhostMatrix G_dobro = G * 2.0;
    println("Matriz G multiplicada pelo escalar 2.0:");
    G_dobro.show();

    GhostMatrix G_relu = G.relu(); // Ativação ReLU elemento a elemento
    println("Matriz apos ativacao ReLU:");
    G_relu.show();
}
```

## Entrada de Dados via Teclado (`input`, `inputln`)

O GhostLang suporta entrada interativa do usuário via console tanto no compilador nativo (C++17) quanto no interpretador/REPL interativo, com dois formatos modernos:

1. **Atribuição Direta (`input`):** A chamada `input(prompt)` opera como expressão e pode ser atribuída diretamente a variáveis de tipos primitivos (`string`, `int`, `float`, `double`, `bool`), convertendo a linha digitada dinamicamente:
   ```ghost
   string nome = input("Digite o seu nome completo: ");
   int idade = input("Digite a sua Idade: ");
   float valor = input("Digite o valor do produto: ");
   ```

2. **Vinculação Direta com f-string (`inputln`):** A instrução `inputln(f"Prompt: {variavel}")` aceita interpolação da variável de destino diretamente entre chaves, atribuindo a digitação do usuário à variável especificada e consumindo a quebra de linha com segurança:
   ```ghost
   string nome;
   inputln(f"Digite o seu nome completo: {nome}");
   int idade;
   inputln(f"Digite a sua Idade: {idade}");
   float valor;
   inputln(f"Digite o valor do produto: {valor}");
   ```

O runtime gerencia o buffer de entrada (`std::cin`), descartando quebras de linha (`\n` e `\r\n`) pendentes para evitar que leituras de texto após números sejam puladas indevidamente.

## Saída e Formatação (`print`, `println`, `printf`)

O GhostLang oferece comandos nativos de saída:
- `println(...)`: Imprime expressões, strings ou f-strings com quebra de linha automática.
- `print(...)`: Imprime sem quebra automática de linha.
- `printf(...)`: Instrução de impressão compatível com estilo formatado e f-strings.
- Aliases: `escreva`, `escrevaln` e `imprimir`.

## F-strings e formatação

O GhostLang suporta tanto a sintaxe moderna `{expressao}` quanto a sintaxe `${expressao}`, além de escapes com duplas chaves `{{` e `}}`.

```ghost
string nome = "Ana";  double pi = 3.14159;  int n = 1234567;
println(f"Ola, {nome}! pi = {pi}");          // {expr} ou ${expr}: qualquer expressão
println(f"{pi:.2f}");                         // 3.14
println(f"[{nome:>6}] [{nome:<6}] [{nome:^7}] [{nome:*^9}]");   // [   Ana] [Ana   ] [  Ana  ] [***Ana***]
println(f"{n:,}  {n:12,}  {42:05d}  {-7:+d}");                  // 1,234,567     1,234,567  00042  -7
println(f"{0.256:.1%}  {255:x}  {255:X}  {8:o}  {5:b}");        // 25.6%  ff  FF  10  101
println(f"{pi:e}  {pi:.3e}  {pi:g}  {pi:.3}");                   // 3.141593e+00  3.142e+00  3.14159  3.14
println(f"{n > 0 ? pi : 0.0:.1f}");                             // o ?: continua valendo antes do ':' do formato
```

A especificação depois do `:` segue o formato do Python, `[[preenchimento]alinhamento][sinal][0][largura][,][.precisão][tipo]`:

- **Alinhamento:** `<` esquerda (padrão de textos), `>` direita (padrão de números), `^` centro. Um caractere antes dele é o preenchimento (`*^9`).
- **Sinal:** `+` sempre, espaço nos positivos, `-` só nos negativos (padrão). `0` completa com zeros depois do sinal.
- **`,`** separa milhares. **`.N`** são as casas (`f`, `e`, `%`), os algarismos (`g` ou sem tipo) ou o corte de textos (`{nome:.2}` dá `An`).
- **Tipos:** `f` fixo, `e`/`E` científico, `g`/`G` geral, `d` inteiro (arredonda), `x`/`X`/`o`/`b` hexadecimal, octal e binário, `%` porcentagem, `s` texto. Um tipo numérico num texto é erro (capturável).
- `bool` com tipo numérico vira `1`/`0`; sem tipo, `true`/`false`. Listas e objetos usam o texto normal e podem ser alinhados.
- O `:` só vira formato quando o resto é uma especificação válida e não fecha um `?:` ternário. A regra fica em `include/GhostFormat.hpp`, o mesmo código no compilador e no REPL.
- Dentro de `{...}` e `${...}` vale qualquer expressão, nos dois caminhos: listas e compreensões (`{[x * x for x in xs]}`), `**`, chamadas genéricas (`{maior<double>(a, b)}`), `Classe.static`, lambdas e outra f-string. O compilador traduz cada expressão com o mesmo tradutor das outras expressões.

## Operadores (precedência, da menor para a maior)

| Nível | Operadores |
|---|---|
| 1 | `?:` (ternário) |
| 2 | `\|\|` (curto-circuito) |
| 3 | `&&` (curto-circuito) |
| 4–6 | `\|`, `^` (XOR), `&` |
| 7 | `==`, `!=` |
| 8 | `<`, `<=`, `>`, `>=` (também entre textos) |
| 9 | `<<`, `>>` |
| 10 | `+`, `-` |
| 11 | `*`, `/`, `%` |
| 12 | unários `-`, `+`, `!`, `~` |
| 13 | `**` (potência; associatividade à direita) |

Atribuição: `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `x++`, `x--`, `++x`, `--x` (também em `xs[i] += 1`, `m["k"]++`).

Potência: operador `**` (ex.: `2 ** 3 == 8`, `3 * 2 ** 3 == 24`, encadeamento associativo à direita `2 ** 3 ** 2 == 512`) ou função `pow(x, y)`. O `^` é **XOR** nos dois caminhos (antes era potência só no REPL).

- **Unários:** como em Python e na matemática, `**` liga mais forte que o sinal à sua esquerda: `-x ** 2 == -(x ** 2)` e `-2 ** 2 == -4`. Para elevar o negativo, use parênteses: `(-2) ** 2 == 4`. No expoente o sinal vale normalmente: `2 ** -1`.
- **Tipo do resultado:** `int ** int` é `int` (por isso `x ** 2 % 4` vale), como `7 / 2 == 3`. Com expoente negativo, o resultado é a divisão inteira de 1 por aⁿ: `2 ** -1 == 0`, `1 ** -5 == 1`, `(-1) ** -3 == -1`, e `0 ** -1` é erro capturável. Se um dos lados for real, o resultado é real: `2.0 ** -1 == 0.5`, `9 ** 0.5 == 3`.
- O cálculo inteiro é o mesmo código nos dois caminhos (`ghost_ipow`, em `include/GhostNumText.hpp`).

## Raízes e Motor Matemático Especialista (`nth_root` / `raiz_n`, `libs/math.g` e `libs/stats.g`)

O GhostLang fornece suporte nativo a raízes quadradas (`sqrt(x)` ou `raiz_quadrada(x)`), raízes cúbicas (`cbrt(x)` ou `raiz_cubica(x)`) e cálculo de **raiz n-ésima universal**:
- `raiz_n(x, n)` ou `nth_root(x, n)` ou `root(x, n)`:
  - O índice $n$ pode ser **qualquer número natural maior do que zero** ($n \in \mathbb{N}, n \ge 1$).
  - Para radicandos positivos ($x \ge 0$), calcula $\sqrt[n]{x} = x^{1/n}$ com precisão IEEE 754 de 64 bits.
  - Para radicandos negativos ($x < 0$) e índice $n$ ímpar (ex.: $n = 3, 5, 7, \dots$), calcula a raiz real exata no corpo dos reais: $\sqrt[3]{-8} = -2$, $\sqrt[5]{-32} = -2$.
  - Para radicandos negativos com índice $n$ par, lança erro de domínio informando que raiz de índice par com radicando negativo não pertence aos reais.
  - Para $n \le 0$, lança exceção exigindo índice natural maior que zero.

A biblioteca `libs/math.g` oferece 80 funções cobrindo constantes fundamentais ($\pi, e, \tau, \phi, \sqrt{2}, \ln 2$), aritmética, teoria dos números (MDC de Euclides, MMC, primalidade determinística, fatorial, permutações e combinações), trigonometria direta e inversa, hiperbólicas e funções especiais ($\Gamma(x)$, beta, erf, erfc, ativações neurais).

A biblioteca `libs/stats.g` oferece 35 funções estatísticas de alto padrão: tendência central (média, mediana, moda, média ponderada, geométrica, harmônica e aparada), dispersão (amplitude, variância amostral e populacional, desvio padrão, erro padrão, coeficiente de variação, MAD), posição e forma (quantis, percentis, quartis $[Q_1, Q_2, Q_3]$, IQR, assimetria de Fisher-Pearson, curtose, z-scores, min-max scale), análise bivariada (covariância, correlação de Pearson, regressão linear com $[inclinação, intercepto, r^2]$) e distribuições probabilísticas (PDF e CDF normal, Poisson e Binomial).

## Controle de fluxo

```ghost
if (x > 0) { ... } else if (x == 0) { ... } else { ... }
for (int i = 0; i < n; i++) { ... }        // i existe só dentro do laço
for (x in xs) { ... }                       // igual a foreach (tipo opcional: for (int x in xs))
for x in range(2, 10, 2) { ... }            // forma do Python, sem parenteses (16/11/2026)
for k, v in mapa { ... }
foreach (x in xs) { ... }                   // listas, conjuntos, range(), textos (letra a letra), matrizes gate/matrix
                                            // (Numero G a Numero G, linha por linha) e qreg (itera qubits 0 .. n-1)
foreach (chave, valor in mapa) { ... }
while (cond) { ... }
do { ... } while (cond);
break;  continue;

switch (valor) {                            // números ou textos
    case 1, 2: ...; break;                  // vários valores por case
    case "a": ...;                          // sem break: cai no próximo case (como em C)
    default: ...;
}
```

`break` dentro de `switch` sai do switch; `continue` vale para o laço de fora.

`range(fim)`, `range(inicio, fim)` e `range(inicio, fim, passo)` seguem o Python: o `fim` não entra e `range(0)` não roda. No C++ gerado, `for x in m` sobre uma `GhostMatrix` usa o iterador da classe (`GhostMatrix::begin/end`, elemento a elemento por linha, por referência). No REPL, o mesmo laço dá o Número G de cada posição (`tests/*/for_range.*`).

## Funções

```ghost
function int dobro(int x) { return x * 2; }
function string saudacao(string nome, string prefixo = "Ola") { ... }   // valor padrão
function media(list<double> xs) { ... }                                 // sem tipo: void
function main(string args[]) { ... }                                    // 0, 1 ou 2 parâmetros
```

- **Ordem:** uma função pode ser chamada antes de ser declarada, porque o compilador gera protótipos.
- **Parâmetros:** são passados por valor. `qreg` e `qcircuit` são passados por referência.
- **Escopo:** parâmetros e variáveis declaradas no corpo são locais. Atribuições a variáveis de fora valem depois da chamada (também no REPL).
- **`return`** encerra a função. No REPL antigo, o corpo continuava executando depois do `return`.
- **Sobrecarga de funções e métodos:** você pode definir múltiplas funções globais ou métodos de classe com o mesmo nome e assinaturas de tipos/aridades diferentes (ex.: `formatar(int)`, `formatar(double)`, `formatar(string)`, `formatar(int, int)`). No compilador, é resolvido diretamente pela sobrecarga nativa do C++; no REPL, o interpretador pontua e seleciona deterministicamente a sobrecarga compatível com avaliação única de argumentos (sem efeitos colaterais duplicados). Caso nenhuma assinatura seja compatível ou ocorra ambiguidade, um erro claro é emitido.

### Funções genéricas

```ghost
function T maior<T>(T a, T b) { if (a > b) { return a; } return b; }
function T primeiro<T>(list<T> xs) { return xs[0]; }
function int contar<T>(T[] xs, T alvo) { ... }
function Pilha<T> pilhaDe<T>(list<T> xs) { Pilha<T> p; ... return p; }
function string parTexto<K, V>(K chave, V valor) { return chave + " -> " + valor; }
function T converter<T>(double v) { T r = v; return r; }

maior(3, 7)   maior(2.5, 1.5)   maior("ana", "bia")      // T deduzido: int, double, string
primeiro(nomes)   contar([1, 2, 1], 1)   pilhaDe([1, 2, 3])
maior<double>(3, 5.5)   converter<int>(7.9)   parTexto<string, double>("pi", 3)   // explícito
anunciar<double>(3);                                         // como comando também
```

- **Dedução de `T`:** quando os tipos não vêm entre `< >`, eles saem dos argumentos, como no C++. A dedução olha os parâmetros declarados como `T`, `list<T>`, `T[]`, `set<T>`, `result<T>` ou uma classe genérica como `Pilha<T>`.
  - Inteiros dão `int`, números com ponto dão `double`, textos dão `string`.
  - Numa lista, o tipo vem dos elementos.
- **Erros de dedução:** se `T` sair com dois tipos (`maior(3, 5.5)`) ou não puder ser deduzido (`T` só no retorno, lista vazia), é erro. Use os tipos explícitos.
- **Tipos explícitos:** `maior<double>(3, 5.5)` converte os argumentos para `double`.
- **Cada combinação é uma função separada:** `maior<int>` e `maior<string>`. Recursão e chamadas entre genéricas funcionam.
- **No compilador**, vira `template <class T> T maior(T a, T b)`. O que a dedução não resolve, como `maior(3, 5.5)` ou `>` entre objetos, é erro do C++.
- **No REPL**, a função fica guardada como modelo e cada combinação de tipos é criada no primeiro uso.
- **Corrigido junto:** parâmetros `T[] xs` e `Ponto[] pts` (antes só `int xs[]` funcionava) e argumentos com `Par<string, int>(...)` dentro de `print(...)` e de chamadas no REPL.

Teste: `tests/ghost/funcoes_genericas.g` (CTest `golden_funcoes_genericas`) e `tests/repl/funcoes_genericas.ghost` (CTest `repl_funcoes_genericas`), com a mesma saída. O roteiro do REPL também confere os erros.

### Métodos genéricos

```ghost
class Registro {
    list<string> linhas = [];
    function void anotar<T>(string rotulo, T valor) { linhas.push(rotulo + "=" + valor); }
    function list<T> repetir<T>(T valor, int n) { ... anotar("repetir", n); ... }   // chama outro genérico
    static function T maior<T>(T a, T b) { if (a > b) { return a; } return b; }
}
class Caixa<T> {
    T valor;
    function U converter<U>() { U r = valor; return r; }                          // genérico dentro de classe genérica
    function Caixa<U> com<U>(U novo) { return Caixa<U>(novo); }
}

r.anotar("idade", 30);   r.anotar<double>("pi", 3);   r.repetir("ab", 2)
Registro.maior(3, 9)   Registro.maior<double>(2, 2.5)   c.converter<double>()   c.com("texto")
```

- **Deduzidos ou explícitos:** os tipos funcionam como nas funções genéricas. Os métodos podem ser `static`, `private:` e chamar uns aos outros pelo nome.
- **Não são virtuais (como no C++):** uma subclasse não pode redefinir um método genérico, e um método genérico não pode redefinir um método da base. Também não cumprem métodos de interface. Os dois caminhos dão erro na declaração da classe.
- O construtor não pode ser genérico.
- **No C++:** `obj.m<double>(x)` vira `obj.template m<double>(x)`, forma que também vale dentro de funções e classes genéricas.
- **No REPL:** cada combinação de tipos (`anotar<int>`) é criada na classe dona do método no primeiro uso.

Teste: `tests/ghost/metodos_genericos.g` (CTest `golden_metodos_genericos`) e `tests/repl/metodos_genericos.ghost` (CTest `repl_metodos_genericos`), com a mesma saída. O roteiro do REPL também confere os erros.

### Restrições de tipo

```ghost
function T maiorMedida<T implements Medivel>(list<T> xs) { ... x.medida() ... }
function string descrever<T implements Medivel & Nomeado>(T x) { return x.nome() + ":" + x.medida(); }
function string falar<T extends Animal>(T a) { return a.especie + " faz " + a.som(); }
function T soma<T extends number>(list<T> xs) { T s = 0; foreach (x in xs) { s += x; } return s; }
function string par<K extends number, V>(K chave, V valor) { ... }       // só K tem restrição
class Ranking<T implements Medivel> { ... }
class Abrigo<T extends Animal> { ... }
static function T dobro<T extends number>(T x) { return x * 2; }        // métodos também
```

- **`T extends Classe`:** `T` precisa ser a classe ou uma subclasse dela. Os objetos continuam sendo valores: um `Abrigo<Animal>` guarda `Animal`, e um `Gato` posto nele é fatiado.
- **`T implements A & B`:** `T` precisa implementar todas as interfaces listadas (também vale uma interface que estenda `A`).
- **`T extends number`:** `T` precisa ser `int`, `double` ou `float`.
- Os tipos sem restrição continuam aceitando qualquer tipo. A restrição vale para tipos deduzidos e explícitos, em funções, métodos e classes genéricas.
- **Erros:** `soma(["a"])`, `falar(Caixa(1))`, `Ranking<Animal>` e outras violações são erro.
  - No compilador, vira um `static_assert` com a mensagem da GhostLang ("T precisa implementar Medivel: restricao de class Ranking").
  - No REPL, a mensagem sai ao criar a combinação ("Ranking<Animal>: Animal nao implementa Medivel").
  - Uma classe ou interface inexistente na restrição dá erro já na declaração, nos dois caminhos.
- **Corrigido junto:** no compilador, compreensões agora recebem cada elemento por cópia, como no REPL. Por isso `[b.som() for b in bichos]` funciona com métodos de objetos.

Teste: `tests/ghost/restricoes.g` (CTest `golden_restricoes`) e `tests/repl/restricoes.ghost` (CTest `repl_restricoes`), com a mesma saída. O roteiro do REPL também confere os erros.

### Lambdas e closures

```ghost
var saudacao = || => "ola";                            // sem parâmetros com pipes duplos
var dobrar = |int x| => x * 2;                         // parâmetro tipado com pipes
var triplo = |x| => x * 3;                             // inferência de tipo com pipes
fn<int(int)> quadrado = (int x) => x * x;              // formato clássico com parênteses
fn<int(int, int)> somar = |int a, int b| => { int r = a + b; return r; };
var rampa = |int x| -> double => { if (x < 0) { return 0; } return x * 1.25; };
var inv = |double x| -> result<double> => { double v = dividir(12.0, x)?; return ok(v); };

// Métodos de Ordem Superior em Listas (Programação Funcional)
list<int> valores = [1, 2, 3, 4, 5];
list<int> pares = valores.filter(|x| => x % 2 == 0);   // [2, 4]
list<int> dobro = valores.map(|x| => x * 2);           // [2, 4, 6, 8, 10]
int soma = valores.reduce(|acc, x| => acc + x, 0);     // 15

function fn<int()> criarContador(int inicio, int passo) {
    int atual = inicio;
    return || => { atual += passo; return atual; };    // closure mutável por valor
}
function list<U> mapear<T, U>(list<T> xs, fn<U(T)> f) { ... }

var c1 = criarContador(10, 5);   c1();  c1();          // 15, 20
var c2 = c1;                                           // cópia independente do estado capturado
mapear([1, 2, 3], dobro)                               // função nomeada como valor
mapear([1, 2, 3], |int x| => f"#${x}")                 // dedução de T e U a partir de fn<U(T)>
(|int a, int b| => a * b + 1)(6, 7)                    // chamada imediata (IIFE)
```

- **Sintaxe Dual Flexível:**
  - **Notação Matemática (Pipes):** `|parametros| => expressao` ou `|parametros| => { ... }` (estilo Rust/Ruby/Swift closures). Para zero parâmetros, utiliza-se `|| => expressao`.
  - **Notação Clássica (Parênteses):** `(parametros) => expressao` ou `(parametros) => { ... }`. Para zero parâmetros, utiliza-se `() => expressao`.
  - Em ambas as formas, os parâmetros aceitam **tipos explícitos** (`|int x|`) ou **inferência automática** (`|x| => x + 1`), sem valor padrão e sem `void`. Parâmetros inferidos geram lambdas polimórficas adaptáveis ao tipo esperado ao chamar ou passar para funções de alta ordem. Para usar `print(...)` dentro de uma lambda, use chaves: `|string s| => { print(s); }`.
- **Coleções com Métodos Funcionais:** `list<T>` oferece métodos nativos de alta ordem tanto no compilador C++17 quanto no REPL:
  - `.map(|x| => ...)`: mapeia cada elemento gerando uma nova lista com os resultados transformados.
  - `.filter(|x| => ...)`: filtra os elementos mantendo apenas os que satisfazem a condição booleana.
  - `.reduce(|acc, x| => ..., init)`: reduz a coleção a um único valor acumulado a partir de um valor inicial.
  - `.for_each(|x| => ...)`: itera sobre cada elemento aplicando efeitos colaterais.
- **Retorno explícito (`-> tipo`):** opcional antes de `=>`. Necessário quando um bloco devolve tipos diferentes (`int` e `double`), quando se quer converter para uma interface (`(string t) -> Descritivel => Etiqueta(t)`) ou para usar o operador `?` dentro da lambda (`-> result<T>`).
- **Tipo `fn<Ret(P1, P2, ...)>`:** escreve apenas os tipos (sem nomes de parâmetros). Vale em variáveis, parâmetros, retornos, campos de classe (inclusive `static`), `list<fn<...>>`, `map<K, fn<...>>` e funções encadeadas (`fn<fn<string(string)>(string)>`).
  - `fn<T> f;` sem valor inicial fica vazia (`!f` é `true`; `if (f)` testa se há função guardada). Chamar uma `fn` vazia lança erro capturável (`"fn sem funcao: guarde uma funcao ou lambda antes de chamar"`).
  - `print(f)`, `str(f)` e `f"${f}"` mostram `<fn>`.
  - Conversões aceitas no retorno ao guardar em `fn<...>`: `int` ou `float` para `double`, e qualquer tipo para `fn<void(...)>` (descarta o valor). Estreitamento (`double` para `int`) é recusado nos dois caminhos.
- **Closures e capturas:** por padrão (sem `[...]`), a captura externa é feita por valor mutável (`mutable`), com semântica de cópia independente nos dois caminhos. A sintaxe de **captura explícita** `[...]` antes dos parâmetros permite controle total:
  - `[]`: sem captura de variáveis externas.
  - `[=]`: captura todas as variáveis externas por valor.
  - `[&]`: captura variáveis externas por referência.
  - `[x]`, `[&x]`: captura a variável especificada por valor ou por referência.
  - `[=, &x]`, `[&, x]`: combina modo padrão e exceções individuais.
  - `[this]`, `[&this]`: dentro de métodos de instância, copia o objeto atual `*this` ou captura `this` por referência.
  - **Segurança de escopo:** retornar de uma função uma lambda que capture variável local por referência é estritamente proibido nos dois caminhos (a variável deixaria de existir ao sair da função).
- **Nome direto de funções genéricas como valor:** uma função genérica pode ser usada diretamente pelo seu nome sem envolver numa lambda manual (ex.: `fn<int(int)> f = id;` ou `var f = id<string>;`). Os tipos podem ser deduzidos pelo contexto da atribuição/chamada ou especificados explicitamente entre `< >`.
- **No compilador:** `fn<R(A...)>` vira `GhostFn<R(A...)>`, lambdas locais usam as capturas especificadas (`[=]`, `[&]`, `[]`, etc.) com `mutable`, lambdas polimórficas viram `ghost_make_poly_fn`, e funções genéricas usadas diretamente viram instâncias C++ deduzidas.

Testes: `tests/ghost/lambdas.g` (CTest `golden_lambdas`), `tests/repl/lambdas.ghost` (CTest `repl_lambdas`), `tests/ghost/lambdas_avancado.g` (CTest `golden_lambdas_avancado`) e `tests/repl/lambdas_avancado.ghost` (CTest `repl_lambdas_avancado`), com a mesma saída.

## Coleções

```ghost
list<int> xs = [5, 3, 8];          list<string> vazio = [];      var ys = [1, 2, 3];
xs.push(4);  xs.pop();  xs.insert_at(0, 9);  xs.remove_at(-1);  xs.sort();  xs.reverse();
xs.length()  xs.sum()  xs.min()  xs.max()  xs.contains(8)  xs.index_of(8)  xs.join(", ")
xs.slice(1, 3)  xs.first()  xs.last()  xs.sorted()  xs + ys
xs[0]  xs[-1]                        // índice negativo conta do fim; fora da lista = erro capturável
list<list<int>> grade = [[1, 2], [3, 4]];   grade[1][0]

map<string, int> m = {"ana": 30, "bia": 25};
m["caio"] = 41;   m["ana"] += 1;   m.has("bia")   m.get("zeca", -1)   m.keys()   m.values()   m.remove("bia")

len(x)   range(5)   range(1, 10, 2)   str(x)   int("42")   double("2.5")   typeof(x) (REPL)
```

`print(xs)` mostra `[5, 3, 8]` e `print(m)` mostra `{"ana": 30}`. No compilador, `GhostList` é um `std::vector` e `GhostMap` é um `std::map` ordenado.

### Conjuntos: `set<T>`

```ghost
set<int> a = [5, 1, 3, 3, 1];        // {1, 3, 5}: ordenado e sem repetidos
set<string> cores = ["verde", "azul", "verde"];
set<int> vazio;                       // {}
var s = set(lista);                   // conjunto a partir de uma lista

a.add(7)        // true se era novo, false se já existia
a.remove(1)     // true se existia
a.contains(3)   a.has(3)   a.length()   a.empty()   a.clear()   a.min()   a.max()   a.to_list()
a.union(b)   a.intersection(b)   a.difference(b)   a.is_subset(b)   a.is_superset(b)   a == b
foreach (x in a) { ... }              // em ordem crescente
```

- `print(a)` mostra `{1, 3, 5}` e textos aparecem entre aspas: `{"azul", "verde"}`.
- Vale como variável, parâmetro, retorno e campo de classe. `set<int> s = lista;` converte, e `set<double> d = conjunto_de_int;` também.
- Use conjuntos de números, textos ou `bool`. No compilador é um `std::set` (`s.union(o)` vira `s.union_(o)` no C++, porque `union` é palavra reservada).
- O literal `[...]` com valores de outro tipo (`set<int> s = [1.5]`) não compila no C++, como em `list<int>`. O REPL converte (`{1}`).

### Compreensões (listas, conjuntos e mapas)

```ghost
list<int> xs = [1, 2, 3, 4, 5, 6];
[x * 2 for x in xs]                          // [2, 4, 6, 8, 10, 12]
[x for x in xs if x % 2 == 1]                // [1, 3, 5]
[k * k for k in range(7) if k % 2 == 0]      // [0, 4, 16, 36]
list<double> metades = [x / 2.0 for x in xs];
set<int> restos = [x % 3 for x in xs];       // {0, 1, 2}: o tipo declarado decide a coleção
[[i * j for j in range(1, 4)] for i in range(1, 4)]   // aninhadas

// Compreensões com vários 'for' e filtros 'if' múltiplos
list<int> produtos = [x * y for x in [1, 2] for y in [10, 20] if x > 1 if y == 20]; // [40]

// Compreensão de conjunto {expr for ...} (ordena e remove duplicatas)
set<int> unicos = {x % 3 for x in [1, 2, 3, 4, 5, 6]}; // {0, 1, 2}

// Compreensão de mapa {chave: valor for ...}
map<string, int> tamanhos = {f: f.length() for f in ["uva", "banana"]};

// Iteração com par de chave e valor 'for k, v in mapa'
map<string, int> estoque = {"caneta": 5, "caderno": 12};
map<string, int> dobroEstoque = {f"item_${k}": v * 2 for k, v in estoque if v >= 10};
```

- A variável do `for` só existe dentro da compreensão: uma variável de fora com o mesmo nome não muda.
- Suporta múltiplos laços `for` aninhados na mesma compreensão (`for x in xs for y in ys`) e encadeamento de múltiplos filtros `if` (`if cond1 if cond2`).
- Suporta iteração em pares `for chave, valor in mapa`.
- A sintaxe `{chave: valor for ...}` cria um mapa, e `{expr for ...}` cria um conjunto ordenado sem repetições.
- **Conversão de elementos de `list<T>`:** no REPL, `list<int> xs = [1.9, 2.8];` converte e trunca os elementos para o tipo declarado (`[1, 2]`). No compilador esse literal é recusado pelo C++ (conversão com perda dentro de `{...}`); escreva os valores já no tipo certo. Veja a tabela de diferenças.
- No compilador, a compreensão vira uma expressão lambda executada na hora com tipos deduzidos ou explícitos.
- O operador `?` de `result` não vale dentro de compreensões.

### Fatiamento com passo e operações vetoriais/matriciais em lote

```ghost
list<int> xs = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
xs[::2];                  // [0, 2, 4, 6, 8]
xs[::-1];                 // [9, 8, 7, 6, 5, 4, 3, 2, 1, 0] (inversão)
xs[8:2:-2];               // [8, 6, 4]
string s = "GhostLang";
s[::2];                   // "Gotag"
s[::-1];                  // "gnaLtsohG"

// Operações vetoriais em lote
list<int> v1 = [1, 2, 3, 4];
list<int> v2 = [10, 20, 30, 40];
vec_add(v1, v2);          // [11, 22, 33, 44] (também v1.vec_add(v2))
vec_sub(v2, v1);          // [9, 18, 27, 36] (também v2.vec_sub(v1))
vec_mul(v1, v2);          // [10, 40, 90, 160] (também v1.vec_mul(v2))
vec_scale(v1, 2.5);       // [2.5, 5, 7.5, 10] (também v1.vec_scale(2.5))
dot(v1, v2);              // 300 (também v1.dot(v2))
v1.norm();                // 5.47723

// Operações matriciais em lote
list<double> mA = [1.0, 2.0, 3.0, 4.0];
list<double> mB = [5.0, 6.0, 7.0, 8.0];
mat_mul(mA, mB, 2, 2, 2); // [19, 22, 43, 50] (multiplicação linear A(rA, cA) * B(cA, cB))
mat_transpose(mA, 2, 2);  // [1, 3, 2, 4] (transposição)

list<list<double>> A2D = [[1.0, 2.0], [3.0, 4.0]];
list<list<double>> B2D = [[2.0, 0.0], [1.0, 2.0]];
mat2d_mul(A2D, B2D);      // [[4, 4], [10, 8]] (multiplicação de matrizes 2D aninhadas)
```

- **Fatiamento com passo `[start:end:step]`:** disponível para listas e textos. Suporta índices positivos, negativos e passo negativo decrescente (`step < 0`). Com passo negativo, início e fim omitidos significam "do último" e "até o primeiro": `xs[::-1]` inverte (corrigido no REPL em 07/11; antes devolvia vazio). Também disponível pelo método `xs.slice(from, to, step)`. Passo zero gera erro de execução.
- As funções vetoriais (`vec_add`, `vec_sub`, `vec_mul`, `vec_div`, `dot`, `mat_mul`) aceitam listas de tipos numéricos diferentes, com o tipo comum: `dot([1.5, 2], [2, 2]) == 7`.
- **Operações vetoriais e matriciais em lote:** executadas no runtime C++ nativo e interpretador, permitindo cálculos de álgebra linear rápidos diretamente em listas ou matrizes sem necessidade de bibliotecas externas.

Testes: `tests/ghost/colecoes.g` (CTest `golden_colecoes`), `tests/repl/colecoes.ghost` (CTest `repl_colecoes`), `tests/ghost/compreensoes_avancadas.g` (CTest `golden_compreensoes_avancadas`), `tests/repl/compreensoes_avancadas.ghost` (CTest `repl_compreensoes_avancadas`), `tests/ghost/nucleo_avancado.g` (CTest `golden_nucleo_avancado`), `tests/repl/nucleo_avancado.ghost` (CTest `repl_nucleo_avancado`), `tests/ghost/colecoes_avancadas2.g` (CTest `golden_colecoes_avancadas2`) e `tests/repl/colecoes_avancadas2.ghost` (CTest `repl_colecoes_avancadas2`).

## Classes

```ghost
class Animal {
    string nome;                 // campos tipados (públicos até o primeiro private:)
    int patas = 4;
    function Animal(string n) { this.nome = n; }          // construtor: mesmo nome da classe
    function string som() { return "..."; }
    function string descrever() { return nome + " faz " + som(); }
}

class Passaro extends Animal {
    function Passaro(string n) { super(n); this.patas = 2; }   // super(...) na 1ª linha chama a base
    function string som() { return "piu"; }                   // métodos são virtuais
    function string descrever() { return super.descrever() + "!"; }
}

var p = new Passaro("Sabia");     // objetos têm semântica de valor: new X(...) == X(...)
Contador c;                       // sem valor: construído sem argumentos
```

- **Construção:** a base é construída primeiro, com `super(...)` ou sem argumentos. Depois vêm os valores iniciais dos campos desta classe e, por último, o corpo do construtor. Parâmetros do construtor podem ter valor padrão: `function Ponto(double x = 0, double y = 0)`.
- **Nomes dentro de métodos:** um nome sem objeto procura primeiro variável local ou parâmetro, depois campo, depois variável global. `this.x` é sempre o campo, o que permite `this.x = x;`. Chamar `metodo(...)` sem objeto usa o método do próprio objeto antes de procurar uma função global.
- **`private:`:** campos e métodos marcados só podem ser usados por métodos da própria classe. Nem as classes filhas acessam. `public:` libera de novo.
- **Métodos virtuais:** vale a versão da classe real do objeto, inclusive quando chamada de um método da base. `super.metodo(...)` chama a versão da classe base.
- **Semântica de valor:** `Ponto b = a;` copia, e alterar `b` não muda `a`. Em listas, `pts[0].mover(1, 1)` e `pts[0].x = 5` alteram o elemento; `Ponto p = pts[0];` é uma cópia.
- **Fatiamento (como no C++):** guardar um objeto derivado numa variável, parâmetro, retorno ou campo do tipo base mantém só a parte da base, e os métodos passam a ser os da base. Por exemplo, `apresentar(Forma f)` chamada com um `Cubo` usa `Forma.area()`.
- **Atribuição em caminhos:** `r.canto.x += 1;`, `obj.lista.push(3);` e `grade[1][0] = 30;` funcionam.
- **Campos quânticos:** `qreg q(2);` e `qcircuit c(2);` declaram registradores dentro da classe, com o número de qubits fixo ou calculado a partir de campos anteriores. Nos métodos, `q.h(0)`, `this.q.cx(0, 1)`, `q.prob(1)` e circuits (`bell(q, 0, 1)` ou `q.bell(0, 1)`) usam o registrador do objeto. De fora, use `obj.q.h(0)` e `print(obj.q)`. Para trocar o tamanho no construtor, use `q = qreg(n);`. Copiar o objeto copia o estado quântico, como no C++. Campos `qlayer` não existem. Teste: `tests/ghost/classes_quantum.g` (CTest `golden_classes_quantum`) e `tests/repl/classes_quantum.ghost`.

Teste: `tests/ghost/classes.g` (CTest `golden_classes`) e `tests/repl/classes.ghost` (CTest `repl_classes`). Os dois têm a mesma saída; o roteiro do REPL também confere os erros de uso.

### Interfaces

```ghost
interface Nomeado {
    function string nome();
}
interface Forma extends Nomeado {           // uma interface pode estender outras
    function double area();
    function void escalar(double k);
}

class Quadrado implements Forma, Desenhavel {   // extends Base implements A, B também vale
    double lado;
    function Quadrado(double l) { lado = l; }
    function string nome() { return "quadrado"; }
    function double area() { return lado * lado; }
    function void escalar(double k) { lado *= k; }
    function string desenho(string pincel) { return pincel + "[]" + pincel; }
}

Forma f = Quadrado(2);                      // guarda o objeto inteiro (sem fatiamento)
list<Forma> formas = [Quadrado(3), Circulo(2), Cubo(1)];
foreach (x in formas) { soma += x.area(); } // cada um usa o método da sua classe
function string descrever(Forma f) { return f.nome() + " de area " + f.area(); }
function Forma criar(int tipo) { if (tipo == 0) { return Quadrado(1); } return Circulo(1); }
```

- Uma interface declara métodos com assinatura (`function tipo nome(parametros);`) ou **métodos padrão** com implementação default (`function bool vazia() { return tamanho() == 0; }`). Classes que dizem `implements` herdam a implementação padrão se não a implementarem, ou podem sobrescrevê-la.
- **Interfaces genéricas:** `interface Colecao<T>` e `interface Somavel<T extends number> extends Colecao<T>`. Classes podem implementar interfaces genéricas (`class Pilha<T> implements Colecao<T>` ou `class IntPilha implements Colecao<int>`).
- A classe que diz `implements` precisa ter todos os métodos das interfaces (que não tenham corpo padrão), também os das interfaces que elas estendem. Os métodos podem estar na própria classe ou numa base, e precisam ser públicos, não `static`, com os mesmos tipos. Se faltar algum, o erro aparece na declaração da classe, nos dois caminhos.
- Variáveis, parâmetros, retornos, campos e elementos de lista podem ter o tipo de uma interface (`list<Forma>`, `list<Colecao<string>>`). Em todos eles valem só os métodos da interface: `f.lado` é erro. A **checagem de tipos de interface dentro de `list<Interface>`** é aplicada nos dois caminhos, recusando valores que não implementam a interface esperada.
- **Semântica de valor:** `Forma g = f;` copia o objeto, e `g.escalar(10)` não muda `f`. Diferente das classes, aqui não há fatiamento: um `Cubo` guardado como `Forma` continua `Cubo`.
- `Forma f;` sem valor fica sem objeto, e chamar um método dele dá o erro "Forma sem objeto". `Forma()` é erro: interfaces não criam objetos.
- No compilador, `interface Forma` vira `ghost_iface_Forma`, com métodos virtuais herdados pelas classes (com implementação concreta no caso de métodos padrão), e o envoltório `Forma` com semântica de valor.

Testes: `tests/ghost/interfaces.g` (CTest `golden_interfaces`), `tests/repl/interfaces.ghost` (CTest `repl_interfaces`), `tests/ghost/interfaces_genericas.g` (CTest `golden_interfaces_genericas`) e `tests/repl/interfaces_genericas.ghost` (CTest `repl_interfaces_genericas`).

### Classes genéricas

```ghost
class Pilha<T> {
    list<T> itens = [];
    function void empilhar(T valor) { itens.push(valor); }
    function T desempilhar() { return itens.pop(); }
    function Pilha<T> copiaInvertida() {            // a própria classe com o mesmo T
        Pilha<T> r;
        for (int k = itens.length() - 1; k >= 0; k--) { T v = itens[k]; r.empilhar(v); }
        return r;
    }
}
class Par<A, B> implements Descritivel {           // vários tipos; implements também vale
    A primeiro;  B segundo;
    function Par(A a, B b) { primeiro = a; segundo = b; }
    function Par<B, A> trocado() { return Par<B, A>(segundo, primeiro); }
    function string descrever() { return "(" + primeiro + ", " + segundo + ")"; }
}
class PilhaDeInteiros extends Pilha<int> { ... }   // base genérica com tipo fixo

Pilha<int> p;                    Pilha<string> ps = palavras("a", "b", "c");
Par<string, int> idade = Par<string, int>("ana", 30);     new Pilha<double>()
Caixa<Ponto>  Caixa<list<int>>  Caixa<Pilha<int>>  list<Par<string, double>>
function int somaPilha(Pilha<int> p) { ... }       function Pilha<string> palavras(...) { ... }
```

- Os tipos entre `< >` podem ser qualquer tipo da linguagem: `int`, `string`, classes, interfaces, `list<int>` e outras classes genéricas.
- Cada combinação (`Pilha<int>`, `Pilha<string>`) é uma classe separada: `Pilha<string>` não pode ser guardada em `Pilha<int>`.
- Os objetos têm semântica de valor, como as outras classes.
- O nome sem tipos é erro: use `Pilha<int>`, não `Pilha`. Dentro da própria classe, escreva `Pilha<T>`.
- Um campo `T valor;` sem valor inicial cria `T` sem argumentos. Por isso, `Caixa<X>` precisa que `X` tenha um construtor sem argumentos obrigatórios, nos dois caminhos.
- **No compilador:** vira `template <class T> class Pilha`.
- **No REPL:** a classe fica guardada como modelo. No primeiro uso de `Pilha<int>`, o REPL cria a classe concreta com `T` trocado por `int` no corpo, como faz o compilador C++.
- **Membros static parametrizados:** classes genéricas suportam campos e métodos `static` (ex.: `Classe<T>.staticCampo` e `Classe<T>.staticMetodo()`). Cada especialização de tipo (`Classe<int>`, `Classe<string>`) possui suas próprias instâncias estáticas independentes.
- **Limitação atual de herança genérica:** a base de uma classe genérica não pode depender de `T` (`class B<T> extends Pilha<T>`, porque o C++ não acharia os campos da base pelo nome). Para interfaces genéricas, veja "Interfaces", e para funções genéricas, veja "Funções genéricas".

Teste: `tests/ghost/genericos.g` (CTest `golden_genericos`) e `tests/repl/genericos.ghost` (CTest `repl_genericos`), com a mesma saída. O roteiro do REPL também confere os erros de uso.

### Membros `static`

```ghost
class Registro {
private:
    static int proximoId = 1;               // um só para a classe (não um por objeto)
public:
    static list<string> log = [];
    int id;
    function Registro() { id = proximoId; proximoId++; log.push("id-" + id); }
    static function string resumo() { return log.length() + " registro(s): " + log.join(", "); }
    static function void reiniciar() { proximoId = 1; log.clear(); }
}

Registro r1;  Registro r2;
print(Registro.resumo());                   // 2 registro(s): id-1, id-2
Registro.log.push("manual");   Registro.reiniciar();
```

- **Modificador `protected:`**: campos e métodos declarados após `protected:` só podem ser acessados pela própria classe e por suas subclasses diretas ou indiretas em qualquer nível de profundidade na árvore de herança (`Base -> Meio -> Folha`). O compilador emite a seção correspondente em C++ e o REPL valida a árvore genealógica de classes recusando acessos externos com mensagem descritiva.
- **Acesso:** de fora, use `Classe.campo` e `Classe.metodo(...)`. Dentro dos métodos da classe e das subclasses, o nome sozinho basta. Subclasses herdam os membros static (`Cubo.criados` é o mesmo campo que `Quadrado.criados`).
- **`private:` e `protected:`** valem para os membros static do mesmo jeito que para os outros.
- **Métodos static** não têm `this` nem campos de objeto. Também não cumprem métodos de interface.
- **Valor inicial:** os campos static começam quando a classe é declarada. Use valores fixos ou expressões que não dependam de variáveis do programa. Um campo static não pode ser do tipo da própria classe.
- **No compilador:** viram `static inline` e `static` do C++, e `Classe.campo` vira `Classe::campo`.

Teste: `tests/ghost/interfaces.g` (CTest `golden_interfaces`) e `tests/repl/interfaces.ghost` (CTest `repl_interfaces`), com a mesma saída. O roteiro do REPL também confere os erros: método faltando, tipo diferente, `private`, `static`, `Forma()`, interface sem objeto e campo static privado.

## Erros

```ghost
try {
    throw "falhou";                      // throw com qualquer valor (vira a mensagem)
} catch (e) {                            // e = mensagem; pega também erros do runtime (índice, divisão...)
    print(e);
    throw;                               // relança
} finally {
    print("sempre roda");                // também em return/break/exceção
}

assert(x > 0, "x precisa ser positivo"); // lança erro capturável

function result<double> dividir(double a, double b) {
    if (b == 0) { return err("divisao por zero"); }
    return ok(a / b);
}
result<double> r = dividir(1, 0);
r.is_ok()  r.is_err()  r.value()  r.value_or(0)  r.error()      // value() de um erro lança a mensagem
```

Detalhes de `result<T>` (iguais no compilador e no REPL):

- `print(r)`, `"texto" + r`, `str(r)` e `f"${r}"` mostram `ok(2.5)`, `ok("texto")` ou `err("mensagem")`.
- O valor de `ok(...)` é convertido para `T`: `result<int> t = ok(3.9);` guarda `ok(3)`. `value_or` converte o reserva do mesmo jeito.
- `ok()` sem argumento é `ok(true)`. `err(404)` vira a mensagem `"404"`. `error()` de um `ok` é `""`.
- `result<T> r;` sem valor começa como `err("result sem valor")`.
- `if (r)` e `!r` testam `is_ok()`.
- Vale como parâmetro, retorno, campo de classe e elemento de lista: `result<list<int>>`, `result<Ponto>`, `list<result<int>>`.
- `result<int>` e `result<double>` são tipos diferentes: um não pode ser guardado no outro.

### Operador `?`: propagar o erro

Dentro de uma `function result<U>`, `expr?` desembrulha um `result<T>`. Se for `ok`, vale o valor. Se for `err`, a função termina na hora e devolve o mesmo erro, com a mesma mensagem.

```ghost
function result<double> mediaDasDivisoes(double a, double b, double c) {
    double x = dividir(a, b)?;          // err aqui: a função devolve err("divisao por zero")
    double y = dividir(a, c)?;
    return ok((x + y) / 2);
}
return raiz(dividir(a, b)?);            // como argumento
s += metade(x)?;                        // dentro de laços (sai da função, não só do laço)
return ok(pares(n)?.length());          // ?. continua com o valor
int m = metade(n)? * 10;                // seguido de operador
```

- O `?` vale para o operando logo antes dele: `f(x)?`, `obj.metodo()?`, `xs[i]?`, `(expr)?`, ou um campo `codigo?` dentro de um método.
- `finally` roda normalmente quando o `?` sai da função.
- O `?:` ternário continua igual. O `?` é o de `result` quando vem seguido de `;` `)` `,` `]` `}` `.` `:` ou de um operador binário (`*` `/` `%` `==` `<` `&&`...). Seguido de `+` ou `-`, é o de `result` se não houver um `:` de ternário depois. Dentro de um ternário, use parênteses: `c ? (f()? + 1) : 0`. A regra fica em `include/GhostTryOp.hpp`, a mesma para os dois caminhos.
- Erros: `?` fora de `function result<T>` (inclusive em `main`) e `?` dentro de `finally` ou `spawn` dão erro de compilação com a linha. No REPL, o erro aparece ao executar. `?` sobre algo que não é `result` dá erro do C++ (`ghost_try_arg`) no compilado e mensagem clara no REPL.
- No C++ gerado, `r?` vira uma expressão de bloco do GCC/Clang: `({ auto t = r; if (t.is_err()) return GhostErr{t.error()}; t.value(); })`.

Testes: `tests/ghost/result.g` (CTest `golden_result`) e `tests/repl/result.ghost` (CTest `repl_result`), com a mesma saída. O roteiro do REPL também confere os erros de uso.

Erro não capturado no programa compilado: a mensagem vai para `stderr` e o código de saída é 1.

## Multiverso: `hypothesis`

```ghost
double melhor = 0;
hypothesis (var t in [1, 2, 3, 4]) {     // listas, range(...) ou matrizes
    melhor = t * 10 - (t - 3) * (t - 3) * 5;
    collapse_max(melhor);                // ou collapse_min(score)
}
print(melhor);                            // 35: a linha do tempo vencedora vira a realidade
```

Cada valor roda o bloco numa cópia das variáveis que ele altera. Só o resultado da linha de melhor score fica valendo.

## Concorrência

`spawn { ... }` cria uma thread destacada. `sleep(ms)` aceita uma expressão. `mutex m;`, `lock(m);` e `unlock(m);` funcionam como antes.

## Módulos

```ghost
import "libs/quantum.g";                 // sem apelido: os nomes do arquivo entram direto no programa
import "modulos/geometria.g" as geo;     // com apelido: os nomes ficam em geo.
import "modulos/geometria.g" as g2;      // outro apelido = outra cópia (globais separadas)

geo.Ponto p = geo.Ponto(3, 4);           // classes, interfaces, funções (inclusive genéricas) e variáveis globais
print(geo.distancia(p, geo.Ponto()), " ", geo.maior<double>(2, 2.5), " ", geo.ESCALA);
geo.ESCALA = 5;                          // g2.ESCALA continua com o próprio valor
function double distancia(double a, double b) { ... }   // não colide com geo.distancia
```

- **Busca:** nesta ordem, na pasta de quem importa, na pasta atual e na raiz da GhostLang (a pasta com `include/` e `libs/`). Vale também no REPL, que agora aceita `import`.
- **Uma vez só:** cada arquivo entra uma vez por apelido. Um import circular (`a.g` importa `b.g`, que importa `a.g`) dá um aviso com a cadeia (`aviso: import circular: a.g -> b.g -> a.g`) e o import repetido é ignorado.
- **O que fica no módulo:** com `as m`, tudo o que o arquivo declara no nível de cima (funções, classes, interfaces, circuits e variáveis globais, inclusive `main`) só é acessível como `m.nome`. Dentro do próprio módulo, os nomes continuam sem prefixo. Os comandos soltos do módulo rodam no momento do import, e o `main` do módulo não roda.
- **Campos e métodos:** campos e métodos de classe não mudam de nome. Dentro de uma classe do módulo, um campo com o mesmo nome de uma função do módulo continua sendo o campo, como no C++.
- **Variável com o nome do apelido:** `m.nome` só vira o nome do módulo quando o módulo exporta `nome`, então uma variável comum chamada `m` continua funcionando (`m.keys()`). Um nome que o módulo não exporta dá erro: `o modulo geo nao tem 'naoExiste'` (GHOST-E1001).
- **Como funciona:** as declarações do arquivo importado são renomeadas para `m__nome`, e o lexer lê `m.nome` como `m__nome`. Nas mensagens de erro o nome aparece de volta como `m.nome`. É o mesmo código no compilador e no REPL (`include/GhostModules.hpp`).
- **Corrigido junto:** no compilado, as variáveis globais agora são declaradas antes das classes, então métodos de classe podem usar globais do programa ou do módulo (antes o C++ gerado não compilava).
- `namespace` declarado dentro do próprio arquivo (sem import) ainda não existe.

Teste: `tests/ghost/modulos.g` com `tests/ghost/modulos/` (CTest `golden_modulos`), `tests/repl/modulos.ghost` (CTest `repl_modulos`) e `tests/diag/modulo_membro.g` (CTest `diag_modulo_membro`).

## Linha de comando

```bash
ghost                          # REPL interativo (também: ghost repl)
ghost run programa.g [args]    # compila numa pasta temporária e executa; devolve o código de saída do programa
ghost build programa.g         # gera programa.exe ao lado do .g (ou -o saida); -x estático (padrão no Windows), -d dinâmico, --wasm
ghost check a.g b.g            # confere sintaxe e tipos (AST + verificador), sem chamar o g++: instantâneo
ghost ast programa.g           # mostra a árvore sintática (AST); com --erros, só a lista de erros de sintaxe
ghost cpp programa.g           # mostra o C++ gerado a partir da AST (o caminho padrão da compilação)
ghost cpp --comparar a.g b.g   # compara o C++ da AST com o do Parser (mesmos tokens e mesmos #line)
ghost test [pasta]             # roda cada X.g que tem X.expected ao lado (ou em ../golden/) e compara a saída
ghost programa.g -o saida      # uso antigo, igual a ghost build
ghost debug programa.g         # compila com -g e #line e abre o gdb nas linhas do .g (sem terminal: gdb em lote, run + bt)
ghost debug --dap              # adaptador DAP (VS Code) sobre o gdb/MI: breakpoints, pilha, variaveis, step, evaluate
ghost debug --saida x.exe a.g  # so compila com simbolos
ghost lsp [--test]             # Language Server (palavras-chave do lexer + funcoes das libs); --test confere 16 respostas
ghost pm add n --path p | --git url [--ref tag]; ghost pm install   # pacotes de pasta ou git, semver, commit no ghost.lock
ghost version | help
```

- `ghost test` compila e roda cada teste num processo separado (com `OMP_NUM_THREADS=4`) e mostra a primeira linha diferente quando falha. `ghost test tests` roda os 36 programas golden e os 26 testes de diagnóstico do projeto (62 testes no total).
- **Testes de erro:** com `X.erros` ao lado de `X.g`, o teste passa quando a compilação falha e cada linha do `.erros` aparece na mensagem do compilador (por exemplo `programa.g:3:13: erro GHOST-E0102`).
- No Windows o executável é estático por padrão: roda em qualquer máquina, sem o MSYS2 no `PATH` (conferido com objdump no CTest `cli_build_estatico`).
- `ghost run` compila para `%TEMP%\ghost-run\<nome>_<hash do caminho>_<pid>.exe` e apaga o executável depois de rodar: duas execuções ao mesmo tempo não se atrapalham.
- Testes do REPL (`tests/repl/run_repl_test.cmake`): além de conferir os trechos esperados, um `[Erro de Execucao]` que o `.expected` não prevê reprova o teste (desde 13/11/2026).
- Goldens com `-DFILTER=<regex>` ignoram linhas que casam com a regex (log de uma thread de servidor, por exemplo).

## Diagnósticos do compilador

Os erros saem no formato do gcc/clang, que os editores reconhecem, com o trecho e um `^` na coluna:

```text
programa.g:3:13: erro GHOST-E0102: faltou ';' no fim da linha (depois de ')')
 3 |     print(x)
   |             ^
```

- **Imports:** o arquivo e a linha são os do lugar certo, mesmo dentro de um arquivo importado (`lib/util.g:6: ...`).
- **`;` ou `)` faltando no fim da linha:** o erro aponta para o fim dessa linha, não para o comando seguinte. Um `(` ou `[` sem par aponta para onde ele foi aberto.
- **Erros que só o g++ percebe (GHOST-E9001):** tipos incompatíveis (`int y = texto;`), campo inexistente, `maior(3, 5.5)` e outros. O C++ gerado traz diretivas `#line` com o arquivo e a linha do `.g`, e o compilador mostra cada erro do g++ nessa linha, sem coluna, porque a coluna do g++ é a do C++. O log completo fica em `saida_gpp.log`.
- **Vários erros de uma vez:** o parser da AST lê o programa inteiro, descarta só o comando com erro e continua. `ghost build`, `run` e `check` mostram todos os erros de sintaxe em ordem de linha (um por linha) e, no fim, `[Ghost] N erros encontrados.` Também saem como erro de sintaxe, com coluna, casos que antes passavam e só davam erro no g++, como `int x = 5` sem `;` antes de outro comando ou `int y = x * ;`. Para usar só o parser antigo, defina `GHOST_AST=0`.

| Código | Categoria |
|---|---|
| GHOST-E0100, E0101, E0102, E0103 | sintaxe: geral, token inesperado, algo esperado/faltando, comando desconhecido |
| GHOST-E0104 | `break`/`continue` fora de laço (ou de `switch`) |
| GHOST-E0201 | declarações e tipos |
| GHOST-E0202 | tipo incompatível: declaração, atribuição, argumento, `return`, condição ou comparação |
| GHOST-E0203 | nome não declarado (variável ou função) |
| GHOST-E0204 | declaração repetida no mesmo bloco |
| GHOST-E0301 | classes (campos, métodos, construtor, `static`, `private`/`protected`, herança) |
| GHOST-E0302 | membro inexistente (a classe não tem o campo ou o método) |
| GHOST-E0401 | interfaces |
| GHOST-E0501 | genéricos e restrições |
| GHOST-E0601 | funções e lambdas |
| GHOST-E0602 | número de argumentos errado (função, método ou construtor) |
| GHOST-E0603 | `return` com valor em função `void`, ou sem valor em função que devolve algo |
| GHOST-E0701 | `result<T>` e operador `?` |
| GHOST-E0801 | coleções e compreensões |
| GHOST-E0901 | quântico (`qreg`, `qcircuit`, `circuit`) |
| GHOST-E1001 | módulos (`import`) |
| GHOST-E9001 | o C++ gerado não compilou (erro de tipo ou de uso que o g++ achou) |

A classificação fica em `include/GhostDiag.hpp`. Testes: `tests/diag/*.g` com `*.erros` e `tests/diag/regras/*.g` (CTest `diag_*`, `cli_test_diag`, `cli_test_diag_regras`, `diag_sem_parser_*` e `ghost test tests/diag`) compilam programas com erro e conferem a mensagem.

## Arquitetura do compilador e Árvore sintática (AST)

O compilador opera em um pipeline moderno unificado:
$$\text{Código Ghost} \longrightarrow \text{Lexer} \longrightarrow \text{GhostAst} \longrightarrow \text{GhostTypes} \longrightarrow \text{GhostCodegen} \longrightarrow \text{g++}$$

- O **parser da AST** (`include/GhostAst.hpp`, `src/GhostAst.cpp`) monta a árvore sintática do programa inteiro sem gerar código, com recuperação de erros e localização exata de diagnósticos;
- O **verificador de tipos** (`src/GhostTypes.cpp`, `include/GhostTypes.hpp`) executa a análise semântica e todas as 102 regras semânticas portadas do antigo parser;
- O **gerador de código** (`src/GhostCodegen.cpp`, `include/GhostCodegen.hpp`) emite o C++17 otimizado diretamente a partir da AST validada;
- O **Parser antigo** (`src/Parser.cpp`, `src/ParserCore.cpp`, `include/Parser.hpp`, ~4.000 linhas) foi **100% descontinuado e removido do build e do código** (Etapa G do plano de aposentadoria).

A AST (c40) serve para:
- encontrar todos os erros de sintaxe e múltiplos diagnósticos na mesma compilação;
- o comando `ghost ast`;
- o verificador de tipos (`src/GhostTypes.cpp`, c41);
- a geração de C++ a partir da árvore, que é o caminho exclusivo da compilação (`src/GhostCodegen.cpp`).

```text
$ ghost ast programa.g          # int a = -2 ** 2 + 3;
Program  @1
  VarDecl a : int  @1
    Binary +  @1
      Unary -  @1
        Binary **  @1
          Number 2  @1
          Number 2  @1
      Number 3  @1
```

- Um nó por linha, com o tipo do nó, o nome ou operador, as marcas entre `[ ]` e a linha do arquivo (`@N`). O significado dos filhos de cada nó está no começo de `include/GhostAst.hpp`.
- A precedência é a da tabela de operadores acima:
  - `**` liga mais forte que o sinal e associa à direita;
  - em `a -1`, o `-1` é subtração;
  - `f<T>(...)` só é genérico quando `f` é uma classe, interface, função ou método genérico declarado.
- Os `${...}` das f-strings viram nós de expressão; o formato (`${x:.2f}`) fica na marca `fmt=`.
- Os comandos de IA, rede e quântico (`state`, `gate`, `qreg`, `network`, `load`, `show`, `x => optimize(...)` e outros) viram nós `Domain`, com o nome declarado e os argumentos.
- Testes:
  - `tests/ast/expressoes.g` (CTest `ast_expressoes`) confere a forma da árvore;
  - `ast_cobertura` confere que a AST e o verificador aceitam todos os programas válidos de `tests/ghost` e `libs`.

### Geração de C++ pela AST (etapa 3 / sem Parser antigo)

O C++ é gerado diretamente a partir da árvore por `src/GhostCodegen.cpp` (`include/GhostCodegen.hpp`), sem depender do Parser antigo. As partes fixas do arquivo (cabeçalho, começo e fim do `main()` e a montagem) ficam em `include/GhostCppEmit.hpp`.

- **Cobre a linguagem compilada inteira:**
  - funções, inclusive genéricas, com restrições (`static_assert`) e as sobrecargas para lambdas polimórficas;
  - variáveis locais e globais, `print`, `if`/`else`, `while`, `do`, `for`, `foreach`, `break`, `continue`, `return`, `switch`, `try`/`catch`/`finally`, `throw`, `spawn` e `hypothesis`/`collapse`;
  - expressões: `**`, fatias, listas, mapas e JSON literais, compreensões (simples e gerais), lambdas (capturas `[x]`, `[&x]`, `[=]`, `[&]`, `[this]`, parâmetros sem tipo, `-> tipo`), f-strings, `result` com `?`, `2.0i`/`3.5g` e a álgebra `ghost`;
  - classes (herança, `super`, `static`, `public`/`private`/`protected`, genéricas), interfaces (métodos padrão, genéricas, `extends`) e `circuit` (com as variantes `_dg` e `_ctrl`);
  - os comandos de IA e quântico;
  - compatibilidade com `load "arquivo.g"` (etapa E: import legível em tempo de compilação).
- **Pipeline exclusivo sem o Parser antigo (Etapa G):**
  - `compileProgram` e `ghost check` utilizam diretamente AST + verificador de tipos + `GhostCodegen`.
  - O Parser antigo foi completamente removido do build e arquivado com segurança em `backups/pre-aposentar-parser-20261108/`.
- **Oráculo congelado:**
  - `tests/codegen/esperado/` guarda a referência de C++ gerado para 53 programas de `tests/ghost` e `libs` (CTest `codegen_congelado`);
  - `tests/codegen/esperado_ia/` guarda a referência para 7 módulos de `ia_propria` (CTest `codegen_congelado_ia`);
  - A suíte completa do CTest contava com 93 testes nessa etapa; em 19/11/2026 são 203, todos passando no Windows x64 com o GCC 16.2 no Windows x64.
  - **Mudanças intencionais no oráculo (12/11/2026):**
    - `x = conv(imagem, kernel[, passo]);` e `x = transpose(a);` voltaram a compilar, com o mesmo C++ do Parser antigo. O programa novo `tests/ghost/conv_transpose.g` foi congelado a partir do gerador da AST, porque o Parser já não existe; o texto gerado copia o de `parseConvAssignment`/`parseTransposeAssignment`.
    - Os laços que inicializam uma matriz `gate N(linhas, colunas)` passaram a usar `ghost_r`, `ghost_c` e `ghost_val` em vez de `r`, `c` e `val`. Antes, uma matriz chamada `r`, `c` ou `val` ficava escondida pelos laços e o g++ recusava o programa. Oito arquivos foram recongelados (seis de `tests/ghost` e `src/main.g` e `src/modelo.g` de `ia_propria`). Antes de recongelar, foi conferido que, aplicando essa troca de nomes ao C++ antigo, o resultado é exatamente o C++ novo.
    - `tests/ghost/device_hetero_std.g` foi reescrito (o programa mudou, não o gerador) quando a fachada Vulkan/Metal/ROCm saiu de `src/GhostDevice.cpp`: agora confere `device_supports(...)` falso e o erro de `use_device("vulkan")`. Só esse arquivo foi recongelado.
    - `tests/ghost/hardware_std.g` também foi reescrito e recongelado quando `qhw_submit` passou a executar o QASM no simulador (antes inventava 50/50): o programa confere só fatos determinísticos (soma das contagens, ausência de 01/10, um circuito sem superposição e o erro de porta desconhecida). A suíte tem agora 170 testes (entrou `cli_debug_dap_honesto`).
    - `tests/ghost/blas_matmul.g` (13/11/2026) é programa novo, congelado a partir do gerador da AST; confere `tensor_matmul` 70x50 por 50x40 com resultado exato, valendo com e sem OpenBLAS (entraram `golden_blas_matmul` e `repl_blas_matmul`).
- **Detalhes que mudam o C++, preservados com paridade exata:**
  - o primeiro nome de um comando sai como está;
  - uma lista literal pega o tipo da declaração;
  - função usada como valor vira `ghost_fn_val(f)`, e função genérica vira `ghost_make_poly_fn(...)`;
  - classes e genéricos valem a partir da linha em que são declarados;
  - o contador dos nomes auxiliares (`ghost_sw_N`, `ghost_fin_N`, `ghost_out_N`, `ghost_try_N`, `ghost_hyp_..._N`) avança nos mesmos pontos;
  - os `#line` saem nos mesmos lugares: antes de cada comando; no nível principal, também antes de cada `}`; nada dentro de `switch`, `try`, lambda, `hypothesis`, `circuit` e métodos padrão de interface.
- **Testes:**
  - `tests/ghost/codegen_nucleo.g` (núcleo) e `tests/ghost/codegen_completo.g` (recursos menos comuns: JSON, compreensões de mapa e conjunto, `switch` com queda, `throw;`, `finally`, `hypothesis` com `collapse_min`, restrições, capturas de lambda, `result ?`, `static`, interface genérica com método padrão, `circuit` com `_dg`);
  - os CTest `codegen_ast_*` compilam pelo caminho da AST (núcleo, completo, classes, lambdas, interfaces genéricas, `circuit`, `nucleo.g`) e conferem a saída do golden.
- **Próximos passos:** Evolução do REPL para também compartilhar a mesma AST unificada.

## Verificador de tipos

Depois da AST, quando não há erro de sintaxe, o verificador de tipos (`include/GhostTypes.hpp`, `src/GhostTypes.cpp`, item c41) passa pelo programa. Ele monta a **tabela de símbolos**:
- funções, com sobrecarga e parâmetros com valor padrão;
- classes, com campos, métodos, construtores, base e interfaces;
- interfaces, `circuit` e variáveis globais;
- os escopos de cada bloco.

Depois ele calcula o tipo de cada expressão. Ele aponta, antes do g++, com linha e coluna:

```text
tipos.g:13:5: erro GHOST-E0202: tipo incompativel: a variavel 'n' e int, mas o valor e string
tipos.g:15:15: erro GHOST-E0602: a funcao soma espera 2 argumentos, mas recebeu 1
tipos.g:18:13: erro GHOST-E0302: a classe Ponto nao tem o campo 'y'
tipos.g:20:5: erro GHOST-E0203: variavel 'total' nao foi declarada (use int total = ...; ou var total = ...;)
tipos.g:23:5: erro GHOST-E0204: 'n' ja foi declarado neste bloco (linha 13)
```

- **Conservador:** cada expressão tem um tipo conhecido ou "desconhecido", e desconhecido nunca gera erro. Contam como desconhecidos, entre outros:
  - genéricos `T`;
  - `json`;
  - interfaces;
  - o retorno das funções do runtime;
  - mapas literais fora de uma declaração `map<K, V>`.
- **Só erros que o g++ também recusaria:** os mesmos programas continuam válidos, mas a mensagem sai em português, com coluna, e várias de uma vez. `diag_tipo_cpp` (`int y = texto;`) era um erro do g++ (`E9001`) e agora é `E0202`.
- **O que continua aceito, como no C++:**
  - `int` para `double` e o contrário;
  - `bool` com números;
  - texto `+` número, que concatena;
  - classe derivada para base;
  - classe com construtor de um argumento recebendo um valor.
- **Nomes do runtime:** as funções dos cabeçalhos (`include/*.hpp`), as do REPL e as da biblioteca de matemática do C estão em `include/GhostBuiltins.hpp`. Esse arquivo é gerado por `python scripts/python/gen_builtins.py`; rode o script de novo ao acrescentar funções ao runtime. Quando um nome é usado como valor (sem chamada), só valem as constantes (`PI`, `true`, ...). As portas quânticas (`h(q)`, `cx(a, b)`) só valem como chamada; `i` e `theta` só valem no valor de uma variável `ghost`.
- **Projetos com C++ nativo** (por exemplo `ia_propria`, com as funções `ia_*` do backend): o `.cpp` gerado é gravado antes da parada. Os erros "funcao ... nao foi declarada" aparecem no lugar do erro do g++, e o alvo CMake do projeto continua compilando o `.cpp` com o backend.
- `GHOST_AST=0` desliga a AST e o verificador.
- **Testes:**
  - `tests/diag/tipos.g`: 14 erros num arquivo, mostrados de uma vez;
  - `tests/ghost/tipos_validos.g` (golden): herança, interfaces com método padrão, genéricos, sobrecarga, `static`, lambdas, compreensões e escopos, sem nenhum falso erro;
  - `ast_cobertura`: todos os programas válidos do projeto passam pelo verificador sem erro.

## Diferenças entre compilador e REPL

| Recurso | Compilador | REPL |
|---|---|---|
| `x = conv(imagem, kernel[, passo]);` e `x = transpose(a);` (forma de comando) | sim (`tests/ghost/conv_transpose.g`; voltou em 12/11/2026, depois de ter parado de compilar com a aposentadoria do Parser); imprime `[Ghost OS] Convolucao ...` / `[Ghost OS] Transposicao ...` | sim, com a mesma saída desde 13/11/2026 (`tests/*/conv_transpose.*`); o REPL também aceita `var m = img.conv2d(k, 1)` e `m.getRows()`/`getCols()` e deixou de imprimir eco `[Matematica]` |
| Classes (`class`, `extends`, `super`, `private`) | sim | sim, com a mesma saída (`tests/*/classes.*`) |
| Campos `qreg`/`qcircuit` em classes | sim | sim, com a mesma saída (`tests/*/classes_quantum.*`) |
| `set<T>`, compreensões de lista, `${x:.2f}` | sim | sim, com a mesma saída (`tests/*/colecoes.*`) |
| Interfaces (`interface`, `implements`) e membros `static` | sim | sim, com a mesma saída (`tests/*/interfaces.*`) |
| Classes genéricas (`class Pilha<T>`) | sim | sim, com a mesma saída (`tests/*/genericos.*`) |
| `import "x.g"` e `import "x.g" as m` | sim | sim (antes o REPL não tinha `import`), com a mesma saída (`tests/*/modulos.*`) |
| Funções genéricas (`function T maior<T>(...)`) | sim | sim, com a mesma saída (`tests/*/funcoes_genericas.*`) |
| Métodos genéricos (`obj.m<T>(...)`, `static`) | sim | sim, com a mesma saída (`tests/*/metodos_genericos.*`) |
| Restrições (`<T extends Animal>`, `<T implements A & B>`, `<T extends number>`) | sim (`static_assert`) | sim, com a mesma saída (`tests/*/restricoes.*`) |
| Lambdas, closures e `fn<Ret(Args)>` | sim | sim, com a mesma saída (`tests/*/lambdas.*`) |
| Lambdas avançadas (inferência em parâmetros, capturas `[&]`, nome direto de função genérica) | sim | sim, com a mesma saída (`tests/*/lambdas_avancado.*`) |
| Interfaces genéricas, métodos padrão e checagem em `list<Interface>` | sim | sim, com a mesma saída (`tests/*/interfaces_genericas.*`) |
| Compreensões avançadas (mapa `{k: v}`, conjunto `{expr}`, múltiplos `for` e `if`) | sim | sim, com a mesma saída (`tests/*/compreensoes_avancadas.*`) |
| Sobrecarga de funções e métodos de classes | sim | sim, com a mesma saída (`tests/*/nucleo_avancado.*`) |
| Membros `static` parametrizados em classes genéricas | sim | sim, com a mesma saída (`tests/*/nucleo_avancado.*`) |
| Modificador `protected:` em hierarquias profundas | sim | sim, com a mesma saída (`tests/*/nucleo_avancado.*`) |
| Fatiamento com passo `[start:end:step]` e operações vetoriais/matriciais em lote | sim | sim, com a mesma saída (`tests/*/colecoes_avancadas2.*`) |
| Operador de potência `**`, `bool(x)` e `foreach (q in reg)` | sim | sim, com a mesma saída (`tests/*/potencia_bool_qiter.*`) |
| Coleções estruturadas (`Stack<T>`, `Queue<T>`, `libs/collections.g`, `libs/test.g`) | sim | sim, com a mesma saída (`tests/*/colecoes_std.*`) |
| Interoperabilidade OpenQASM 3.0 (`to_qasm`, `from_qasm`, `libs/qasm.g`) | sim | sim, com a mesma saída (`tests/*/quantum_qasm.*`) |
| Sequências de escape em strings (`\"`, `\n`, `\t`, `\\`, `\xHH`) | sim (C++) | sim desde 13/11/2026 (`Lexer::unescape`); antes o REPL mostrava as barras (`tests/*/strings_escape.*`) |
| `texto.find(trecho)`, `json_extract`, `app.route(...)` | sim | sim desde 13/11/2026; `find` devolve -1 quando não acha (`tests/*/http_cliente.*`) |
| `ghost(x, y, p, theta2)` dentro de expressões | sim | sim desde 13/11/2026; a declaração `ghost g = ghost(...)` e o texto de `print(g)` iguais ao compilador desde 16/11/2026 |
| `for (x in ...)` com tipo (`var`, `int`...), `for x in ... { }` sem parênteses e `for` sobre matriz | sim | sim desde 16/11/2026 (antes o REPL só aceitava `foreach` e `for (x in ...)` sem tipo) (`tests/*/for_range.*`) |
| Ler `m(i, j)` de uma matriz numa expressão | sim (Número G) | sim desde 16/11/2026 (antes dava "funcao nao encontrada"); o REPL devolve a parte real |
| Modelos de IA: `text_model`, `transformer_model`, `transformer_model_v5`, `matrix_g`, `temporal_context`, `knowledge_consult` | sim | sim desde 13/11/2026: o REPL guarda o objeto C++ num registro (`src/InterpreterNative.cpp`) e a variável aponta para ele (`tests/*/ia_nativa.*`) |
| Simulador quântico na GPU (CUDA) | sim | sim, mesmo resultado na GPU e na CPU (`tests/*/quantum_gpu_resultado.*`) |
| Janela Win32 (`gui_create_window`, `gui_window_visible`, `gui_window_title`) | sim | sim (`tests/*/gui_janela_real.*`) |
| Manipulação de strings (`starts_with`, `ends_with`, `to_upper`, `to_lower`, `pad`, `replace`) | sim | sim, com a mesma saída (`tests/*/string_std.*`) |
| Estatística e matemática (`media`, `variancia`, `desvio_padrao`, `clamp`, `lerp`, `lcg_uniform`) | sim | sim, com a mesma saída (`tests/*/stats_std.*`) |
| Números complexos (`complex`, literal `4.0i`, constante `i`, raízes complexas de números negativos `csqrt`, `raiz_n_complexa`, sobrecarga `raiz_n(complex, n)`, `complex_abs/arg/conj/real/imag`) | sim | sim, com a mesma saída (`tests/*/complex_std.*`) |
| Criptografia básica (`crypto_base64_encode`, `crypto_base64_decode`, `libs/crypto.g`) | sim | sim, com a mesma saída (`tests/*/crypto_std.*`) |
| Manipulação de JSON (`json_parse`, `json_get`, `json_stringify`, `GhostJson`) | sim | sim, com a mesma saída (`tests/*/json_std.*`) |
| Datas e tempo (`now`, `parse_date`, `format_date`, `libs/time.g`) | sim | sim, com a mesma saída (`tests/*/tempo_std.*`) |
| Expressões regulares (`regex_match`, `regex_search`, `regex_replace`) | sim | sim, com a mesma saída (`tests/*/regex_std.*`) |
| Sistema de arquivos (`readfile`, `writefile`, `copyfile`, `deletefile`, `file_exists`, `libs/fs.g`) | sim | sim, com a mesma saída (`tests/*/fs_os_std.*`) |
| Sistema operacional e processos (`ghost_env_get`, `ghost_system_exec`, `libs/os.g`) | sim | sim, com a mesma saída (`tests/*/fs_os_std.*`) |
| Funções de ativação de IA (`activate(tensor, tanh/leaky_relu/gelu/softmax)`) | sim | sim, com a mesma saída (`tests/*/ativacoes_ia.*`) |
| Tipo incompatível em `fn<...>` ou retornos mistos sem `-> tipo` | erro de compilação do C++ | erro com mensagem na hora |
| Variável global fora de `main` usada dentro de lambda | acessa a variável global estática | no escopo principal do roteiro, captura por valor como variável local |
| `maior(3, 5.5)` (T com dois tipos) | erro do C++ ao compilar | erro com mensagem ao chamar |
| Erro dentro do corpo de uma classe genérica | ao compilar | quando `Pilha<int>` é usada pela primeira vez |
| Mapa literal sem tipo declarado (`{"a": 1}` no meio de uma expressão) | vira o texto JSON antigo (`{"a":1}`) | mapa (`{"a": 1}`); declare `map<K, V> m = {...}` para ter mapa nos dois |
| Tipo `ghost` (números hipercomplexos, `3.5g`, `2.5*i + 3.5*theta`) | sim | não |
| Inteiros acima de 10¹⁵ (`2 ** 62`) | exatos (`long long`) | guardados como `double`, mostrados em notação científica |
| `set<int> s = [1.5]`, `list<int> xs = [1.5]` | erro de compilação do C++ | converte para `int` |
| Uso indevido (`private`, campo ou método inexistente) | erro de compilação do C++ | erro com mensagem na hora |
| `print(objeto)` | não compila | mostra `Ponto{x: 2, y: 3}` |
| `result<T>`, `ok`, `err`, operador `?` | sim | sim, com a mesma saída (`tests/*/result.*`) |
| `result<T> r = 5;`, `result<int>` em `result<double>` | erro de compilação do C++ | erro com mensagem na hora |
| `print(ok(2))` sem variável tipada | não compila | mostra `ok(2)` |
| Chaves de `map` | qualquer tipo comparável | texto (números viram texto) |
| `T[]` e `list<T>` | `GhostList<T>` | lista dinâmica |
| Arrays C antigos (`int a[3] = {1, 2, 3};`) | — | continuam funcionando |

O resto (tipos primitivos e complexos `complex`, operadores inclusive potência `**`, conversão explícita `bool(x)`, `if`/`else if`, laços inclusive `foreach (q in reg)`, `break`/`continue`, `switch`, funções e métodos com sobrecarga, lambdas e closures com `fn<...>`, capturas explícitas `[&]`, inferência em lambdas, uso direto de função genérica como valor, coleções com fatiamento com passo `[start:end:step]` e operações vetoriais/matriciais em lote, estruturas `Stack<T>` e `Queue<T>`, compreensões de lista, mapa e conjunto com múltiplos `for` e `if`, classes com modificador `protected`, `try`/`catch`/`finally`/`throw`, `assert`, `result<T>`, `set<T>`, interfaces genéricas com métodos padrão, `static` em classes normais e genéricas parametrizadas, classes, funções e métodos genéricos com restrições, `hypothesis`, OpenQASM 3.0, criptografia base64, JSON estruturado, datas e tempo (`now`, `parse_date`, `format_date`), regex (`match`, `search`, `replace`), sistema de arquivos (`readfile`, `writefile`, `copyfile`, `deletefile`), comandos de sistema (`env`, `exec`), funções de ativação de redes neurais (`tanh`, `leaky_relu`, `gelu`, `softmax`), f-strings com formatação e qualquer expressão dentro de `${...}`) tem o mesmo resultado nos dois caminhos.
