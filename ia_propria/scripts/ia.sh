#!/usr/bin/env bash
set -euo pipefail
PROJECT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd -- "$PROJECT"
export IA_DEVICE=${IA_DEVICE:-auto}
if [[ ${1:-} == --device ]]; then
    [[ $# -ge 2 ]] || { echo 'Use --device auto|cpu|gpu' >&2; exit 1; }
    export IA_DEVICE=$2
    shift 2
fi
case "$IA_DEVICE" in auto|cpu|gpu) ;; *) echo 'Dispositivo invalido: use auto, cpu ou gpu.' >&2; exit 1;; esac
[[ -x build/ia ]] || { echo 'Execute primeiro: bash scripts/compilar.sh' >&2; exit 1; }
BINARY=./build/ia
if [[ "$IA_DEVICE" != cpu && -x build/ia_cuda && -f build/backends.txt ]]; then
    if grep -qx cuda build/backends.txt; then BINARY=./build/ia_cuda; fi
fi
if [[ ${1:-} == --hardware ]]; then
    printf '[IA] CPU: %s; processadores disponiveis: %s\n' \
        "$(awk -F ': ' '/model name/{print $2; exit}' /proc/cpuinfo)" "$(nproc)"
    if command -v nvidia-smi >/dev/null; then
        timeout 5 nvidia-smi --query-gpu=name,memory.total,driver_version --format=csv,noheader || true
    fi
    shift
    set -- hardware "$@"
fi
export OMP_NUM_THREADS=${OMP_NUM_THREADS:-1}
export OMP_DYNAMIC=FALSE
exec "$BINARY" "$@"
