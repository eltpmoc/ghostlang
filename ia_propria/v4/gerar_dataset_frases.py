#!/usr/bin/env python3
"""Adiciona um datasheet de frases completas e vocabulário ao treino."""
import hashlib
import json
from pathlib import Path
import random
import struct

def prompt_format(q):
    return "Usuário: " + q + "\nAssistente:"

def main():
    folder = Path(__file__).resolve().parent / "dados"
    # Partir do treino ampliado para ter todas as habilidades base
    base_file = folder / "treino_ampliado.jsonl"
    if not base_file.exists():
        base_file = folder / "treino.jsonl"
        
    rows = [json.loads(x) for x in base_file.read_text(encoding="utf-8").splitlines()]
    seen = {r["prompt"] for r in rows}
    rng = random.Random(554433)
    
    # Novo "datasheet" de palavras e frases completas
    perguntas_respostas = [
        ("Como você está?", "Estou funcionando perfeitamente, obrigado por perguntar!"),
        ("Qual é o seu propósito?", "Meu propósito é ajudar respondendo perguntas e auxiliando em tarefas como uma IA local."),
        ("Conte uma piada.", "Por que o computador foi ao médico? Porque estava com um vírus!"),
        ("O que é o sol?", "O Sol é a estrela central do nosso sistema solar."),
        ("Diga uma frase inspiradora.", "O único lugar onde o sucesso vem antes do trabalho é no dicionário."),
        ("Qual a cor do céu?", "O céu geralmente parece azul durante o dia devido à dispersão da luz solar."),
        ("O que é programação?", "Programação é o processo de escrever instruções para que um computador execute tarefas."),
        ("Você gosta de aprender?", "Como uma IA, sou projetada para processar dados e aprender com os exemplos do meu treinamento."),
        ("Fale sobre o vento.", "O vento é o movimento do ar na atmosfera terrestre, causado por diferenças de pressão e temperatura."),
        ("Como se diz olá em espanhol?", "Em espanhol, olá se diz 'hola'."),
        ("O que é GhostLang?", "GhostLang é a linguagem de programação usada para coordenar e executar meu modelo e ferramentas."),
        ("Você tem sentimentos?", "Não tenho sentimentos ou consciência, sou um modelo matemático que processa texto."),
        ("Diga a palavra abacaxi.", "abacaxi"),
        ("Qual o maior planeta do nosso sistema solar?", "Júpiter é o maior planeta do sistema solar."),
        ("Quantas horas tem um dia?", "Um dia tem 24 horas."),
        ("Qual é a capital do Brasil?", "A capital do Brasil é Brasília."),
        ("O que significa IA?", "IA significa Inteligência Artificial."),
        ("Escreva uma saudação amigável.", "Olá! Que bom falar com você, espero que esteja tendo um ótimo dia."),
        ("Descreva a água.", "A água é um líquido transparente, incolor e inodoro, essencial para a vida na Terra."),
        ("O que é uma montanha?", "Uma montanha é uma elevação natural da superfície terrestre, geralmente com altitude considerável."),
        ("Qual é o contrário de rápido?", "O contrário de rápido é lento."),
        ("Diga a palavra liberdade.", "liberdade")
    ]
    
    # Expandir com variações e combinações
    frases_geradas = []
    
    for _ in range(3000):
        q, a = rng.choice(perguntas_respostas)
        # Adicionar pequenas variações no prompt para o modelo não decorar apenas 1 forma
        prefix = rng.choice(["", "Por favor, ", "Pode me dizer, ", "Responda: "])
        q_var = prefix + q
        if rng.random() < 0.2:
            q_var = q_var.lower()
            
        p = prompt_format(q_var)
        if p not in seen and len((p + a).encode('utf-8')) + 1 <= 384:
            seen.add(p)
            frases_geradas.append(dict(categoria="frases_completas", prompt=p, resposta=a))

    rows.extend(frases_geradas)
    rng.shuffle(rows)
    
    # Converter para formato GTD binário (GhostLang Treino Data)
    raw = b"GTRDATA1" + struct.pack("<I", len(rows))
    for r in rows:
        p = r["prompt"].encode('utf-8')
        a = r["resposta"].encode('utf-8')
        raw += struct.pack("<II", len(p), len(a)) + p + a
        
    path_gtd = folder / "treino_frases.gtd"
    path_gtd.write_bytes(raw)
    
    path_jsonl = folder / "treino_frases.jsonl"
    path_jsonl.write_text("".join(json.dumps(r, ensure_ascii=False) + "\n" for r in rows), encoding="utf-8")
    
    manifest = dict(
        seed=554433,
        registros=len(rows),
        sha256=hashlib.sha256(raw).hexdigest(),
        base="treino_ampliado.jsonl",
        alteracao="Adicionadas frases completas e vocabulário diverso para melhor conversação"
    )
    
    path_manifest = folder / "frases_manifesto.json"
    path_manifest.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("Novo datasheet gerado com sucesso!")
    print(json.dumps(manifest, ensure_ascii=False, indent=2))

if __name__ == "__main__":
    main()
