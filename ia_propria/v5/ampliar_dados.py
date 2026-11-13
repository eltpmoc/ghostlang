#!/usr/bin/env python3
"""Reproduz o corpus adicional de copia e contexto; nao consulta validacao/teste."""
import hashlib
import json
from pathlib import Path
import random
import struct
def main():
    folder=Path(__file__).resolve().parent/"dados"
    rows=[json.loads(x) for x in (folder/"treino.jsonl").read_text().splitlines()]
    seen={r["prompt"] for r in rows}; rng=random.Random(882716)
    words=["azul","verde","texto","modelo","rede","local","dia","gato","casa","memória","atenção","amanhã","computação","português"]
    forms=["Copie apenas {w}.","A palavra escolhida é {w}. Repita somente essa palavra.","Escreva somente a palavra {w}, sem mais nada.","Retorne somente {w}.","Sua resposta deve ser {w}, sem explicação.","Reproduza literalmente {w}.","Palavra solicitada: {w}. Escreva apenas ela.","Diga {w}, sem comentários."]
    prefixes=["","Tarefa curta. ","Exercício de escrita. ","Siga a instrução final. ","Para esta resposta, siga a solicitação. "]
    for family,count in [("copia_variada",4000),("contexto_variado",2000)]:
        n=0
        while n<count:
            if family=="copia_variada":
                w=rng.choice(words) if rng.random()<.35 else "".join(rng.choices("abcdefghijklmnopqrstuvwxyz",k=rng.randint(2,10)))
                question=rng.choice(prefixes)+rng.choice(forms).format(w=w)
                prompt="Usuário: "+question+"\nAssistente:"; answer=w
            else:
                color=rng.choice(["azul","verde","vermelha","amarela"]); value=rng.randint(10,999)
                first=rng.choice([f"Informação para esta conversa: a caixa {color} contém {value} moedas. Confirme com OK.",f"Anote o dado: a caixa {color} possui {value} moedas. Responda OK.",f"Dado do exercício: há {value} moedas na caixa {color}. Diga OK."])
                last=rng.choice([f"Quantas moedas contém a caixa {color}? Retorne somente o número.",f"Qual quantidade foi indicada para a caixa {color}? Apenas o número.",f"Recupere o número de moedas mencionado para a caixa {color}. Só o número."])
                prompt="Usuário: "+first+"\nAssistente: OK\nUsuário: "+last+"\nAssistente:"; answer=str(value)
                if n%4==0:
                    rows.append(dict(categoria="confirmacao",prompt="Usuário: "+first+"\nAssistente:",resposta="OK"))
            if prompt in seen or len((prompt+answer).encode())+1>384: continue
            seen.add(prompt); rows.append(dict(categoria=family,prompt=prompt,resposta=answer)); n+=1
    rng.shuffle(rows)
    raw=b"GTRDATA1"+struct.pack("<I",len(rows))
    for r in rows:
        p,a=r["prompt"].encode(),r["resposta"].encode(); raw+=struct.pack("<II",len(p),len(a))+p+a
    path=folder/"treino_ampliado.gtd"
    if path.exists() and path.read_bytes()!=raw:
        raise SystemExit("O corpus existente difere da receita. Preservado para revisão.")
    path.write_bytes(raw)
    (folder/"treino_ampliado.jsonl").write_text("".join(json.dumps(r,ensure_ascii=False)+"\n" for r in rows))
    manifest=dict(seed=882716,registros=len(rows),sha256=hashlib.sha256(raw).hexdigest(),base="treino.jsonl",
                  alteracao="6000 tarefas de copia/contexto com novas formulacoes e confirmacoes; validacao e teste permanecem separados")
    (folder/"ampliado_manifesto.json").write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+"\n")
    print(json.dumps(manifest,ensure_ascii=False,indent=2))
if __name__=="__main__": main()
