#!/usr/bin/env python3
"""Compara respostas completas, preservando a avaliacao historica da V3."""
import json
from datetime import datetime,timezone
from avaliar import HERE,run_chat,canonical

def main():
    original=json.loads((HERE/'dados/avaliacao_neural.json').read_text())
    extra=json.loads((HERE/'dados/avaliacao_instrucoes.json').read_text())
    cases=original+extra
    report={'data_utc':datetime.now(timezone.utc).isoformat(),
            'metodo':'geracao livre; CPU; temperatura 0; sem penalidades; limite 320 tokens; correspondencia ignora espacos/maiusculas',
            'limite':'As respostas sao conhecidas do treino. As 89+6 formulacoes nao foram usadas no treino; isto nao testa conhecimento inedito.',
            'modelos':{}}
    for version,checkpoint in [('v3','checkpoints/v3/ghost_g.bin'),('v31','checkpoints/v3/ghost_g_v31.bin')]:
        answers=run_chat([c['pergunta'] for c in cases],checkpoint,True)
        records=[dict(**case,gerada=answer,exata=canonical(answer)==canonical(case['resposta'])) for case,answer in zip(cases,answers)]
        data=dict(checkpoint=checkpoint,exatas_89=sum(c['exata'] for c in records[:len(original)]),
                  exatas_exemplos_6=sum(c['exata'] for c in records[len(original):]),respostas=records)
        data['regressao_usuario']=run_chat(['Descreva o que é GhostLang e crie um exemplo.'],checkpoint,True)[0]
        report['modelos'][version]=data
        print(version, data['exatas_89'],'/89;',data['exatas_exemplos_6'],'/6 exemplos')
        print('Pergunta do usuario (incluida no treino V3.1):',data['regressao_usuario'])
    (HERE/'COMPARACAO_V31.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
if __name__=='__main__': main()
