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
case "$IA_DEVICE" in auto|cpu|gpu) ;; *) echo 'Dispositivo invalido' >&2; exit 1;; esac
[[ -x build/v2/ia ]] || { echo 'Execute bash scripts/compilar_v2.sh' >&2; exit 1; }
BINARY=./build/v2/ia
if [[ "$IA_DEVICE" != cpu && -x build/v2/ia_cuda && -f build/v2/backends.txt ]]; then
    if grep -qx cuda build/v2/backends.txt; then BINARY=./build/v2/ia_cuda; fi
fi
export OMP_NUM_THREADS=${OMP_NUM_THREADS:-1}
exec "$BINARY" "$@"
