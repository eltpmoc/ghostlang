#!/usr/bin/env python3
"""Corpus proprio; templates e enunciados separados entre treino, validacao e teste."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import random
import struct
HERE=Path(__file__).resolve().parent
APP=HERE.parent
NAMES=["Ana","Bruno","Carla","Davi","Eva","Fabio","Gabi","Hugo","Iara","Joao","Lia","Mario","Nara","Olga","Paulo","Rita"]
WORDS=["azul","verde","vermelho","casa","texto","modelo","rede","local","dia","noite","livro","janela","porta","estrela","lua","sol","gato","peixe","agua","cafe","ação","coração"]
def prompt(q): return "Usuário: "+q+"\nAssistente:"
def example(rng,family,split):
    n,a,b=rng.randint(1,99),rng.randint(1,29),rng.randint(1,9)
    color=rng.choice(["azul","verde","amarela","vermelha"])
    box=rng.choice(list("ABCDEFGH")); name=rng.choice(NAMES)
    form=rng.randrange(4) if split=="treino" else (4 if split=="validacao" else 5)
    if family=="instrucoes":
        word=rng.choice(WORDS) if rng.random()<.6 else "".join(rng.choices("abcdefghijklmnopqrstuvwxyz",k=rng.randint(3,8)))
        forms=[f"Diga somente a palavra {word}.",f"Escreva {word}. A resposta deve conter apenas essa palavra.",
               f"Retorne a palavra {word}, sem comentários.",f"A saída solicitada é {word}. Não acrescente outra palavra.",
               f"Produza exclusivamente a palavra {word}.",f"Responda com {word} e nada além disso."]
        return prompt(forms[form]),word
    if family=="contexto":
        value=rng.randint(1,999)
        statement=f"Guarde a informação deste exercício: a caixa {color} tem {value} moedas."
        forms=[f"Qual o número de moedas da caixa {color}? Diga apenas o número.",
               f"Quantas moedas foram informadas para a caixa {color}? Apenas o número.",
               f"Recupere a quantidade que eu informei sobre a caixa {color}. Resposta numérica.",
               "Escreva somente a quantidade de moedas mencionada antes.",
               f"Segundo a mensagem anterior, quantas moedas há na caixa {color}? Apenas o número.",
               f"Retome o dado sobre a caixa {color}: qual era a quantidade? Só o número."]
        return prompt(statement)+" OK.\nUsuário: "+forms[form]+"\nAssistente:",str(value)
    if family=="deducao":
        forms=[f"Regra: itens de cor {color} pertencem ao grupo {box}. O item de {name} tem cor {color}. Diga apenas a letra do grupo desse item.",
               f"Todo objeto de cor {color} vai para {box}. Um objeto de {name} tem essa cor. Indique somente a letra do destino.",
               f"Se uma peça tem cor {color}, sua caixa é {box}. A peça de {name} tem cor {color}. Responda apenas a letra da caixa.",
               f"Cada item de cor {color} está no grupo {box}. O item de {name} é dessa cor. Qual grupo? Apenas a letra.",
               f"Objetos de cor {color} devem ficar em {box}. O objeto de {name} tem cor {color}. Qual o destino? Só a letra.",
               f"Considere a regra: tudo de cor {color} fica em {box}. Uma peça de {name} é dessa cor. Diga a letra correta."]
        return prompt(forms[form]),box
    if family=="leitura":
        names=rng.sample(NAMES,3); values=rng.sample(range(1,100),3)
        table="; ".join(f"{name}={value}" for name,value in zip(names,values))
        forms=[f"Pontuação: {table}. Quem tem o maior valor? Responda apenas o nome.",
               f"Dados: {table}. Escolha o nome associado ao maior número, sem explicar.",
               f"Compare os pontos de {table}. Diga somente o nome de quem lidera.",
               f"Placar: {table}. Qual pessoa tem mais pontos? Só o nome.",
               f"Observe {table}. Identifique apenas o nome com a pontuação mais alta.",
               f"Use esta tabela: {table}. Escreva o nome do maior valor."]
        return prompt(forms[form]),names[values.index(max(values))]
    if family=="calculo":
        forms=[f"O estoque começou em {n}. Entraram {a} unidades e saíram {b}. Qual o saldo? Responda apenas o inteiro.",
               f"Calcule {n}+{a}-{b}. Escreva somente o resultado.",
               f"Há {n} livros. Chegam mais {a} e são retirados {b}. Quantos restam? Apenas o número.",
               f"Saldo inicial {n}, acréscimo {a}, retirada {b}. Informe somente o saldo final.",
               f"Some {n} a {a} e subtraia {b}. Diga somente o resultado.",
               f"Um depósito tinha {n} itens. Ganhou {a}, perdeu {b}. Escreva apenas a quantidade final."]
        return prompt(forms[form]),str(n+a-b)
    x,y,p,u,v,q=[rng.randint(-3,6) for _ in range(6)]
    prefix="No produto G, (x,y,p)*(u,v,q)=(x*u-y*v,x*v+y*u,x*q+u*p). "
    forms=[f"Multiplique ({x},{y},{p}) por ({u},{v},{q}).",
           f"Use A=({x},{y},{p}) e B=({u},{v},{q}). Calcule A*B.",
           f"Determine o produto dos vetores G ({x},{y},{p}) e ({u},{v},{q}).",
           f"Com as entradas ({x},{y},{p}) e ({u},{v},{q}), faça o produto G.",
           f"Quais coordenadas resultam de ({x},{y},{p})*({u},{v},{q})?",
           f"Obtenha as três coordenadas ao multiplicar ({x},{y},{p}) e ({u},{v},{q})."]
    return prompt(prefix+forms[form]+" Responda só os três inteiros separados por vírgulas."),f"{x*u-y*v},{x*v+y*u},{x*q+u*p}"
def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--context",type=int,default=384); args=parser.parse_args()
    if not 320 <= args.context <= 512: parser.error("O gerador de tarefas aceita contexto 320..512.")
    folder=HERE/"dados"; folder.mkdir(parents=True,exist_ok=True)
    seen=set(); manifest={"contexto_bytes":args.context,"fontes":["tarefas sinteticas proprias","v3/dados/conhecimento.json"],
        "criterio":"Templates 0..3 treino, 4 validacao, 5 teste; prompts disjuntos. Controles documentais conhecidos ficam fora do teste novo.","particoes":{}}
    for split,seed,count in [("treino",64192,1400),("validacao",91127,100),("teste",73193,100)]:
        rng=random.Random(seed); records=[]
        for family in ["instrucoes","contexto","deducao","leitura","calculo","numeros_g"]:
            n=0
            while n<count:
                text,answer=example(rng,family,split)
                if text in seen or len((text+answer).encode())+1>args.context: continue
                seen.add(text); records.append(dict(categoria=family,prompt=text,resposta=answer)); n+=1
        if split=="treino":
            for card in json.loads((APP/"v3/dados/conhecimento.json").read_text()):
                for q in card["perguntas"]:
                    text,answer=prompt(q),card["resposta"]
                    if len((text+answer).encode())+1<=args.context: records.append(dict(categoria="conhecimento_local",prompt=text,resposta=answer))
            for _ in range(300):
                value=rng.randint(1,999); color=rng.choice(["azul","verde","amarela","vermelha"])
                records.append(dict(categoria="dialogo",prompt=prompt(f"Guarde a informação deste exercício: a caixa {color} tem {value} moedas."),resposta="OK"))
            for q,a in [("Olá!","Olá! Como posso ajudar?"),("Bom dia!","Bom dia! Em que posso ajudar?"),("Quem é você?","Sou a IA experimental própria deste projeto, executada em GhostLang."),("Obrigado!","De nada!")]:
                records.append(dict(categoria="dialogo",prompt=prompt(q),resposta=a))
        rng.shuffle(records); raw=b"GTRDATA1"+struct.pack("<I",len(records))
        for record in records:
            q,a=record["prompt"].encode(),record["resposta"].encode(); raw+=struct.pack("<II",len(q),len(a))+q+a
        (folder/(split+".gtd")).write_bytes(raw)
        (folder/(split+".jsonl")).write_text("".join(json.dumps(r,ensure_ascii=False)+"\n" for r in records))
        manifest["particoes"][split]={"seed":seed,"registros":len(records),"categorias":dict(Counter(r["categoria"] for r in records)),"sha256":hashlib.sha256(raw).hexdigest()}
    (folder/"manifesto.json").write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+"\n")
    print(json.dumps(manifest["particoes"],ensure_ascii=False,indent=2))
if __name__=="__main__": main()
