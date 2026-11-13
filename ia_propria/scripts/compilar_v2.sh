#!/usr/bin/env bash
set -euo pipefail
PROJECT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
ROOT=$(cd -- "$PROJECT/.." && pwd)
cd -- "$PROJECT"
mkdir -p build/v2 checkpoints/v2 logs/v2
export GHOST_CXX=${GHOST_CXX:-$(command -v g++)}
COMPILER=build/v2/ghost
REBUILD=0
[[ -x "$COMPILER" ]] || REBUILD=1
for SOURCE in "$ROOT"/src/*.cpp "$ROOT"/include/*.hpp; do
    [[ "$SOURCE" -nt "$COMPILER" ]] && REBUILD=1
done
if [[ "$REBUILD" == 1 ]]; then
    "$GHOST_CXX" -O2 -std=c++17 -fopenmp -I"$ROOT/include" \
        "$ROOT/src/ghost_compiler.cpp" "$ROOT/src/Lexer.cpp" "$ROOT/src/Parser.cpp" \
        "$ROOT/src/Interpreter.cpp" "$ROOT/src/GhostNumberG.cpp" -o "$COMPILER"
fi
MODE=${IA_BUILD:-auto}
case "$MODE" in auto|cpu|gpu) ;; *) echo 'IA_BUILD invalido' >&2; exit 1;; esac
export PATH="$PROJECT/scripts/v2-toolchain:$PATH"
export IA_LINK_CUDA=0
STAGING=$(mktemp -d build/v2/compilacao.XXXXXX)
"$COMPILER" v2/src/main.g -o "$STAGING/ia"
[[ -x "$STAGING/ia" ]] || { echo 'Falha ao compilar V2 CPU' >&2; exit 1; }
mv -- "$STAGING/ia" build/v2/ia
mv -- "$STAGING/ia_temp.cpp" build/v2/ia_temp.cpp
printf '%s\n' cpu > build/v2/backends.txt
if [[ "$MODE" != cpu ]]; then
    NVCC=${NVCC:-$(command -v nvcc || true)}
    if [[ -z "$NVCC" && -x /usr/local/cuda/bin/nvcc ]]; then NVCC=/usr/local/cuda/bin/nvcc; fi
    CUDA_OK=0
    if [[ -n "$NVCC" && -x "$NVCC" ]]; then
        export IA_CUDA_HOME=${CUDA_HOME:-$(cd -- "$(dirname -- "$NVCC")/.." && pwd)}
        export IA_CUDA_OBJECT="$PROJECT/$STAGING/cuda.o"
        if "$NVCC" -std=c++17 -O3 --fmad=false -arch="${IA_CUDA_ARCH:-native}" -ccbin "$GHOST_CXX" \
            -c v2/backend/cuda.cu -o "$IA_CUDA_OBJECT" > build/v2/cuda-compilacao.log 2>&1; then
            if IA_LINK_CUDA=1 "$COMPILER" v2/src/main.g -o "$STAGING/ia_cuda" >> build/v2/cuda-compilacao.log 2>&1; then
                if [[ -x "$STAGING/ia_cuda" ]]; then
                    mv -- "$STAGING/ia_cuda" build/v2/ia_cuda
                    mv -- "$STAGING/ia_cuda_temp.cpp" build/v2/ia_cuda_temp.cpp
                    printf '%s\n' cpu cuda > build/v2/backends.txt
                    CUDA_OK=1
                fi
            fi
        fi
    fi
    if [[ "$CUDA_OK" == 0 ]]; then
        echo 'V2 CUDA nao compilada; CPU disponivel. Veja build/v2/cuda-compilacao.log.' >&2
        if [[ "$MODE" == gpu ]]; then exit 1; fi
    fi
fi
echo 'V2 pronta: bash scripts/ia_v2.sh hardware'
