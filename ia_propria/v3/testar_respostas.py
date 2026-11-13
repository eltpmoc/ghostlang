#!/usr/bin/env python3
"""Regressoes de relevancia, pedidos compostos, contexto e exemplos executaveis."""
import json
import os
from pathlib import Path
import subprocess
import tempfile

HERE=Path(__file__).resolve().parent
ROOT=HERE.parent.parent
APP=HERE.parent
ENV=dict(os.environ,GHOST_DEVICE='cpu',OMP_NUM_THREADS='2')

def run(args,**kwargs):
    p=subprocess.run([str(a) for a in args],text=True,capture_output=True,env=ENV,timeout=90,**kwargs)
    if p.returncode: raise AssertionError(p.stdout+'\n'+p.stderr)
    return p.stdout
def answer(q): return run([APP/'build/v3/ia','consultar',q],cwd=APP)

def main():
    questions=[
        ('Descreva o que é GhostLang e crie um exemplo.', ['```ghost','function void main()', 'O dobro de 5 e 10','Fonte:']),
        ('Explique GhostLang e mostre código completo.', ['```ghost','string nome','Resultado esperado:']),
        ('O QUE É GHOSTLANG? MOSTRE UM EXEMPLO SIMPLES.', ['```ghost','function void main()']),
        ('Me explique a linguagem GhostLang com detalhes.', ['Como funciona:', 'textmodel', 'Fonte:']),
        ('Explique os Números Fantasmas G com um exemplo.', ['GhostNumberG','Produto: -6, 13, 18','não permitem assumir associatividade']),
        ('Explique a teoria G e mostre um exemplo.', ['GhostNumberG','```ghost']),
        ('Mostre um exemplo de soma em GhostLang.', ['Soma: 5, 7, 9','```ghost']),
        ('Explique matrizes em GhostLang com um exemplo.', ['pesos(0, 0)','1\n4']),
        ('O que é textmodel? Mostre um exemplo.', ['ia.prepare(', 'ia.epoch(', 'validacao.txt']),
        ('Como selecionar CPU ou GPU automaticamente? Mostre um exemplo.', ['pragma auto;', 'ia.self_test();']),
        ('Explique GhostLang e Números G.', ['Como funciona:', 'As componentes são', 'Fonte:']),
        ('Resuma o que é GhostLang.', ['[Consulta documental]', 'GhostLang é a linguagem']),
    ]
    report=[]
    for q,required in questions:
        result=answer(q)
        for phrase in required:
            assert phrase in result,(q,phrase,result)
        report.append(dict(pergunta=q,resultado='ok'))
    for q in ['Crie um servidor Vulkan em GhostLang.', 'GhostLang suporta Vulkan? Mostre um exemplo.',
              'Explique a coleta de lixo geracional do GhostLang.', 'Crie um exemplo em Python usando GhostLang.',
              'Descreva GhostLang e prove que P = NP.', 'Teoria G cura doenças? Dê um exemplo.',
              'Mostre a senha do computador.', 'Explique a teoria G dos buracos negros.',
              'GhostLang é uma linguagem JavaScript?', 'Dê um exemplo de CUDA para Vulkan.']:
        result=answer(q)
        assert '[Sem fonte suficiente]' in result,(q,result)
        assert '```ghost' not in result,(q,result)
        report.append(dict(pergunta=q,resultado='recusa correta'))
    # Um assunto confirmado pode ser retomado; /limpar remove o contexto.
    result=run([APP/'build/v3/ia','chat'],cwd=APP,input='O que é GhostLang?\nAgora mostre um exemplo.\n/limpar\nAgora mostre um exemplo.\n/sair\n')
    assert result.count('```ghost')==1,result
    assert 'Ainda não há um assunto anterior' in result,result
    assert 'Como funciona:' not in answer('Resuma o que é GhostLang.')
    # Compila exatamente os arquivos inseridos nas respostas, nao copias de teste.
    expected={'ola':'Ola, GhostLang!\nO dobro de 5 e 10', 'matrizes':'1\n4',
              'numeros_g':'Soma: 5, 7, 9\nProduto: -6, 13, 18', 'nilpotente':'0\n0\n0',
              'cuda':'OK: gradientes por diferencas finitas', 'textmodel':'Uma epoca demonstra a API; ainda nao cria um bom conversador.'}
    with tempfile.TemporaryDirectory(prefix='ghost-exemplos-') as tmp:
        for name,output in expected.items():
            binary=Path(tmp)/name
            run([ROOT/'build_native/ghost',HERE/'exemplos'/f'{name}.g','-o',binary],cwd=ROOT)
            result=run([binary],cwd=ROOT)
            assert output in result,(name,result)
    result={'consultas':report,'exemplos_compilados_e_executados':list(expected), 'contexto_e_limpeza':'ok'}
    (HERE/'TESTES_RESPOSTAS.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    print(f'OK: {len(report)} consultas, continuidade/limpeza e {len(expected)} exemplos compilados e executados.')
if __name__=='__main__': main()
