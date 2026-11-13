#!/bin/bash
# GhostLang macOS Build Script (Clang + OpenMP / Metal)

echo "=== GhostLang macOS Build Script ==="
echo "[1] Preparando diretorio de build..."

PROJ_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$PROJ_DIR" || exit
BUILD_DIR="/tmp/ghost_build_macos_$$"

mkdir -p "$BUILD_DIR"
cp -r "$PROJ_DIR/src" "$PROJ_DIR/include" "$PROJ_DIR/CMakeLists.txt" "$BUILD_DIR/" 2>/dev/null

cd "$BUILD_DIR" || exit

echo "[2] Configurando CMake com compilador Clang nativo do Apple LLVM..."
export CC=clang
export CXX=clang++

cmake -DCMAKE_BUILD_TYPE=Release -DGHOST_CUDA=OFF .
if [ $? -ne 0 ]; then
    echo "Erro ao gerar os arquivos do CMake."
    cd "$PROJ_DIR"
    rm -rf "$BUILD_DIR"
    exit 1
fi

echo "[3] Compilando os executaveis nativos GhostLang no macOS..."
NPROC=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
make -j"$NPROC"

if [ $? -eq 0 ]; then
    echo "=== Compilacao finalizada com sucesso no macOS! ==="
    
    cd "$PROJ_DIR"
    rm -rf build_macos
    mkdir -p build_macos
    
    mv "$BUILD_DIR"/ghost "$BUILD_DIR"/ghost_ask "$BUILD_DIR"/ghost_node "$BUILD_DIR"/ghost_swarm "$BUILD_DIR"/ghost_api build_macos/ 2>/dev/null
    
    echo "Os executaveis foram gerados dentro da pasta 'build_macos/'."
    echo "Para executar, use: ./build_macos/ghost <argumentos>"
    
    rm -rf "$BUILD_DIR"
    exit 0
else
    echo "Falha na compilacao no macOS."
    cd "$PROJ_DIR"
    rm -rf "$BUILD_DIR"
    exit 1
fi
