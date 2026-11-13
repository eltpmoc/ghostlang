#!/usr/bin/env python3
"""Testa gradientes, aprendizado, retomada, UTF-8, isolamento e fallback."""
import csv
import os
from pathlib import Path
import re
import subprocess
import tempfile

PROJECT = Path(__file__).resolve().parents[1]
DEVICE = os.environ.get("GHOST_TEST_DEVICE", "cpu")
ENV = dict(os.environ, GHOST_DEVICE=DEVICE, GHOST_TEXT_TEMP="0", GHOST_TEXT_NO_REPEAT="3", OMP_NUM_THREADS="1")


def run(*args, ok=True, env=None, stdin=None):
    result = subprocess.run(["bash", "scripts/ia_v3.sh", *map(str, args)],
                            cwd=PROJECT, env=env or ENV, input=stdin, capture_output=True,
                            text=True, timeout=180)
    if (result.returncode == 0) != ok:
        raise AssertionError(f"{args}\n{result.stdout}\n{result.stderr}")
    return result


def require(condition, message):
    if not condition:
        raise AssertionError(message)


run("autoteste")
with tempfile.TemporaryDirectory(prefix="testes-", dir=PROJECT / "build/v3") as tmp:
    folder = Path(tmp)
    train, validation = folder / "treino", folder / "validacao"
    train.write_text("Usuário: Olá!\nAssistente: Olá! Como posso ajudar?\n\n"
                     "Usuário: Bom dia!\nAssistente: Bom dia! Como posso ajudar?\n", encoding="utf-8")
    validation.write_text("Usuário: Olá! Bom dia!\nAssistente: Olá! Como posso ajudar?\n", encoding="utf-8")
    checkpoint = folder / "completo"
    output = run("treinar", train, validation, 12, "0.005", checkpoint).stdout
    initial = float(re.search(r"CE inicial validacao: ([0-9.e+-]+)", output)[1])
    metrics = list(csv.DictReader(Path(str(checkpoint) + ".metricas.csv").open()))
    best = min(initial, *(float(row["ce_validacao"]) for row in metrics))
    require(best < initial * 0.7, f"Nao houve aprendizado suficiente: {initial} -> {best}")
    measured = float(re.search(r"CE=([0-9.e+-]+)", run("avaliar", validation, checkpoint).stdout)[1])
    require(abs(measured - best) < 2e-6, "Nao selecionou ou preservou o melhor checkpoint.")
    generated = run("gerar", checkpoint, "Usuário: Olá!\nAssistente:", 30).stdout
    require(generated == run("gerar", checkpoint, "Usuário: Olá!\nAssistente:", 30).stdout,
            "A semente de geracao nao reproduziu a resposta.")
    generated.encode("utf-8", errors="strict")
    require("<UNK>" not in generated and "<BOS>" not in generated, "Gerou tokens reservados.")
    run("chat", checkpoint, stdin="Olá!\n/neural\nOlá!\n/fontes\n/limpar\n/sair\n")
    run("chat", checkpoint, stdin="")  # EOF precisa encerrar sem loop.
    split, resumed = folder / "parcial", folder / "retomado"
    run("treinar", train, validation, 6, "0.005", split)
    run("continuar", train, validation, 6, "0.005", str(split)+".ultimo", resumed)
    if DEVICE == "cpu":
        require(Path(str(checkpoint)+".ultimo").read_bytes() == Path(str(resumed)+".ultimo").read_bytes(),
                "Retomada nao preservou pesos, Adam, RNG e passos.")
    else:
        full_ce = float(re.search(r"CE=([0-9.e+-]+)", run("avaliar", validation, str(checkpoint)+".ultimo").stdout)[1])
        resumed_ce = float(re.search(r"CE=([0-9.e+-]+)", run("avaliar", validation, str(resumed)+".ultimo").stdout)[1])
        require(abs(full_ce-resumed_ce)<1e-4, "Retomada CUDA divergiu alem da tolerancia.")
    before = checkpoint.read_bytes()
    run("treinar", train, validation, 1, "0.005", checkpoint, ok=False)
    require(before == checkpoint.read_bytes(), "Sobrescreveu checkpoint existente.")
    broken = folder / "corrompido"
    payload = bytearray(before); payload[len(payload)//2] ^= 1
    broken.write_bytes(payload)
    run("info", broken, ok=False)
    broken.write_bytes(before[:60])
    run("info", broken, ok=False)
    run("info", folder / "ausente", ok=False)
    run("treinar", train, train, 1, "0.005", folder / "erro", ok=False)
    run("treinar", train, validation, "1.5", "0.005", folder / "erro", ok=False)
    run("treinar", train, validation, 1, "nan", folder / "erro", ok=False)
    broken.write_bytes(b"\xff")
    run("avaliar", broken, checkpoint, ok=False)
    run("gerar", checkpoint, "ola", 0, ok=False)
    invalid = dict(ENV, GHOST_TEXT_TEMP="nan")
    run("gerar", checkpoint, "ola", 4, env=invalid, ok=False)
    if DEVICE == "cuda":
        cpu = dict(ENV, GHOST_DEVICE="cpu")
        cpu_ce = float(re.search(r"CE=([0-9.e+-]+)", run("avaliar", validation, checkpoint, env=cpu).stdout)[1])
        require(abs(cpu_ce-measured)<1e-5, "Checkpoint CUDA incompativel com CPU.")
        fallback = run("hardware", env=dict(ENV, GHOST_DEVICE="auto", CUDA_VISIBLE_DEVICES=""))
        require("CPU" in fallback.stdout, "Fallback automatico falhou.")
        run("hardware", env=dict(ENV, GHOST_DEVICE="cuda", CUDA_VISIBLE_DEVICES=""), ok=False)
    print(f"OK V3 {DEVICE}: CE {initial:.6f} -> {best:.6f}; gradientes, retomada, melhor modelo, "
          "UTF-8, chat/EOF, entradas invalidas e checkpoints verificados.")
