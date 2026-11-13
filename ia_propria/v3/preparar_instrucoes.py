#!/usr/bin/env python3
"""Amplia formulacoes de treino; mantem as 89 perguntas originais fora do treino."""
import hashlib
import json
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parent.parent

def main():
    cards=json.loads((HERE/'dados/conhecimento.json').read_text())
    by_id={c['id']:c for c in cards}
    general=json.loads((HERE.parent/'v2/dados/conversas.json').read_text())
    cases=cards+[dict(c,id=f'conversa_{i}') for i,c in enumerate(general) if i not in (4,5,8)]
    validation=[dict(id=c['id'],pergunta=c['perguntas'][2],resposta=c['resposta']) for c in cases]
    extras={
        'ghostlang': ['Descreva o que é GhostLang e crie um exemplo.', 'Explique GhostLang com um exemplo.', 'Mostre um programa simples em GhostLang.', 'Crie um exemplo de GhostLang e explique o código.', 'Quero aprender GhostLang com código.'],
        'teoria': ['Explique Números G com um exemplo.', 'Mostre um exemplo de Números Fantasmas G.', 'Crie um programa usando os Números G.', 'Descreva a teoria G e demonstre o cálculo.', 'Quero entender os fantasmas com código.'],
        'gate': ['Explique matrizes em GhostLang com um exemplo.', 'Crie um programa de multiplicação matricial.', 'Mostre uma matriz multiplicando um vetor.', 'Demonstre state e gate com código.', 'Quero um exemplo de cálculo matricial em GhostLang.'],
        'nilpotente': ['Mostre um exemplo de nilpotência.', 'Demonstre o quadrado do fantasma puro.', 'Crie um programa que calcula theta vezes theta.', 'Explique theta nilpotente com código.', 'Quero verificar o quadrado de theta.'],
        'textmodel': ['Mostre um exemplo de textmodel.', 'Crie um programa de treinamento de texto.', 'Demonstre o treinamento nativo em GhostLang.', 'Explique textmodel com código.', 'Quero um exemplo de rede treinável.'],
        'auto': ['Explique a escolha de CPU e GPU com um exemplo.', 'Mostre código para seleção automática de hardware.', 'Crie um programa que usa pragma auto.', 'Demonstre o autoteste do modelo.', 'Quero um exemplo de seleção CUDA.'],
    }
    heldout={
        'ghostlang':'Apresente a linguagem GhostLang e um pequeno programa comentado.',
        'teoria':'Como representar e operar Números G num programa completo?',
        'gate':'Pode mostrar uma matriz 2x2 aplicada a um vetor?',
        'nilpotente':'Escreva um exemplo verificando que theta ao quadrado resulta em zero.',
        'textmodel':'Como fica um arquivo .g que treina um modelo de texto?',
        'auto':'Preciso de um programa que identifique o hardware e teste CUDA.',
    }
    training=[]
    for c in cases:
        for question in c['perguntas'][:2]:
            # Transformacoes superficiais: nao representam nova informacao.
            for variant in [question,'Por favor, '+question,'Tenho uma dúvida: '+question,question+' Pode explicar?']:
                training.append(dict(pergunta=variant,resposta=c['resposta']))
    extra_eval=[]
    for id,questions in extras.items():
        c=by_id[id]
        answer=c['resposta']+'\nExemplo:\n'+(ROOT/c['exemplo']).read_text().strip()+'\n'+c['explicacao_exemplo']
        extra_eval.append(dict(id=id+'_exemplo',pergunta=heldout[id],resposta=answer))
        for q in questions:
            for variant in [q,'Por favor, '+q,q+' Explique cada parte.']:
                training.append(dict(pergunta=variant,resposta=answer))
    reserved={c['pergunta'].casefold() for c in validation+extra_eval}
    training=[c for c in training if c['pergunta'].casefold() not in reserved]
    def doc(c):
        answer=c['resposta'].replace('\n\n','\n')
        return f"Usuário: {c['pergunta']}\nAssistente: {answer}"
    train=list(dict.fromkeys(doc(c) for c in training));val=[doc(c) for c in validation+extra_eval]
    assert not set(train)&set(val)
    for name,docs in [('instrucoes_treino.txt',train),('instrucoes_validacao.txt',val)]:
        (HERE/'dados'/name).write_text('\n\n'.join(docs)+'\n')
    (HERE/'dados/avaliacao_instrucoes.json').write_text(json.dumps(extra_eval,ensure_ascii=False,indent=2)+'\n')
    report={'treino':len(train),'validacao':len(val),'metodo':'parafrases autorais e variacoes superficiais; respostas conhecidas compartilhadas com validacao',
            'pergunta_usuario_incluida_no_treino':extras['ghostlang'][0],
            'sha256':{name:hashlib.sha256((HERE/'dados'/name).read_bytes()).hexdigest() for name in ['instrucoes_treino.txt','instrucoes_validacao.txt']}}
    (HERE/'dados/instrucoes_manifesto.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(report,ensure_ascii=False,indent=2))
if __name__=='__main__': main()
