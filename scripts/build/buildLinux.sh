#!/bin/bash

echo "=== GhostLang Linux Build Script ==="
echo "[1] Preparando diretorio de build..."

# Usa uma pasta temporária no sistema de arquivos local para contornar 
# o erro de 'Operation not supported' do CMake em pastas de rede e 
# erros do compilador 'make' quando o caminho possui ':' (ex: smb-share:server=)
PROJ_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$PROJ_DIR" || exit
BUILD_DIR="/tmp/ghost_build_linux_$$"

mkdir -p "$BUILD_DIR"
# Copia apenas os arquivos necessários para a pasta temporária (evita copiar .exe antigos)
cp -r "$PROJ_DIR/src" "$PROJ_DIR/include" "$PROJ_DIR/CMakeLists.txt" "$BUILD_DIR/" 2>/dev/null

cd "$BUILD_DIR" || exit

echo "[2] Gerando os arquivos Makefiles com o CMake..."
# Compila o projeto totalmente isolado no /tmp
cmake .
if [ $? -ne 0 ]; then
    echo "Erro ao gerar os arquivos do CMake."
    cd "$PROJ_DIR"
    rm -rf "$BUILD_DIR"
    exit 1
fi


echo "[3] Compilando os executaveis..."
# Utiliza todos os nucleos disponiveis para compilar mais rapido
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "=== Compilacao finalizada com sucesso! ==="
    
    # Prepara a pasta de destino no projeto
    cd "$PROJ_DIR"
    rm -rf build_linux
    mkdir -p build_linux
    
    # Move os executaveis para a pasta do projeto
    mv "$BUILD_DIR"/ghost "$BUILD_DIR"/ghost_ask "$BUILD_DIR"/ghost_node "$BUILD_DIR"/ghost_swarm "$BUILD_DIR"/ghost_api build_linux/ 2>/dev/null
    
    echo "Os executaveis foram gerados dentro da pasta 'build_linux/'."

    echo "Para executar, use: ./build_linux/ghost <argumentos>";
    echo "Para testar, use: ./build_linux/ghost_ask <argumentos>";
    echo "Para testar, use: ./build_linux/ghost_node <argumentos>";
    echo "Para testar, use: ./build_linux/ghost_swarm <argumentos>";
    echo "Para testar, use: ./build_linux/ghost_api <argumentos>";
    echo "Para testar, use: ./build_linux/ghost_swarm <argumentos>";
    
else
    echo "=== Falha durante a compilacao. ==="
fi

# Limpa a pasta temporária do sistema
rm -rf "$BUILD_DIR"
