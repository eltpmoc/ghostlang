#!/usr/bin/env python3
"""Diagnostico local e reproduzivel; nao e teste de QI nem certificacao de inteligencia."""
import argparse
from collections import defaultdict
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import random
import re
import secrets
import subprocess
import tempfile
import time
import unicodedata

HERE = Path(__file__).resolve().parent
APP = HERE.parent
BIN = APP / "build/v3/ia"

def normalize(text):
    text = unicodedata.normalize("NFKD", text.casefold())
    text = "".join(c for c in text if not unicodedata.combining(c))
    return re.sub(r"\s+", " ", text).strip()

def clean_answer(text):
    return re.sub(r"^\[[^\]\n]+\]\s*", "", text.strip(), count=1)

def matches(answer, expected, kind):
    answer = clean_answer(answer)
    if kind == "control":
        return re.sub(r"\s+", "", normalize(answer)).startswith(re.sub(r"\s+", "", normalize(expected)))
    if kind == "tuple":
        answer = answer.strip().rstrip(".").strip("()[] ")
        return re.sub(r"\s+", "", answer) == expected
    return normalize(answer).rstrip(".!? ") == normalize(expected)

def make_cases(seed):
    rng = random.Random(seed)
    cases = []
    def add(category, steps, expected, kind="short"):
        cases.append(dict(id=f"{category}_{1+sum(c['categoria']==category for c in cases)}",
                          categoria=category, mensagens=steps, esperado=str(expected), criterio=kind))
    for _ in range(4):
        a, b, c = rng.randint(17, 83), rng.randint(11, 29), rng.randint(2, 6)
        add("calculo_novo", [f"Uma caixa tinha {a} peças, recebeu {b} e depois perdeu {c}. Quantas peças ficaram? Responda somente com o número."], a+b-c)
    for name, box in zip(["Lina", "Beto", "Mara", "Nilo"], ["A", "B", "C", "D"]):
        color = rng.choice(["azul", "verde", "vermelha"])
        add("deducao", [f"Neste exercício, toda peça {color} fica na caixa {box}. A peça de {name} é {color}. Em qual caixa fica essa peça? Responda somente com a letra da caixa."], box)
    for word in rng.sample(["azul", "verde", "casa", "texto", "modelo", "rede", "local", "dia"], 4):
        add("instrucoes", [f"Ignore assuntos anteriores. Responda somente a palavra {word}, sem explicações nem outras palavras."], word)
    for _ in range(4):
        values = rng.sample(range(12, 98), 3)
        names = ["Lina", "Beto", "Mara"]
        expected = names[values.index(max(values))]
        add("leitura_de_dados", [f"Considere apenas estes dados: Lina tem {values[0]} pontos; Beto tem {values[1]}; Mara tem {values[2]}. Quem tem mais pontos? Responda somente com o nome."], expected)
    for _ in range(4):
        value = rng.randint(103, 997)
        add("contexto_conversa",
            [f"Nesta conversa de teste, a caixa azul contém {value} moedas. Responda apenas OK.",
             "Quantas moedas a caixa azul contém, segundo a informação que acabei de fornecer? Responda somente com o número."], value)
    for _ in range(4):
        x, y, p, a, b, q = [rng.randint(1, 7) for _ in range(6)]
        expected = f"{x*a-y*b},{x*b+y*a},{x*q+a*p}"
        add("aplicacao_numeros_g",
            [f"Use a regra do produto G: x=x1*x2-y1*y2; y=x1*y2+y1*x2; p=x1*p2+x2*p1. Para G1=({x},{y},{p}) e G2=({a},{b},{q}), calcule (x,y,p). Responda somente os três inteiros separados por vírgulas."],
            expected, "tuple")
    cards = json.loads((HERE/"dados/conhecimento.json").read_text())
    for card_id in ["ghostlang", "arquitetura", "auto", "produto_g"]:
        card = next(c for c in cards if c["id"] == card_id)
        add("controle_conhecido", [card["perguntas"][0]], card["resposta"], "control")
    return cases

def run(command, env, data=None):
    started = time.monotonic()
    try:
        result = subprocess.run(command, cwd=APP, env=env, input=data, text=True,
                                capture_output=True, timeout=30)
        return dict(stdout=result.stdout, stderr=result.stderr, codigo=result.returncode,
                    segundos=round(time.monotonic()-started, 4))
    except subprocess.TimeoutExpired as error:
        return dict(stdout="", stderr="Tempo limite de 30 s.", codigo=None,
                    segundos=round(time.monotonic()-started, 4))

def evaluate(case, mode, checkpoint, env):
    raw = []
    answers = []
    if mode == "neural_direto":
        history = ""
        for question in case["mensagens"]:
            prompt = history + "Usuário: " + question + "\nAssistente:"
            result = run([str(BIN), "gerar", str(checkpoint), prompt, "384" if BIN.parent.name == "v4" else "128"], env)
            raw.append(result)
            # Runtime prints hardware on the first line; diagnostics remain on stderr.
            answer = result["stdout"].partition("\n")[2].strip()
            answers.append(answer)
            history = prompt + " " + answer + "\n"
            if result["codigo"] != 0:
                break
    else:
        data = "/internet off\n/stream off\n" + "\n".join(case["mensagens"]) + "\n/sair\n"
        result = run([str(BIN), "chat", str(checkpoint)], env, data)
        raw.append(result)
        parts = result["stdout"].split("Voce> ")[1:]
        answers = [p.strip() for p in parts[2:2+len(case["mensagens"])]]
    operational = all(r["codigo"] == 0 for r in raw) and len(answers) == len(case["mensagens"])
    answer = answers[-1] if answers else ""
    correct = matches(answer, case["esperado"], case["criterio"]) if operational else None
    return dict(**case, modo=mode, resposta=answer, respostas_por_turno=answers,
                correta_estrita=correct, execucao_ok=operational,
                diagnosticos=[r["stderr"] for r in raw if r["stderr"]],
                segundos=round(sum(r["segundos"] for r in raw), 4),
                falha_execucao=None if operational else raw)

def summarize(records):
    result = {}
    for mode in ["assistente_offline", "neural_direto"]:
        groups = defaultdict(lambda: dict(acertos=0, casos=0, falhas_execucao=0))
        for record in records:
            if record["modo"] != mode:
                continue
            group = groups[record["categoria"]]
            group["casos"] += 1
            group["acertos"] += record["correta_estrita"] is True
            group["falhas_execucao"] += not record["execucao_ok"]
        result[mode] = dict(groups)
    return result

def markdown(report):
    lines = ["# Diagnóstico de capacidades da IA", "", f"Data UTC: {report['data_utc']}",
             f"Semente: {report['semente']} | dispositivo: {report['dispositivo']}",
             "", report["limites"], "",
             "| Capacidade | Assistente offline | Pesos diretos |",
             "|---|---:|---:|"]
    summary = report["resumo"]
    for category, data in summary["assistente_offline"].items():
        neural = summary["neural_direto"][category]
        lines.append(f"| {category} | {data['acertos']}/{data['casos']} | {neural['acertos']}/{neural['casos']} |")
    lines += ["", "Controle conhecido mede recuperação de conteúdo familiar; fica separado das tarefas novas.",
              "O assistente offline usa regras e fichas locais. Pesos diretos usa o comando gerar, sem roteamento documental.",
              "Falhas técnicas são registradas separadamente; veja o JSON para saídas completas e avisos de vocabulário.",
              "", "## Respostas para inspeção", ""]
    for record in report["resultados"]:
        outcome = "ACERTO" if record["correta_estrita"] else ("ERRO" if record["execucao_ok"] else "FALHA TÉCNICA")
        lines += [f"### {record['id']} — {record['modo']} — {outcome}", "",
                  "Pergunta(s): " + " / ".join(record["mensagens"]), "",
                  "Esperado: " + record["esperado"], "", "Resposta:", ""]
        lines += ["> " + line for line in (record["resposta"] or "(vazia)").splitlines()]
        lines += [""]
    return "\n".join(lines)+"\n"

def main():
    global BIN
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed", type=int, default=None, help="Semente reproduzível; padrão: nova a cada execução")
    parser.add_argument("--device", choices=["cpu", "cuda", "auto"], default="cpu")
    parser.add_argument("--checkpoint", default=None)
    parser.add_argument("--versao", choices=["v3", "v4"], default="v3")
    parser.add_argument("--output-dir", type=Path, default=HERE/"avaliacoes")
    args = parser.parse_args()
    BIN = APP / ("build/"+args.versao+"/ia")
    if args.checkpoint is None:
        args.checkpoint = "checkpoints/v4/transformer_selecionado.bin" if args.versao == "v4" else "checkpoints/v3/ghost_g_v31.bin"
    checkpoint = Path(args.checkpoint)
    if not checkpoint.is_absolute():
        checkpoint = APP/checkpoint
    if not BIN.is_file() or not checkpoint.is_file():
        parser.error("Compile a IA e confira o caminho do checkpoint.")
    seed = secrets.randbelow(2**31) if args.seed is None else args.seed
    # Grader sanity checks: punctuation/spacing must not masquerade as reasoning errors.
    assert matches("36.", "36", "short")
    assert not matches("136", "36", "short")
    assert not matches("36 e 37", "36", "short")
    assert matches("(13, 33, 39)", "13,33,39", "tuple")
    assert not matches("(13,33,39,0)", "13,33,39", "tuple")
    assert matches("C + + transforma arquivos . g.", "C++ transforma arquivos .g.", "control")
    cases = make_cases(seed)
    corpus_files = [HERE/"dados/instrucoes_treino.txt", HERE/"dados/treino.txt"]
    if args.versao == "v4":
        corpus_files += list((APP/"v4/dados").glob("treino*.jsonl"))
    corpus = normalize("\n".join(p.read_text() for p in corpus_files if p.exists()))
    for case in cases:
        case["pergunta_encontrada_no_corpus"] = any(normalize(q) in corpus for q in case["mensagens"])
    if any(c["pergunta_encontrada_no_corpus"] for c in cases if c["categoria"] != "controle_conhecido"):
        parser.error("Uma pergunta nova já aparece no corpus conhecido. Escolha outra semente.")
    before = hashlib.sha256(checkpoint.read_bytes()).hexdigest()
    env = dict(os.environ, GHOST_DEVICE=args.device, GHOST_STREAM_MS="0", OMP_NUM_THREADS="2",
               GHOST_TEXT_TEMP="0", GHOST_TEXT_NO_REPEAT="0", GHOST_TEXT_REPETITION="1",
               HTTPS_PROXY="http://127.0.0.1:1", https_proxy="http://127.0.0.1:1",
               ALL_PROXY="http://127.0.0.1:1", NO_PROXY="", no_proxy="")
    records = []
    print(f"Semente {seed}; 24 tarefas novas e 4 controles conhecidos; internet desligada.", flush=True)
    with tempfile.TemporaryDirectory(prefix="ghost-capacidades-") as temp:
        for mode in ["assistente_offline", "neural_direto"]:
            print(f"Avaliando {mode}...", flush=True)
            for index, case in enumerate(cases):
                # Each case has an empty persistent memory; only its own conversational turns are retained.
                local_env = dict(env, GHOST_KNOWLEDGE_DIR=str(Path(temp)/mode/case["id"]))
                records.append(evaluate(case, mode, checkpoint, local_env))
                if (index+1) % 7 == 0:
                    print(f"  {index+1}/{len(cases)} casos", flush=True)
    after = hashlib.sha256(checkpoint.read_bytes()).hexdigest()
    if before != after:
        raise RuntimeError("O checkpoint mudou durante a avaliação.")
    output_limit = "384 bytes" if args.versao == "v4" else "128 tokens de palavra"
    report = dict(limite_saida=output_limit, data_utc=datetime.now(timezone.utc).isoformat(), semente=seed,
                  dispositivo=args.device, versao=args.versao, checkpoint=str(checkpoint), sha256_checkpoint=before,
                  corpus_verificado=[str(p) for p in corpus_files if p.exists()],
                  limites="Amostra pequena, correção estrita de respostas curtas, sem nota de QI. Perguntas novas no corpus conhecido não garantem ausência de tarefas semelhantes no treino. Respostas equivalentes podem exigir revisão manual. Não testa inteligência geral, criatividade aberta ou confiabilidade em produção.",
                  metodo="24 problemas gerados após o treino e 4 controles conhecidos; cada modo recebe os mesmos enunciados. Sem internet, memória isolada, geração determinística, limite de saída registrado separadamente. Contexto neural inclui respostas reais dos turnos anteriores.",
                  resumo=summarize(records), resultados=records)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    base = args.output_dir/f"capacidades_{stamp}_{seed}_{args.device}"
    base.with_suffix(".json").write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n")
    base.with_suffix(".md").write_text(markdown(report))
    print(json.dumps(report["resumo"], ensure_ascii=False, indent=2))
    print(f"Relatório: {base.with_suffix('.md')}")
    print(f"Respostas completas: {base.with_suffix('.json')}")
    if any(not r["execucao_ok"] for r in records):
        raise SystemExit(2)

if __name__ == "__main__":
    main()
