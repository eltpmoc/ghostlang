#!/usr/bin/env bash
set -euo pipefail
PROJECT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
ROOT=$(cd -- "$PROJECT/.." && pwd)
BUILD=${GHOST_BUILD_DIR:-"$ROOT/build_native"}
cmake -S "$ROOT" -B "$BUILD" -DGHOST_CUDA="${GHOST_CUDA:-AUTO}" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" --target ghost -j "${GHOST_BUILD_JOBS:-4}"
cd -- "$PROJECT"
mkdir -p build/v3 checkpoints/v3
python3 v3/preparar_dados.py
"$BUILD/ghost" v3/src/main.g -o build/v3/ia
echo 'V3 compilada. Execute: bash scripts/ia_v3.sh hardware'
