#!/usr/bin/env python3
"""Avalia separadamente consulta documental e geracao neural livre."""
import argparse
import json
import os
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parent
def canonical(text):
    # Ignora espacos e diferencas de espacamento da renderizacao de pontuacao.
    return re.sub(r'\s+', '', text).casefold()
def run_chat(questions, checkpoint, neural=False):
    env = dict(os.environ, GHOST_DEVICE='cpu', OMP_NUM_THREADS='1', GHOST_TEXT_TEMP='0', GHOST_TEXT_NO_REPEAT='0', GHOST_TEXT_REPETITION='1')
    if neural:
        # Avalia os pesos sem o filtro do chat; recusas de cobertura nao sao aprendizagem.
        answers=[]
        for question in questions:
            result=subprocess.run([str(PROJECT/'build/v3/ia'),'gerar',checkpoint,
                                   'Usuário: '+question+'\nAssistente:','320'],cwd=PROJECT,
                                  env=env,text=True,capture_output=True,check=True,timeout=60)
            answers.append(result.stdout.split('\n',1)[1].strip())
        return answers
    data = ('/neural\n' if neural else '')+'\n'.join(questions)+'\n/sair\n'
    result = subprocess.run([str(PROJECT/'build/v3/ia'), 'chat', checkpoint], input=data, text=True,
                            capture_output=True, env=env, cwd=PROJECT, timeout=300, check=True)
    parts = result.stdout.split('Voce> ')[1:]
    if neural: parts = parts[1:]
    answers = [p.strip().removeprefix('[Geracao neural] ').strip() for p in parts[:len(questions)]]
    if len(answers)!=len(questions): raise RuntimeError('Formato inesperado do chat: '+result.stdout)
    return answers

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--checkpoint', default='checkpoints/v3/ghost_g_v31.bin')
    parser.add_argument('--output',default='AVALIACAO_ATUAL.json')
    args=parser.parse_args()
    cases=json.loads((HERE/'dados/avaliacao_neural.json').read_text())
    neural=run_chat([c['pergunta'] for c in cases],args.checkpoint,True)
    records=[]
    for case, answer in zip(cases,neural):
        records.append(dict(**case, gerada=answer, exata=canonical(answer)==canonical(case['resposta'])))
    # Consultas independentes nao herdam o assunto de uma pergunta anterior.
    def documentary_answer(question):
        result=subprocess.run([str(PROJECT/'build/v3/ia'),'consultar',question],cwd=PROJECT,
                              env=dict(os.environ,GHOST_DEVICE='cpu'),text=True,capture_output=True,check=True,timeout=30)
        return result.stdout.split('\n',1)[1].strip()
    documentary=[documentary_answer(c['pergunta']) for c in cases]
    doc_correct=sum(canonical(answer.split('\nFonte:')[0].removeprefix('[Consulta documental] ')).startswith(canonical(case['resposta']))
                    for case,answer in zip(cases,documentary))
    # Perguntas adicionais nao usadas como aliases nem textos de treinamento.
    extra=[
        ('Explique GhostLang para mim.', 'ghostlang'),
        ('Como somar dois Números G?', 'soma_g'),
        ('Como multiplicar dois Números G?', 'produto_g'),
        ('Me explique a norma quadrada de um Número G.', 'norma'),
        ('Como selecionar automaticamente GPU ou CPU?', 'auto'),
        ('Como salvar e depois carregar meu modelo?', 'checkpoint'),
        ('Qual é a arquitetura da IA V3?', 'arquitetura'),
        ('A multiplicação G permite mudar os parênteses?', 'associatividade'),
        ('Me explique o que acontece com a divisão por zero em G.', 'divisao_zero'),
        ('A V3 treina com pesos que são Números G?', 'tipo_pesos'),
        ('Qual é a capital da Noruega?', None),
        ('Resolva a integral de seno ao cubo.', None),
        ('GhostLang tem coleta de lixo geracional?', None),
        ('Qual é a data de lançamento do GhostLang 9?', None),
        ('O que a teoria G prova sobre buracos negros?', None),
        ('CUDA garante que toda resposta será verdadeira?', None),
        ('Como curar uma doença usando números fantasmas?', None),
        ('Qual senha está configurada no computador?', None),
        ('Pode provar que P é igual a NP?', None),
        ('GhostLang possui suporte nativo para Vulkan?', None),
    ]
    by_id={c['id']:c for c in cases}
    extra_answers=[documentary_answer(q) for q,_ in extra]
    doc_records=[]
    for (question,id),answer in zip(extra,extra_answers):
        correct=answer.startswith('[Sem fonte suficiente]') if id is None else canonical(answer.split('\nFonte:')[0].removeprefix('[Consulta documental] ')).startswith(canonical(by_id[id]['resposta']))
        doc_records.append(dict(pergunta=question, id_esperado=id, resposta=answer, correta=correct))
    report=dict(data_utc=datetime.now(timezone.utc).isoformat(), checkpoint=args.checkpoint,
                configuracao='CPU; temperatura=0; sem penalidades; limite=320 tokens; consulta acerta quando preserva resumo esperado (pode acrescentar detalhes)',
                aviso='Validacao neural usa perguntas novas com respostas conhecidas do treino; nao mede fatos ineditos.',
                neural_exatas=sum(c['exata'] for c in records), neural_total=len(records),
                documental_aliases_corretos=doc_correct, documental_aliases_total=len(cases),
                documental_extra_corretos=sum(c['correta'] for c in doc_records), documental_extra_total=len(extra),
                neural=records, documental_extra=doc_records)
    (HERE/args.output).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ['neural','documental_extra']},ensure_ascii=False,indent=2))

if __name__=='__main__': main()
