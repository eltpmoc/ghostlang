# GhostLang WebAssembly (WASM) Demo

Este diretório contém a demonstração oficial de compilação do GhostLang para WebAssembly.

## Como compilar para WebAssembly

Com o compilador GhostLang e o Emscripten (emsdk) ativos:

```bash
# 1. Compilar o código Ghost para bundle WebAssembly (.html + .js + .wasm)
ghost build calculo.g --wasm -o calculo

# 2. Iniciar um servidor local para testar no navegador
python -m http.server 8080
# Ou via Node: npx serve
```

Abra `http://localhost:8080` no navegador para interagir diretamente com as funções GhostLang rodando no cliente via WebAssembly!

## Inspecionar C++ gerado para WebAssembly

Você pode inspecionar o código transpilado com as anotações `extern "C" EMSCRIPTEN_KEEPALIVE`:

```bash
ghost cpp --wasm calculo.g
```
