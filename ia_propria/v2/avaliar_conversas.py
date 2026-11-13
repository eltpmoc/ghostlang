#!/usr/bin/env python3
"""Avaliacao de respostas completas; nao confundir com acuracia de tokens."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("checkpoint")
parser.add_argument("--saida", default="logs/v2/conversas_avaliadas.json")
args = parser.parse_args()
project = Path(__file__).resolve().parent.parent
cases = json.loads((project / "v2/dados/conversas.json").read_text(encoding="utf-8"))
result = {"nota": "Corpus sintetico. Validacao usa perguntas diferentes com respostas ja vistas no treino.",
          "temperatura": 0, "resultados": []}
for fold, indexes in (("treino", (0, 1)), ("validacao", (2,))):
    examples = [(case["perguntas"][i], case["resposta"]) for case in cases for i in indexes]
    prompts = "\n".join(q for q, _ in examples) + "\n/sair\n"
    process = subprocess.run(["bash", "scripts/ia_v2.sh", "chat", args.checkpoint], cwd=project,
                             env=dict(os.environ, V2_TEMP="0", V2_HISTORY="0", V2_GENERATION_SEED="42"),
                             input=prompts, text=True, capture_output=True, timeout=300, check=True)
    answers = re.findall(r"Voce> IA> (.*?)(?=\nVoce> |\Z)", process.stdout, flags=re.S)
    if len(answers) != len(examples):
        raise RuntimeError(f"Esperadas {len(examples)} respostas, recebidas {len(answers)}")
    exact = 0
    for (question, expected), answer in zip(examples, answers):
        match = " ".join(answer.split()) == " ".join(expected.split())
        exact += match
        result["resultados"].append(dict(conjunto=fold, pergunta=question, esperada=expected,
                                         gerada=answer.strip(), exata=match))
    result[fold] = dict(exatas=exact, total=len(examples), proporcao=exact / len(examples))
    print(f"{fold}: {exact}/{len(examples)} respostas exatas ({100*exact/len(examples):.1f}%)")
destination = Path(args.saida)
if not destination.is_absolute():
    destination = project / destination
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(f"Relatorio: {destination}")
