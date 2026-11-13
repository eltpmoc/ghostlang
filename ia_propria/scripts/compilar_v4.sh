#!/usr/bin/env bash
set -eo pipefail
PROJECT=$(cd -- "$(dirname -- "$0")/.." && pwd)
ROOT=$(cd -- "$PROJECT/.." && pwd)
[[ -n "$GHOST_BUILD_DIR" ]] || GHOST_BUILD_DIR="$ROOT/build_native"
[[ -n "$GHOST_CUDA" ]] || GHOST_CUDA=AUTO
[[ -n "$GHOST_BUILD_JOBS" ]] || GHOST_BUILD_JOBS=4
cmake -S "$ROOT" -B "$GHOST_BUILD_DIR" -DGHOST_CUDA="$GHOST_CUDA" -DCMAKE_BUILD_TYPE=Release
cmake --build "$GHOST_BUILD_DIR" --target ghost -j "$GHOST_BUILD_JOBS"
cd -- "$PROJECT"
mkdir -p build/v4 checkpoints/v4
"$GHOST_BUILD_DIR/ghost" v4/src/main.g -o build/v4/ia
echo 'V4 compilada. Execute bash scripts/ia_v4.sh autoteste.'
