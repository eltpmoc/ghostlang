# GhostLang 👻

[![GhostLang CI](https://github.com/eltpmoc/ghostlang/actions/workflows/ci.yml/badge.svg)](https://github.com/eltpmoc/ghostlang/actions/workflows/ci.yml)
[![Licença: MIT](https://img.shields.io/badge/licen%C3%A7a-MIT-blue.svg)](LICENSE)

Linguagem de programação compilada (transpilada para C++17) voltada a **computação quântica**, **IA/deep learning** e à **álgebra hipercomplexa G** (os Números Fantasmas de 4 componentes). O mesmo programa roda compilado (`ghost run`) ou no REPL interativo (`ghost`), com a mesma saída.

> **Estado:** versão 0.9, em desenvolvimento. Testada no **Windows x64** (MSYS2 UCRT64, GCC 16.2): 203 testes no CTest. **Linux e macOS ainda não funcionam** (o CI os roda como experimentais); são as próximas etapas. A maturidade de cada recurso, com a evidência, está em [`dashboard/ghostlang-status.json`](dashboard/ghostlang-status.json) (painel em `dashboard/index.html`).

## Um exemplo

```ghost
import "libs/quantum.g";

function main() {
    // Estado de Bell: |00> e |11> com 50% cada
    qreg q(2);
    q.h(0);
    q.cx(0, 1);
    print(q.prob_state(0), " ", q.prob_state(3));

    // for no estilo do Python
    for i in range(1, 4) {
        print("i = ", i);
    }

    // Número G (x, yi, p, θ²)
    ghost g = ghost(1.0, 2.0, 0.5, 0.25);
    print(g);
}
```

## O que a linguagem tem

- **Núcleo:** tipos com inferência, classes com herança e `private`/`protected` conferidos, interfaces, genéricos com restrições, lambdas e closures, `result<T>` com `?`, `try/catch`, `switch`, `for ... in` com `range`, compreensões, `set`/`map`, f-strings, módulos (`import`), threads (`spawn`, `mutex`).
- **Quântico:** registradores `qreg` com portas, medição e observáveis; `circuit` com inverso e controlado automáticos; algoritmos (Grover, QFT, GHZ); ruído e matriz densidade; OpenQASM 2/3; simulação na GPU com CUDA (quando disponível).
- **IA:** `network` treinada com `optimize` (Adam, SGD, momentum, RMSProp, GD), autograd, convolução, `qcircuit`/`qlayer` (QML: VQC, QNN, kernels, VQE/QAOA), redes híbridas, modelo de texto e Transformers nativos.
- **Álgebra G:** Números Fantasmas 4D, matrizes `gate`/`matrix` e tensores N-dimensionais (com OpenBLAS quando ligado).
- **Biblioteca padrão (`libs/`):** arquivos, strings, JSON, regex, tempo, SQLite, HTTP (cliente e servidor) e WebSocket (chat e jogos em tempo real), GUI Win32, estatística, testes.
- **Ferramentas:** `ghost check` (verificador de tipos), `ghost fmt`, `ghost lsp` (Language Server), `ghost debug` e `ghost debug --dap` (depurador com o gdb, ligado à extensão VS Code), `ghost install sqlite` (pacotes pelo índice `pacotes/indice.json`) e `ghost pm` (pacotes de pasta ou git, com semver).

Ainda **não** implementados: backends Vulkan, Metal e ROCm; envio de circuitos a QPUs reais (o `qhw_submit` simula localmente e diz isso); registro remoto de pacotes (com publicação; o índice é um arquivo); WebSocket seguro (`wss://`).

## Compilar a partir do código (Windows)

Pré-requisito: [MSYS2](https://www.msys2.org/) com o ambiente UCRT64.

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-libgomp mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-gdb
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build -j 4
```

Depois: `build/ghost.exe run examples/ola_mundo.g`, ou `build/ghost.exe` para o REPL.

Opcionais:
- **OpenBLAS** (multiplicação de tensores mais rápida): `powershell -ExecutionPolicy Bypass -File scripts\build-openblas.ps1`; o CMake liga sozinho.
- **Pacote portátil** (traz o GCC, o gdb e o instalador, roda sem MSYS2): `powershell -ExecutionPolicy Bypass -File scripts\package_release.ps1` gera `dist/GhostLang-Portable-x64.zip`.

## Documentação

- [`docs/MANUAL.md`](docs/MANUAL.md): manual do programador (sintaxe, bibliotecas, tabelas de maturidade).
- [`docs/GhostLang_Guia_Oficial_do_Programador.pdf`](docs/GhostLang_Guia_Oficial_do_Programador.pdf) (e `.html`): guia com capítulos e exercícios.
- [`docs/SPEC.md`](docs/SPEC.md): especificação da linguagem.
- [`docs/NUCLEO.md`](docs/NUCLEO.md): núcleo, arquitetura do compilador e diferenças entre compilador e REPL.
- [`docs/ROADMAP_GHOSTLANG_COMPLETA.md`](docs/ROADMAP_GHOSTLANG_COMPLETA.md): roadmap (cabeçalho gerado do dashboard).

Os exemplos `ghost` do Manual, do SPEC e do Guia passam no `ghost check` como parte do CTest.

## Estrutura

| Pasta | Conteúdo |
|---|---|
| `src/`, `include/` | compilador (lexer, AST, verificador de tipos, gerador de C++), REPL e runtime |
| `libs/` | biblioteca padrão escrita em GhostLang |
| `tests/` | goldens do compilador, roteiros do REPL, diagnósticos, oráculo do C++ gerado, testes da CLI |
| `examples/` | exemplos (cada um passa no `ghost check` no CTest) |
| `vscode-ghostlang/` | extensão do VS Code (sintaxe, snippets, LSP e depurador) |
| `dashboard/` | estado de maturidade de cada recurso |
| `ia_propria/` | IA escrita em GhostLang (experimental) |
| `scripts/` | empacotamento, instalador, OpenBLAS, gdb e scripts de documentação |

## Licença

[MIT](LICENSE) © 2026 Elton Alves Pereira.

Componentes de terceiros mantêm as próprias licenças: o SQLite (`src/sqlite3.c`) é de domínio público; o OpenBLAS, quando compilado e embutido no runtime, é BSD-3 (a licença vai em `third_party/openblas/LICENSE` e em `docs/licencas/` no pacote portátil); o GCC e as bibliotecas do MinGW-w64 levados no pacote portátil seguem as licenças do projeto MSYS2.
