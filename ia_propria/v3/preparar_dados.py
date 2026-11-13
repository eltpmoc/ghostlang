#!/usr/bin/env python3
"""Gera corpus, consulta em GhostLang e manifesto de fontes locais."""
import hashlib
import json
import re
import unicodedata
from gerar_consulta import generate
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
STOP = set('o a os as um uma uns umas de do da dos das e em no na nos nas que qual quais como por para com se eu voce pode explique me sobre é'.split())
STOP.update('descreva detalhe detalhes crie criar mostre mostrar apresente exemplo exemplos codigo simples pratico completo favor mim meu depois agora mais resuma resumo breve curto dois duas'.split())
def normalize(text):
    text = ''.join(c for c in unicodedata.normalize('NFD', text.lower()) if unicodedata.category(c) != 'Mn')
    return ' '.join(dict.fromkeys(w for w in re.findall('[a-z0-9]+', text) if w not in STOP))
def literal(text): return json.dumps(text, ensure_ascii=False)

def main():
    cards = json.loads((HERE/'dados/conhecimento.json').read_text())
    general = json.loads((HERE.parent/'v2/dados/conversas.json').read_text())
    # Duas formulacoes para treino; a terceira para validacao neural.
    # A base documental, explicitamente separada, inclui as tres formulacoes.
    for i, case in enumerate(general):
        if i in (4, 5, 8): continue  # A identidade da V3 tem sua ficha propria.
        cards.append(dict(id=f'conversa_{i}', **case,
                          arquivo='ia_propria/v2/dados/conversas.json', ancora=case['perguntas'][0]))
    train, val, manifest = [], [], []
    for card in cards:
        source = ROOT/card['arquivo']
        contents = source.read_text()
        if card['ancora'] not in contents:
            raise ValueError(f"Ancora nao encontrada: {source}: {card['ancora']}")
        line = contents[:contents.index(card['ancora'])].count('\n')+1
        card['fonte'] = f"{card['arquivo']}:{line}"
        manifest.append(dict(id=card['id'], fonte=card['fonte'], sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
        for extra in card.get('fontes_adicionais',[])+([card['exemplo']] if card.get('exemplo') else []):
            path=ROOT/extra
            if not path.is_file(): raise ValueError('Fonte ausente: '+extra)
            manifest.append(dict(id=card['id'], fonte=extra+':1', sha256=hashlib.sha256(path.read_bytes()).hexdigest()))

        for index, question in enumerate(card['perguntas']):
            document = f"Usuário: {question}\nAssistente: {card['resposta']}"
            (train if index < 2 else val).append(document)
    if set(train)&set(val): raise ValueError('Sobreposicao treino-validacao')
    for filename, docs in [('treino.txt',train), ('validacao.txt',val)]:
        (HERE/'dados'/filename).write_text('\n\n'.join(docs)+'\n')
    (HERE/'dados/fontes.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    (HERE/'dados/avaliacao_neural.json').write_text(json.dumps([
        dict(id=c['id'], pergunta=c['perguntas'][2], resposta=c['resposta']) for c in cards
    ], ensure_ascii=False,indent=2)+'\n')
    (HERE/'src').mkdir(exist_ok=True)
    (HERE/'src/conhecimento.g').write_text(generate(cards, ROOT, STOP, normalize))
    print(f'{len(cards)} fichas; {len(train)} conversas de treino; {len(val)} de validação.')
    print('Validação neural compartilha respostas com treino: mede novas formulações, não novos fatos.')

if __name__=='__main__': main()
