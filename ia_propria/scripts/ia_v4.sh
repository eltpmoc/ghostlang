#!/usr/bin/env bash
set -eo pipefail
PROJECT=$(cd -- "$(dirname -- "$0")/.." && pwd)
cd -- "$PROJECT"
[[ -n "$GHOST_DEVICE" ]] || GHOST_DEVICE=auto
if [[ "$1" == --device ]]; then
    [[ $# -ge 2 ]] || { echo 'Use --device auto|cpu|cuda' >&2; exit 1; }
    GHOST_DEVICE=$2
    shift 2
fi
case "$GHOST_DEVICE" in auto|cpu|cuda|gpu) ;; *) echo 'Dispositivo invalido' >&2; exit 1;; esac
[[ -n "$OMP_NUM_THREADS" ]] || OMP_NUM_THREADS=4
export GHOST_DEVICE OMP_NUM_THREADS
[[ -x build/v4/ia ]] || { echo 'Execute bash scripts/compilar_v4.sh' >&2; exit 1; }
exec ./build/v4/ia "$@"
