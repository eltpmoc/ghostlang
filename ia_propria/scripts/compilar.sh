#!/usr/bin/env bash
set -euo pipefail
PROJECT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
ROOT=$(cd -- "$PROJECT/.." && pwd)
export GHOST_CXX=${GHOST_CXX:-$(command -v g++)}
cd -- "$PROJECT"
mkdir -p build checkpoints logs
if [[ -z ${GHOST_COMPILER:-} ]]; then
    # Compila os fontes originais dentro da IA, sem editar a linguagem.
    GHOST_COMPILER="$PROJECT/build/ghost"
    REBUILD=0
    [[ -x "$GHOST_COMPILER" ]] || REBUILD=1
    for SOURCE in "$ROOT"/src/*.cpp "$ROOT"/include/*.hpp; do
        [[ "$SOURCE" -nt "$GHOST_COMPILER" ]] && REBUILD=1
    done
    if [[ "$REBUILD" == 1 ]]; then
        "$GHOST_CXX" -std=c++17 -O2 -fopenmp -I"$ROOT/include" \
            "$ROOT/src/ghost_compiler.cpp" "$ROOT/src/Lexer.cpp" \
            "$ROOT/src/Parser.cpp" "$ROOT/src/Interpreter.cpp" \
            "$ROOT/src/GhostNumberG.cpp" -o build/ghost
    fi
fi
[[ -x "$GHOST_COMPILER" ]] || { echo 'Compilador Ghost nao encontrado.' >&2; exit 1; }
BUILD_MODE=${IA_BUILD:-auto}
case "$BUILD_MODE" in auto|cpu|gpu) ;; *) echo 'IA_BUILD deve ser auto, cpu ou gpu.' >&2; exit 1;; esac
export PATH="$PROJECT/scripts/toolchain:$PATH"
export IA_LINK_CUDA=0
STAGING=$(mktemp -d build/compilacao.XXXXXX)
# Caminhos relativos contornam a falta de quoting no comando emitido pelo Ghost.
"$GHOST_COMPILER" src/main.g -o "$STAGING/ia"
# O Ghost pode retornar 0 mesmo quando a ligacao falha; verificamos o artefato novo.
[[ -x "$STAGING/ia" ]] || { echo "Falha na compilacao; diagnostico em $STAGING" >&2; exit 1; }
mv -- "$STAGING/ia" build/ia
mv -- "$STAGING/ia_temp.cpp" build/ia_temp.cpp
printf '%s\n' cpu > build/backends.txt
if [[ "$BUILD_MODE" != cpu ]]; then
    NVCC=${NVCC:-$(command -v nvcc || true)}
    if [[ -z "$NVCC" && -x /usr/local/cuda/bin/nvcc ]]; then NVCC=/usr/local/cuda/bin/nvcc; fi
    CUDA_OK=0
    if [[ -n "$NVCC" && -x "$NVCC" ]]; then
        export IA_CUDA_HOME=${CUDA_HOME:-$(cd -- "$(dirname -- "$NVCC")/.." && pwd)}
        export IA_CUDA_OBJECT="$PROJECT/$STAGING/cuda.o"
        if "$NVCC" -std=c++17 -O3 --fmad=false -arch="${IA_CUDA_ARCH:-native}" \
            -ccbin "$GHOST_CXX" -c backend/cuda.cu -o "$IA_CUDA_OBJECT" > build/cuda-compilacao.log 2>&1; then
            if IA_LINK_CUDA=1 "$GHOST_COMPILER" src/main.g -o "$STAGING/ia_cuda" >> build/cuda-compilacao.log 2>&1; then
                if [[ -x "$STAGING/ia_cuda" ]]; then
                    mv -- "$STAGING/ia_cuda" build/ia_cuda
                    mv -- "$STAGING/ia_cuda_temp.cpp" build/ia_cuda_temp.cpp
                    printf '%s\n' cpu cuda > build/backends.txt
                    CUDA_OK=1
                    echo 'Backend CUDA compilado; o dispositivo sera testado ao executar.'
                fi
            fi
        fi
    fi
    if [[ "$CUDA_OK" == 0 ]]; then
        echo 'CUDA nao compilado. CPU disponivel; detalhes em build/cuda-compilacao.log.' >&2
        if [[ "$BUILD_MODE" == gpu ]]; then exit 1; fi
    fi
fi
# Intermediarios ficam dentro de build para diagnostico.
echo 'Pronto: bash scripts/ia.sh'
