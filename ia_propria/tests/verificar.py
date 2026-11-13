#!/usr/bin/env python3
"""Verificacao externa; treinamento e inferencia permanecem em GhostLang."""
import csv
import os
from pathlib import Path
import re
import subprocess
import tempfile

PROJECT = Path(__file__).resolve().parents[1]
DEVICE = os.environ.get("IA_TEST_DEVICE", "cpu")
BINARY = PROJECT / ("build/ia_cuda" if DEVICE == "gpu" else "build/ia")
ENV = dict(os.environ, IA_DEVICE=DEVICE, OMP_NUM_THREADS="1", OMP_DYNAMIC="FALSE")


def run(*args, ok=True):
    result = subprocess.run(
        [str(BINARY), *map(str, args)], cwd=PROJECT, env=ENV,
        text=True, capture_output=True, timeout=120,
    )
    if (result.returncode == 0) != ok:
        raise AssertionError(f"{args}\n{result.stdout}\n{result.stderr}")
    return result.stdout


def require(condition, message):
    if not condition:
        raise AssertionError(message)


with tempfile.TemporaryDirectory(prefix="teste-", dir=PROJECT / "build") as tmp:
    directory = Path(tmp)
    train = directory / "treino.txt"
    validation = directory / "validacao.txt"
    checkpoint = directory / "modelo.txt"
    train.write_text("ola mundo. " * 12, encoding="utf-8")
    validation.write_text("ola mundo. ola mundo. ola!\n", encoding="utf-8")
    output = run("treinar", train, validation, 8, "0.003", checkpoint)
    initial = float(re.search(r"MSE inicial treino: ([0-9.e+-]+)", output)[1])
    with Path(str(checkpoint) + ".metricas.csv").open() as handle:
        metrics = list(csv.DictReader(handle))
    final = float(metrics[-1]["mse_treino"])
    require(final < initial * 0.6, f"Aprendizado insuficiente: {initial} -> {final}")
    measured = run("avaliar", validation, checkpoint)
    restored = float(re.search(r"MSE=([0-9.e+-]+)", measured)[1])
    require(abs(restored - float(metrics[-1]["mse_validacao"])) < 1e-10,
            "O checkpoint nao preservou pesos e biases.")
    generated = run("gerar", checkpoint, "olá ", 24).splitlines()[-1]
    require(generated.startswith("ola ") and len(generated) == 28,
            "Prefixo normalizado ou quantidade de caracteres incorretos.")
    require(run("gerar", checkpoint, "ola ", 24).splitlines()[-1] == generated,
            "A geracao gulosa deve ser deterministica.")
    normalized = run("normalizar", "AÇÃO, Café! 123").splitlines()[-1]
    require(normalized == "acao, cafe! 123", "Falha de normalizacao UTF-8.")
    repeat = directory / "repetido.txt"
    run("treinar", train, validation, 8, "0.003", repeat)
    require(checkpoint.read_bytes() == repeat.read_bytes(), "Semente nao reproduzivel.")
    resumed = directory / "continuado.txt"
    run("continuar", train, validation, 1, "0.003", checkpoint, resumed)
    require(resumed.read_bytes() != checkpoint.read_bytes(), "Continuacao nao treinou.")
    before = checkpoint.read_bytes()
    run("treinar", train, validation, 1, "0.003", checkpoint, ok=False)
    require(checkpoint.read_bytes() == before, "Checkpoint existente foi sobrescrito.")
    for bad in ("0", "-2", "abc", "3.5", "201"):
        run("treinar", train, validation, bad, "0.003", directory / "erro", ok=False)
    run("treinar", train, train, 1, "0.003", directory / "erro", ok=False)
    run("gerar", directory / "ausente", "ola", 5, ok=False)
    run("gerar", checkpoint, "ola", 0, ok=False)
    run("avaliar", directory / "ausente", checkpoint, ok=False)
    broken = directory / "corrompido.txt"
    broken.write_text("GHOST_TEXT_V1\n168\n32\n42\n")
    run("gerar", broken, "ola", 5, ok=False)
    lines = checkpoint.read_text().splitlines()
    lines[4] = "nan"
    broken.write_text("\n".join(lines) + "\n")
    run("gerar", broken, "ola", 5, ok=False)
    train.write_text("   \n   ")
    run("treinar", train, validation, 1, "0.003", directory / "erro", ok=False)
    print(f"OK: aprendizado {initial:.8f} -> {final:.8f}; persistencia, geracao, "
          "normalizacao, reproducibilidade, continuacao e rejeicao de entradas invalidas.")
