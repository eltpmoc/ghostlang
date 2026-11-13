#!/usr/bin/env bash
set -euo pipefail
PROJECT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd -- "$PROJECT"
export GHOST_DEVICE=${GHOST_DEVICE:-auto}
if [[ ${1:-} == --device ]]; then
    [[ $# -ge 2 ]] || { echo 'Use --device auto|cpu|cuda' >&2; exit 1; }
    export GHOST_DEVICE=$2
    shift 2
fi
case "$GHOST_DEVICE" in auto|cpu|cuda|gpu) ;; *) echo 'Dispositivo invalido' >&2; exit 1;; esac
[[ -x build/v3/ia ]] || { echo 'Execute bash scripts/compilar_v3.sh' >&2; exit 1; }
export OMP_NUM_THREADS=${OMP_NUM_THREADS:-4}
export GHOST_TEXT_TEMP=${GHOST_TEXT_TEMP:-0}
export GHOST_TEXT_NO_REPEAT=${GHOST_TEXT_NO_REPEAT:-0}
export GHOST_TEXT_REPETITION=${GHOST_TEXT_REPETITION:-1}
exec ./build/v3/ia "$@"
