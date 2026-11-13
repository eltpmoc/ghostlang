#!/usr/bin/env python3
"""Comparacao numerica CPU/CUDA e selecao automatica no hardware local."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

PROJECT = Path(__file__).resolve().parents[1]


def run(mode, *args, visible=None, ok=True):
    env = dict(os.environ, IA_DEVICE=mode, OMP_NUM_THREADS="1")
    if visible is not None:
        env["CUDA_VISIBLE_DEVICES"] = visible
    result = subprocess.run(["bash", "scripts/ia.sh", *map(str, args)], cwd=PROJECT,
                            env=env, capture_output=True, text=True, timeout=120)
    if (result.returncode == 0) != ok:
        raise AssertionError(result.stdout + result.stderr)
    return result


def check(condition, message):
    if not condition:
        raise AssertionError(message)


auto = run("auto", "hardware")
check("Dispositivo: GPU CUDA" in auto.stderr and "autoteste OK" in auto.stderr,
      "O hardware local deve executar CUDA neste teste.")
cpu = run("cpu", "hardware")
check("Dispositivo: CPU" in cpu.stderr, "Modo CPU ignorado.")
fallback = run("auto", "hardware", visible="")
check("Dispositivo: CPU" in fallback.stderr, "Fallback sem GPU falhou.")
forced = run("gpu", "hardware", visible="", ok=False)
check("Erro de backend" in forced.stderr, "GPU forcada nao deve mascarar falha.")
run("invalido", "hardware", ok=False)

with tempfile.TemporaryDirectory(prefix="backends-", dir=PROJECT / "build") as tmp:
    folder = Path(tmp)
    train, validation = folder / "treino", folder / "validacao"
    train.write_text("ola mundo. " * 5)
    validation.write_text("ola mundo. ola!\n")
    paths = {}
    for mode in ("cpu", "gpu"):
        paths[mode] = folder / mode
        result = run(mode, "treinar", train, validation, 3, "0.003", paths[mode])
        check(("Dispositivo: GPU CUDA" if mode == "gpu" else "Dispositivo: CPU") in result.stderr,
              "Treinamento usou backend errado.")
    cpu_weights = list(map(float, paths["cpu"].read_text().splitlines()[4:]))
    gpu_weights = list(map(float, paths["gpu"].read_text().splitlines()[4:]))
    max_error = max(abs(a - b) for a, b in zip(cpu_weights, gpu_weights))
    check(max_error < 1e-10, f"Adam/gradientes CPU e GPU divergem: {max_error}")
    for checkpoint in paths.values():
        measures = []
        generations = []
        for mode in ("cpu", "gpu"):
            result = run(mode, "avaliar", validation, checkpoint)
            measures.append(float(re.search(r"MSE=([0-9.e+-]+)", result.stdout)[1]))
            generations.append(run(mode, "gerar", checkpoint, "ola ", 24).stdout)
        check(abs(measures[0] - measures[1]) < 1e-10, "Checkpoint nao e portavel entre backends.")
        check(generations[0] == generations[1], "Inferencia GPU difere da CPU.")
    print(f"OK: CUDA real, auto/CPU/GPU, fallback, erros e checkpoints cruzados; "
          f"diferenca maxima dos pesos CPU/CUDA = {max_error:.3g}.")
