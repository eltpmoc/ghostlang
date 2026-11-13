# Computação quântica na GhostLang (F3)

O tipo `qreg` é um registrador quântico com simulação **statevector** nativa (runtime C++, OpenMP). Ele está em `include/GhostQuantum.hpp` e `src/GhostQuantum.cpp`.

```ghost
import "libs/quantum.g";

function main() {
    qreg q(2);              // |00>
    q.seed(42);             // medições reprodutíveis
    q.h(0);
    q.cx(0, 1);             // par de Bell
    print(q);               // 0.7071|00> + 0.7071|11>
    print(q.draw());        // circuito em ASCII
    print(q.histogram(1000));
    print(f"<ZZ> = ${q.expect("ZZ")}");
    int r = q.measure(0);   // mede e colapsa
}
```

## Convenções

- O qubit 0 é o **bit menos significativo**. Kets são escritos `|q(n-1)...q0>`, então `|01>` significa q0 = 1.
- Strings de Pauli seguem a mesma ordem: `"XZ"` aplica X no q1 e Z no q0.
- As amplitudes são `std::complex<double>`. A álgebra G não é usada nas amplitudes porque o produto G não é associativo, e a física precisa de unitariedade exata.
- O limite é de 28 qubits (2^28 amplitudes, 4 GB de RAM). A partir de 2^13 amplitudes, o OpenMP paraleliza.

## Referência

| Grupo | Métodos |
|---|---|
| Estado | `size()` `reset()` `seed(n)` `set_basis(i)` `amplitude(i)` `norm()` |
| 1 qubit | `h x y z s sdg t tdg sx` · `rx(q,θ) ry(q,θ) rz(q,θ) p(q,φ) u(q,θ,φ,λ)` · `h_all() x_all()` |
| Controladas | `cx/cnot cy cz ch` · `cp(c,t,φ) crx cry crz` · `swap(a,b)` · `ccx/toffoli(c1,c2,t)` · `cswap(c,a,b)` · `mcx(c1,c2,...,t)` / `mcx([controles],t)` |
| Algoritmos | `qft() iqft() qft_range(início,n) iqft_range(...)` · `phase_flip(i)` (oráculo) · `diffuser()` (Grover) |
| Medição | `measure(q)` · `measure_all()` · `histogram(shots)` · `counts(shots)` (retorna `map<string, int>` no C++ e no REPL) |
| Observáveis | `prob(q)` = P(q=1) · `prob_state(i)` · `expect_z(q)` · `expect("ZZI")` |
| Visualização | `print(q)` / `show(q)` (ket) · `probabilities()` · `draw()` |

A constante `PI` está disponível. Em funções, `qreg` é passado por referência: as portas aplicadas dentro da função alteram o registrador de quem chamou.

**Iteração de qubits com `foreach`:** você pode iterar diretamente sobre um registrador quântico `qreg` nos dois caminhos (compilador e REPL):
```ghost
qreg q(4);
foreach (qubit in q) {
    q.h(qubit); // aplica Hadamard em cada qubit: 0, 1, 2, 3
}
```
Também é aceito `foreach (int q in reg)` com tipo explícito.

## Blocos `circuit { }`

Um `circuit` é um sub-circuito com nome e parâmetros, escrito uma vez e aplicado em qualquer `qreg` ou `qcircuit`. No corpo, as portas são chamadas **sem objeto**: elas atuam no registrador em que o circuit foi aplicado.

```ghost
circuit emaranhar(a, b) {          // parâmetros sem tipo: números
    h(a);
    cx(a, b);
}

circuit cadeia_ghz(int n) {        // int ou double também podem ser declarados
    h(0);
    for (int i = 0; i < n - 1; i = i + 1) {
        cx(i, i + 1);
    }
}

circuit passo_grover(int alvo) {   // um circuit pode chamar outro
    phase_flip(alvo);
    diffuser();
}

circuit camada(int n) {            // portas treináveis: serve para qcircuit
    for (int i = 0; i < n; i = i + 1) { ry_train(i); }
    for (int i = 0; i < n - 1; i = i + 1) { cx(i, i + 1); }
}

qreg q(5);
q.emaranhar(0, 1);         // forma de método
emaranhar(q, 2, 3);        // forma de função: o registrador vem primeiro
q.cadeia_ghz(5);

qcircuit h2(2);
h2.camada(2); h2.camada(2);   // ansatz montado com circuits; minimize() treina os 4 parâmetros
```

- **No corpo podem entrar:** portas e operações de `qreg`/`qcircuit` (incluindo `*_train`, `*_param`, `encode_*`, `measure`), outros circuits (também recursivos), `for`/`if`/`while`, variáveis `int`/`double` e `print`.
- **Onde declarar:** no nível principal do arquivo (fora de funções e classes), em qualquer posição. Um circuit pode ser usado antes da linha em que é declarado, inclusive em `import`.
- **Erros detectados pela GhostLang:**
  - número de argumentos errado (`circuit bell espera 2 argumento(s)...`);
  - chamada que não é porta nem circuit;
  - nome de circuit igual a uma porta (`circuit h(...)`);
  - comandos não permitidos no corpo (`qreg`, `function`...).
- **Compilador:** cada circuit vira um template C++ `template <class Q, ...> void nome(Q& q, ...)`. Por isso o mesmo circuit funciona com `GhostQReg` e `GhostQCircuit`, sem custo extra em tempo de execução, inclusive na GPU.
- **REPL:** a mesma sintaxe funciona. Os parâmetros ficam num escopo próprio, sombreiam variáveis e registradores de mesmo nome e somem no fim da chamada. O limite de recursão é 64 níveis.
- **`q.draw()`:** mostra as portas já expandidas, porque o circuit é aplicado porta a porta.
- **Laços:** `i++`, `i += 1` e `i = i + 1` funcionam (ver `docs/NUCLEO.md`).

### Inverso automático: `nome_dg`

Todo circuit ganha o seu inverso (adjunto, U†), com o sufixo `_dg` como em `sdg`/`tdg`. Ele aplica as portas inversas em ordem reversa: (G₁…Gₖ)† = Gₖ†…G₁†.

```ghost
circuit somar(int n, int k) {      // somador de Draper: |x> -> |x + k mod 2^n>
    qft();
    double ang = PI * k;
    for (int j = n - 1; j >= 0; j = j - 1) { p(j, ang); ang = ang / 2.0; }
    iqft();
}

qreg q(4);
q.set_basis(5);
q.somar(4, 3);        // |8>
q.somar_dg(4, 6);     // o inverso subtrai: |2>
somar_dg(q, 4, 2);    // forma de função também funciona

circuit conjugar(a, b) {           // dentro de um circuit: U · T · U†
    emaranhar(a, b);
    t(b);
    emaranhar_dg(a, b);
}
```

- **Exato, inclusive a fase global:**
  - S† = Sdg e T† = Tdg;
  - SX† = SX³;
  - U(θ,φ,λ)† = U(−θ,−λ,−φ);
  - rotações, fases, RZZ e controladas com o ângulo negado;
  - QFT† = IQFT;
  - as demais portas são as próprias inversas.

  O teste `circuit_inverse` confere U·U† = I com 500 portas aleatórias de todos os tipos, com diferença < 1e-10.
- **Como funciona:**
  - Em `qreg`, o corpo é executado sobre um gravador (`GhostQTape`, em `include/GhostQTape.hpp`), que só anota as portas, sem simular. Depois as portas inversas são aplicadas no registrador.
  - Em `qcircuit`, o corpo é gravado numa cópia leve do circuito, e o inverso nega ângulos fixos e escalas de parâmetros e de features. Por isso ele também inverte `ansatz`, `encode_*` e `zz_feature_map`.
- **Portas treináveis no inverso:** `ry_train` e afins criam **parâmetros novos**, como em toda chamada de circuit, porque cada chamada de `c.camada(2)` também cria os seus. Para U(θ)·U(θ)† com os mesmos θ, use parâmetros compartilhados (`add_param` + `ry_param(q, indice)`): os parâmetros que já existiam antes da chamada são reaproveitados no inverso.
- **Não reversíveis:** `measure`, `measure_all` e `reset` dentro de um circuit invertido geram erro, e o registrador não é alterado.
- **Nomes terminados em `_dg`** são reservados: `circuit bell_dg(...)` dá erro.

### Versão controlada: `nome_ctrl(controle, ...)`

Todo circuit também ganha `nome_ctrl` e `nome_dg_ctrl`. O primeiro argumento é o qubit de controle, e o circuit só age quando esse qubit está em 1: |0⟩⟨0|⊗I + |1⟩⟨1|⊗U.

```ghost
circuit fase(int alvo, double th) { p(alvo, th); }

// Estimativa de fase (QPE): U = P(2π·0,375) no autoestado |1> do q3
qreg qpe(4);
qpe.x(3);
for (int j = 0; j < 3; j = j + 1) { qpe.h(j); }
int reps = 1;
for (int j = 0; j < 3; j = j + 1) {
    for (int r = 0; r < reps; r = r + 1) { qpe.fase_ctrl(j, 3, 2.0 * PI * 0.375); }  // U^(2^j) controlado por q_j
    reps = reps * 2;
}
qpe.iqft_range(0, 3);
print(qpe);                       // 1|1011>: contagem 011 = 3 -> fase 3/8

q.emaranhar_ctrl(2, 0, 1);        // controle q2
q.emaranhar_dg_ctrl(2, 0, 1);     // inverso controlado
emaranhar_ctrl(q, 2, 0, 1);       // forma de função
```

- **Como funciona:** o corpo é gravado num `GhostQTape` e reaplicado com uma **máscara de controle** no `qreg`. Toda porta passa pelo mesmo kernel com máscara, que já existia para `cx`/`ccx`, inclusive na GPU. `swap` e `cswap` usam a máscara direto, e `rzz` controlado vira CX·RZ·CX.
- **Fase exata:** o controle usa a matriz exata de cada porta, então a fase global de U vira fase relativa, como deve ser. O teste `circuit_control` compara com a referência por subespaço (amplitudes com controle 0 intactas, com controle 1 recebendo U) com 400 portas aleatórias, dois controles aninhados e o inverso controlado. Também compara GPU e CPU com 15 qubits.
- **Controle aninhado:** um circuit que chama `outro_ctrl(...)` e é chamado com `_ctrl` fica com dois controles, e assim por diante.
- **Validação antes de aplicar** (em caso de erro, o registrador não muda):
  - nenhuma porta pode usar o qubit de controle;
  - `h_all`, `x_all`, `qft()`, `iqft()`, `phase_flip` e `diffuser` atuam no registrador inteiro e não podem ser controlados (use `qft_range`/`iqft_range` sem o controle);
  - `measure`/`reset` também dão erro.
- **Só em `qreg`:** em `qcircuit` dá erro claro, porque rotações treináveis controladas precisam de outra regra de gradiente (parameter-shift de 4 termos).
- **Nomes terminados em `_ctrl`** são reservados.

Exemplos: `tests/ghost/quantum_circuit.g` (Bell, GHZ, Grover e VQE do H₂ com ansatz feito de circuits; somador por QFT e o seu inverso) e `tests/repl/circuit.ghost` (CTest `repl_circuit`). O teste do runtime fica em `tests/cpp/circuit_inverse.cpp` (CTest `circuit_inverse`).

## No REPL

Rodando `ghost` sem argumentos, `qreg` e `qcircuit` funcionam com os mesmos métodos do compilador. A GPU também é usada no modo `auto`.

```text
ghost> qreg q(2);
ghost> q.h(0); q.cx(0, 1);
ghost> print(q);
0.7071|00> + 0.7071|11>
ghost> print(f"<ZZ> = ${q.expect("ZZ")}");
<ZZ> = 1
ghost> int r = q.measure(0);
ghost> string d = q.draw();
ghost> qcircuit h2(2); h2.ansatz(2); h2.observable("ZZ", 1.0);
ghost> print(h2.minimize(100, 0.1));
```

- **Métodos que devolvem número:** podem entrar em expressões, por exemplo `q.prob(0) * 100` ou `if (q.measure(0) == 1) { ... }`.
- **Métodos que devolvem texto** (`draw`, `histogram`, `ket`, `params`, `maxcut_solution`...): vão em `print`, f-strings ou `string s = ...`.
- **Funções globais:** `quantum_backend()` e `quantum_device("cpu" | "cuda" | "auto")`.
- **F-strings no REPL:** agora aceitam variáveis string e objetos (`${nome}`, `${q}`).
- **Campos de classe:** `class Par { qreg q(2); function void preparar() { bell(q, 0, 1); } }` funciona no REPL e no compilador, com a mesma saída (`tests/ghost/classes_quantum.g`). Detalhes em `docs/NUCLEO.md`, seção Classes.
- **Limitação:** `qlayer` e redes híbridas continuam só no compilador.

Teste: `tests/repl/quantum.ghost` (CTest `repl_quantum`) roda o roteiro na entrada padrão e confere a saída.

## GPU (CUDA)

O mesmo `qreg` roda na GPU, sem mudar o código:

```ghost
print(quantum_backend());   // "CUDA: NVIDIA GeForce GTX 1050 Ti, 4096 MB, sm_61" ou "CPU (...)"
qreg q(20);                 // "auto": 14+ qubits vão para a GPU se houver backend
print(q.device_name());     // "cuda" ou "cpu"
q.device("cpu");            // move o estado para a CPU; q.device("cuda") exige GPU
quantum_device("cuda");     // padrão para os próximos qreg/qcircuit ("cpu", "cuda" ou "auto")
```

- **Backend:** `ghost_quantum_cuda.dll` no Windows (`libghost_quantum_cuda.so` no Linux), compilado de `src/cuda/GhostQuantumCuda.cu` com uma **API em C**. O runtime é compilado com MinGW e o nvcc exige MSVC; a API em C evita misturar as ABIs de C++. Ele é carregado em tempo de execução, e sem ele (ou sem GPU) tudo roda na CPU.
- **O que roda na GPU:** portas de 1 qubit e controladas, Toffoli, SWAP/CSWAP, RZZ, oráculo, difusor de Grover, probabilidades, norma, colapso da medição e ⟨Pauli⟩ (calculado direto, sem copiar o estado). Ket, contagens, `measure_all` e amplitudes usam a cópia na CPU, que é sincronizada só quando precisa ser lida.
- **Seleção:** variável `GHOST_QDEVICE=cpu|cuda|auto`. O caminho da biblioteca pode ser forçado por `GHOST_QCUDA_LIB`.
- **Compilação de programas:** o `ghost.exe` copia a DLL para junto do executável de programas que usam `qreg`, `qcircuit` ou `qlayer`.
- **Build da DLL:** `scripts/build/build_quantum_cuda.ps1`, também chamado no fim do `buildWin.ps1`.
  - Usa CUDA 12.x, porque o 13.x não gera mais código para Pascal (sm_61).
  - Usa o toolset MSVC 14.4x, porque o nvcc 12.9 trava (`cudafe++`) com os headers do MSVC 14.5 do VS 18.
  - Gera sm_61/75/86/89, mais PTX.
- **Memória:** 2ⁿ × 16 bytes na GPU, além da mesma quantidade na RAM. Numa GPU de 4 GB, o limite prático é 27 qubits.

**Medido na GTX 1050 Ti** (`tests/cpp/quantum_cuda.cpp`, camadas de RY + CNOT):

| Qubits | Portas | CPU (OpenMP) | GPU | Aceleração |
|---|---|---|---|---|
| 20 | 390 | 0,82 s | 0,11 s | 7,3× |
| 24 | 470 | 14,9 s | 2,1 s | 7,1× |

O teste `quantum_cuda` confere que **CPU e GPU dão o mesmo estado** (diferença < 1e-10) em:
- 300 portas aleatórias + QFT, para 3, 8 e 15 qubits;
- probabilidades, ⟨Pauli⟩ aleatórios e medições com a mesma seed;
- cópia, movimento e troca de dispositivo;
- Grover com 16 qubits e GHZ com 22;
- valor esperado e gradiente do `qcircuit`.

Sem backend, o teste imprime SKIP.

Exemplo: `tests/ghost/quantum_gpu.g`, com Grover em 20 qubits e GHZ em 22. O programa leva 0,97 s na GPU e 3,0 s na CPU.

## Biblioteca `libs/quantum.g`

`bell(q, a, b)`, `ghz(q)`, `grover(q, marcado)`, `grover_iterations(n)`, `deutsch_jozsa(n, tipo)` e `qft_de(q, valor)`.

O `import` procura o arquivo na pasta de quem importa, depois na pasta atual e por fim na raiz da GhostLang. Cada arquivo é importado uma única vez.

## Testes

- `tests/cpp/quantum_runtime.cpp` (CTest `quantum_runtime`) cobre:
  - portas básicas, Bell e GHZ;
  - unitariedade, com 400 portas aleatórias seguidas das inversas;
  - QFT contra a fórmula analítica e Grover com P > 0,95;
  - medição reprodutível, erros, `draw()` e GHZ com 20 qubits.
- `tests/ghost/quantum_bell.g` e `tests/ghost/quantum_algoritmos.g` são exemplos completos em GhostLang.

## QML: circuitos treináveis (`qcircuit`, F4)

O tipo `qcircuit` guarda um circuito parametrizado (`include/GhostQML.hpp`). Ele aceita três tipos de porta:

- **fixas:** `h x y z s t cx cz swap rx ry rz`;
- **treináveis (θ):** `rx_train`, `ry_train`, `rz_train`, `ansatz(camadas)` e, para reutilizar um parâmetro, `ry_param(q, i)`;
- **codificação de dados:** `encode_ry(q, feature, escala)` e `encode_all()`.

Os gradientes são **exatos**, calculados pela regra parameter-shift: dE/dθ = [E(θ+π/2) − E(θ−π/2)]/2. O otimizador é Adam.

```ghost
// VQE: estado fundamental do H2
import "libs/quantum.g";
qcircuit h2(2);
h2.ansatz(2);
hamiltoniano_h2(h2);              // soma de Paulis via observable("ZZ", coef)
double e = h2.minimize(300, 0.05); // -1.85727 Ha (exata: -1.857275)

// Classificador híbrido: saída = w * <Z0>(x) + b
qcircuit m(2);
m.encode_all();
m.ansatz(2);
m.add_sample(1, 2.0, 1.5);        // rótulo, x0, x1 (ou m.load_csv("dados.csv"))
m.fit(60, 0.1);                   // Adam em θ, w e b
int classe = m.predict(0.3, 0.5);
```

| Grupo | Métodos |
|---|---|
| Observável | `observable("XZ", coef)` · `clear_observable()` · `observable_string()` (padrão: Z no q0) |
| Avaliação | `expect(x0, ...)` · `energy()` · `state(x0, ...)` (ket) · `draw()` |
| VQE | `minimize(passos, lr)` |
| Classificador | `add_sample(rótulo, x...)` · `load_csv(arq)` · `fit(épocas, lr)` · `predict(x...)` · `score(x...)` · `accuracy()` · `loss()` · `set_verbose(k)` |
| Parâmetros | `seed(n)` (antes de criar os parâmetros) · `params()` · `param(i)` · `set_param(i, v)` · `save(arq)` · `load(arq)` |

Os rótulos podem ser 0/1 ou −1/+1, e o `predict` devolve no mesmo formato. O treino roda as amostras em paralelo com OpenMP.

**Testes** (`tests/cpp/qml_runtime.cpp`, CTest `qml_runtime`):
- o parameter-shift coincide com diferenças finitas (erro < 1e-6), inclusive com parâmetro compartilhado;
- o VQE do H₂ chega a menos de 1e-4 da energia obtida por diagonalização exata;
- o classificador passa de 90% de acurácia;
- save/load preserva o modelo, e os erros têm mensagens claras.

Exemplos: `tests/ghost/qml_classificador.g` (de 59% para 97%) e `tests/ghost/qml_vqe.g`.

## Redes híbridas: `qlayer` dentro de `network >>`

```ghost
gate X(2, 64);  gate Y(1, 64);   // uma amostra por coluna (preencha com X(0, k) = ... ou load)
gate W1(2, 2);                   // densa 2 -> 2
qlayer Q(2, 2);                  // quântica: 2 qubits, 2 camadas de ansatz
gate W2(1, 2);                   // densa 2 -> 1
Q.seed(3);
network rede = W1 >> Q >> W2;    // vira GhostHybridNetwork quando há uma qlayer
rede => optimize(X, Y, epochs=150, lr=0.05, opt="adam");
print(f"acuracia ${rede.accuracy(X, Y)}");
int classe = rede.classify(0.0, 0.0);
```

- **qlayer:** as n entradas viram ângulos RY, passam pelo ansatz treinável e saem como ⟨Z⟩ de cada qubit (n saídas em [−1, 1]).
- **Ativações:**
  - ReLU entre duas camadas densas;
  - nenhuma antes da qlayer (a saída densa vira ângulo);
  - a saída da qlayer entra direto na camada seguinte;
  - a última camada é linear.
- **Backprop:** nas camadas densas usa a regra da cadeia; na qlayer usa parameter-shift tanto para θ quanto para as entradas, então os gradientes atravessam a camada quântica.
- **Treino:** em lote (média sobre as colunas de X/Y), com amostras em paralelo (OpenMP). O otimizador é `opt="adam"` ou `"gd"`. A perda é MSE.
- **Métodos da rede:** `loss(X, Y)`, `accuracy(X, Y)` (limiar 0,5 com alvos 0/1, 0 com ±1, argmax com várias saídas), `predict(x...)`, `classify(x...)`, `summary()`, `set_verbose(k)` e `rede * X`.
- **Dimensões:** são conferidas na montagem da rede, por exemplo: "a camada 2 espera 2 entradas, mas a camada 1 produz 3".
- **Redes só clássicas** continuam usando a `GhostNetwork` original.

**Testes** (`tests/cpp/hybrid_runtime.cpp`, CTest `hybrid_runtime`):
- os gradientes da qlayer (entradas e θ) coincidem com diferenças finitas;
- o backprop da rede densa → densa (ReLU) → qlayer → densa coincide com diferenças finitas em todos os parâmetros;
- a rede separa o círculo x² + y² < 0,45 com acurácia ≥ 90%;
- dimensões incompatíveis geram erro.

Exemplo: `tests/ghost/qml_rede_hibrida.g`, que vai de 75% para 100% em 150 épocas.

## QAOA para MaxCut

```ghost
qcircuit grafo(6);                 // 1 qubit por vértice
grafo.edge(0, 1);  grafo.edge(1, 2, 2.5);   // aresta (a, b, peso = 1)
grafo.qaoa(3);                     // monta o circuito do zero: H; por camada RZZ(-w·γ) + RX(2β)
grafo.minimize(150, 0.05);         // energia = -corte esperado
string s = grafo.maxcut_solution(2000);     // melhor corte entre as amostras
print(f"${grafo.expected_cut()} de ${grafo.maxcut_exact()}; ${s} -> ${grafo.cut_of(s)}");
```

- **Observável:** −C, com C = Σ w (1 − Z_a Z_b)/2, então `minimize()` maximiza o corte esperado.
- **Parâmetros:** γ e β são compartilhados por camada, com escala (`rzz_param(a, b, θ, escala)` e `rx_param_scaled`). O parameter-shift considera a escala.
- **Inicialização:** "adiabática", com γ crescendo e β decrescendo ao longo das camadas.
- **Bits:** a solução é lida como `|q(n−1)...q0>`, e o bit indica o lado do corte.

Exemplo `tests/ghost/qml_qaoa.g`, com um anel de 6 vértices e 2 cordas: o corte esperado vai de 5,98 para 7,80 (de 8), razão 0,975, e a amostragem encontra o ótimo 8.

## Kernels quânticos

```ghost
qcircuit k(2);
k.zz_feature_map(1, 1.0);           // H; RZ(2x_q); RZZ(2 x_i x_j) em cada par
double v = k.kernel(x0, x1, y0, y1); // |<φ(x)|φ(y)>|²
k.add_sample(rotulo, x0, x1);  k.add_test(rotulo, x0, x1);
k.kernel_fit(0.001);                 // kernel ridge: (K + λI)α = y
print(k.kernel_test_accuracy());
```

- `encode_rzz(a, b, i, j, escala)` codifica o produto x_i·x_j. Os gradientes de entrada desse termo também funcionam na `qlayer`.
- **Métodos:** `kernel_matrix(n)` (texto), `kernel_score`, `kernel_predict`, `kernel_accuracy` (treino) e `kernel_test_accuracy`. O `test_accuracy()` mede o classificador variacional (`fit`) no mesmo conjunto de teste.
- **Classificador:** usa *kernel ridge* (mínimos quadrados regularizados), e não SVM com margem.
- **Escolha do feature map:** no XOR com 2 qubits, 1 repetição e escala até 1,5 dão 100% em treino e teste. Com 2 repetições, ou escala ≥ 2, o kernel perde posto e nem o ajuste de treino chega a 100%. Mais repetições não é sempre melhor, e `kernel_fit` devolve a acurácia de treino justamente para diagnosticar isso.

**Testes** (`tests/cpp/qaoa_kernel_runtime.cpp`, CTest `qaoa_kernel_runtime`):
- fases da porta RZZ;
- parameter-shift com escala, parâmetro compartilhado e x_i·x_j comparado com diferenças finitas;
- MaxCut ponderado por força bruta;
- QAOA com razão ≥ 0,8 e amostragem atingindo o ótimo;
- kernel com k(x,x) = 1, simétrico, em [0,1] e matriz de Gram semidefinida positiva;
- XOR com teste ≥ 90%.

## Interoperabilidade OpenQASM 3.0

Tanto `GhostQCircuit` quanto `GhostQReg` suportam exportação e importação de programas quânticos no formato padrão OpenQASM 3.0 (`OPENQASM 3;`), disponível tanto no compilador quanto no REPL via biblioteca modular `libs/qasm.g` ou métodos diretos:

```ghost
import "libs/quantum.g";
import "libs/qasm.g";

function void main() {
    qcircuit c(2);
    c.h(0);
    c.cx(0, 1);
    string codigo = c.to_qasm();     // exporta o circuito como string OpenQASM 3.0
    print(codigo);

    qcircuit c2(2);
    c2.load_qasm(codigo);            // importa e reconstrói as portas
    print(c2.to_qasm());
}
```

O analisador de OpenQASM 3.0 suporta portas fixas (`h`, `x`, `y`, `z`, `s`, `t`, `sdg`, `tdg`), portas controladas (`cx`, `cnot`, `cz`, `swap`, `ccx`), e rotações parametrizadas (`rx(θ)`, `ry(θ)`, `rz(θ)`).

## Ainda não feito

Circuit controlado em `qcircuit` (parameter-shift de 4 termos), `qlayer` no REPL, ruído e matriz densidade, SVM de margem (QSVM clássico) e autograd genérico (o backprop híbrido atual é específico das camadas densa e qlayer).
