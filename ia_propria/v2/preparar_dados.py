#!/usr/bin/env python3
"""Corpus de demonstracao autoral. Sem downloads nem dados privados."""
import json
from pathlib import Path

folder = Path(__file__).resolve().parent / "dados"
cases = json.loads((folder / "conversas.json").read_text(encoding="utf-8"))
train, validation = [], []
for case in cases:
    prompts = case["perguntas"]
    if len(prompts) != 3 or not case["resposta"].strip():
        raise ValueError("Cada exemplo precisa de tres perguntas e uma resposta.")
    for i, prompt in enumerate(prompts):
        document = f"Usuário: {prompt}\nAssistente: {case['resposta']}"
        (train if i < 2 else validation).append(document)
if set(train) & set(validation):
    raise ValueError("Sobreposicao exata entre treino e validacao.")
for name, data in (("treino.txt", train), ("validacao.txt", validation)):
    (folder / name).write_text("\n\n".join(data) + "\n", encoding="utf-8")
print(f"Corpus preparado: {len(train)} conversas de treino, {len(validation)} de validacao.")
print("As respostas sao compartilhadas entre variantes. Isto testa novas formulacoes, nao novos conhecimentos.")
