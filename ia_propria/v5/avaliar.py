#!/usr/bin/env python3
"""Geração pelos pesos V4; teste novo e diagnóstico legado ficam separados."""
import argparse
from collections import defaultdict
from datetime import datetime,timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile

HERE=Path(__file__).resolve().parent
APP=HERE.parent
spec=importlib.util.spec_from_file_location("diagnostico_v3",APP/"v3/testar_capacidades.py")
legacy=importlib.util.module_from_spec(spec); spec.loader.exec_module(legacy)
BIN=APP/"build/v4/ia"
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--checkpoint",default="checkpoints/v4/transformer_selecionado.bin")
    parser.add_argument("--device",choices=["cpu","cuda","auto"],default="cuda")
    parser.add_argument("--diagnostico",action="store_true")
    parser.add_argument("--seed",type=int,default=1000701692)
    parser.add_argument("--por-categoria",type=int,default=10)
    parser.add_argument("--particao",choices=["validacao","teste"],default="teste")
    args=parser.parse_args()
    if not 1 <= args.por_categoria <= 100: parser.error("Use --por-categoria entre 1 e 100.")
    checkpoint=APP/args.checkpoint
    cases=[]
    if args.diagnostico:
        cases=legacy.make_cases(args.seed)
    else:
        groups=defaultdict(list)
        for line in (HERE/"dados"/(args.particao+".jsonl")).read_text().splitlines():
            row=json.loads(line); groups[row["categoria"]].append(row)
        for category,rows in sorted(groups.items()):
            for n,row in enumerate(rows[:args.por_categoria]):
                cases.append(dict(id=f"{category}_{n}",categoria=category,prompt=row["prompt"],esperado=row["resposta"],criterio="short"))
    train=[json.loads(line)["prompt"] for path in (HERE/"dados").glob("treino*.jsonl") for line in path.read_text().splitlines()]
    train_normalized={legacy.normalize(q) for q in train}
    env=dict(os.environ,GHOST_DEVICE=args.device,OMP_NUM_THREADS="2",GHOST_STREAM_MS="0")
    records=[]
    with tempfile.TemporaryDirectory(prefix="ghost-v4-eval-") as temp:
        env["GHOST_KNOWLEDGE_DIR"]=temp
        for index,case in enumerate(cases):
            history=""; answers=[]; diagnostics=[]
            prompts=case.get("mensagens",[case.get("prompt")])
            for question in prompts:
                prompt=history+"Usuário: "+question+"\nAssistente:" if args.diagnostico else question
                result=subprocess.run([str(BIN),"gerar",str(checkpoint),prompt,"384"],cwd=APP,env=env,text=True,capture_output=True,timeout=90)
                if result.returncode: raise RuntimeError(result.stdout+result.stderr)
                answer=result.stdout.partition("\n")[2].strip(); answers.append(answer); diagnostics.append(result.stderr)
                history=prompt+" "+answer+"\n"
            answer=answers[-1]
            normalized=legacy.normalize(prompt)
            seen=normalized in train_normalized
            correct=legacy.matches(answer,case["esperado"],case["criterio"])
            records.append(dict(**case,resposta=answer,respostas=answers,correta=correct,avisos=diagnostics,prompt_no_treino=seen))
            if (index+1)%10==0: print(f"{index+1}/{len(cases)}",flush=True)
    summary={}
    for category in sorted({r["categoria"] for r in records}):
        rows=[r for r in records if r["categoria"]==category]
        summary[category]=dict(acertos=sum(r["correta"] for r in rows),total=len(rows),prompts_no_treino=sum(r["prompt_no_treino"] for r in rows))
    report=dict(data_utc=datetime.now(timezone.utc).isoformat(),dispositivo=args.device,checkpoint=str(checkpoint),
        sha256=hashlib.sha256(checkpoint.read_bytes()).hexdigest(),configuracao="greedy; limite 384 bytes; sem ferramentas, internet ou respostas prontas",
        conjunto="diagnostico_v3" if args.diagnostico else args.particao,
        limite="Amostra pequena; tarefas sintéticas. Respostas curtas com correção estrita. Controles conhecidos não demonstram generalização.",
        resumo=summary,respostas=records)
    folder=HERE/"relatorios"; folder.mkdir(exist_ok=True)
    name=("diagnostico" if args.diagnostico else args.particao)+"_"+datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%f")+".json"
    path=folder/name; path.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n")
    print(json.dumps(summary,ensure_ascii=False,indent=2)); print(path)
if __name__=="__main__": main()
