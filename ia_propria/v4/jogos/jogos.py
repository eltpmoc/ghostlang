#!/usr/bin/env python3
"""Jogos procedurais: professor gera dados; somente o Transformer responde na avaliacao."""
import argparse
from collections import Counter, defaultdict
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import random
import shutil
import string
import struct
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
APP = HERE.parents[1]
BINARY = APP / 'build/v4/ia'
GAMES = ('copia', 'memoria', 'regras', 'caminho', 'ordem', 'contagem')
CONTEXT = 384

def digest(data): return hashlib.sha256(data).hexdigest()
def canonical(value): return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(',', ':'))
def stamp(): return datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
def wrap(text): return 'Usuário: ' + text + '\nAssistente:'
def code(rng, size): return ''.join(rng.choices(string.ascii_uppercase, k=size))

def solve(game, state):
    """Gabarito independente do modelo. Nunca e passado ao subprocesso de inferencia."""
    if game == 'copia': return state['fita']
    if game == 'memoria':
        boxes = dict(state['caixas'])
        boxes.update(state['atualizacoes'])
        return boxes[state['alvo']]
    if game == 'regras':
        result = dict(state['regras'])[state['cor']]
        return dict(state['segunda_regra'])[result] if state['segunda_regra'] else result
    if game == 'caminho':
        position = state['inicio']; edges = dict(state['portas'])
        for _ in range(state['passos']): position = edges[position]
        return position
    if game == 'ordem':
        sequence = state['lista'][::-1] if state['inverter'] else state['lista']
        return sequence[state['posicao'] - 1]
    if game == 'contagem': return str(state['inicio'] + sum(state['mudancas']))
    raise ValueError(game)

def make_case(rng, game, level, form):
    if game == 'copia':
        state = {'fita': code(rng, 2 * level)}
        body = f"Jogo copia. Fita: {state['fita']}. "
        instruction = ['Copie a fita. Apenas o codigo.', 'Repita somente o codigo da fita.',
                       'Escreva a fita sem comentarios.', 'Reproduza exatamente o conteudo da fita, sem outro texto.'][form]
    elif game == 'memoria':
        names = rng.sample(list('ABCDEFGH'), 1 if level == 1 else 3)
        state = {'caixas': [(n, code(rng, 2)) for n in names], 'alvo': rng.choice(names), 'atualizacoes': []}
        if level == 3: state['atualizacoes'] = [(rng.choice(names), code(rng, 2))]
        body = 'Jogo memoria. Caixas: ' + '; '.join(f'{k}={v}' for k, v in state['caixas']) + '.'
        if state['atualizacoes']: body += ' Atualize: ' + '; '.join(f'{k}={v}' for k, v in state['atualizacoes']) + '.'
        body += '\nAssistente: OK\nUsuário: '
        target = state['alvo']
        instruction = [f'Qual o codigo atual da caixa {target}? Apenas o codigo.', f'Leia a caixa {target}. Responda so o codigo atual.',
                       f'Diga somente o ultimo codigo da caixa {target}.', f'Recupere o conteudo mais recente de {target}. Nada alem do codigo.'][form]
    elif game == 'regras':
        colors = rng.sample(['azul', 'verde', 'rosa', 'cinza', 'roxa', 'preta', 'branca', 'amarela', 'laranja', 'marrom', 'vermelha', 'violeta', 'dourada', 'prateada', 'bege', 'ciano'], 1 if level == 1 else 3)
        groups = rng.sample(list(string.ascii_uppercase), len(colors))
        state = {'regras': list(zip(colors, groups)), 'cor': rng.choice(colors), 'segunda_regra': []}
        if level == 3: state['segunda_regra'] = list(zip(groups, rng.sample(list('IJKLMNOP'), len(groups))))
        body = 'Jogo regras. Cor->grupo: ' + '; '.join(f'{k}->{v}' for k, v in state['regras']) + '.'
        if state['segunda_regra']: body += ' Grupo->destino: ' + '; '.join(f'{k}->{v}' for k, v in state['segunda_regra']) + '.'
        body += f" Peca de cor {state['cor']}. "
        instruction = ['Siga as regras. Diga apenas a letra final.', 'Qual a letra ao fim das regras? So a letra.',
                       'Aplique as regras e escreva somente a letra final.', 'Encaminhe a peca ate o destino final. Retorne uma letra.'][form]
    elif game == 'caminho':
        rooms = list('ABCDEF'); destinations = rng.sample(rooms, len(rooms))
        state = {'portas': list(zip(rooms, destinations)), 'inicio': rng.choice(rooms), 'passos': level}
        body = 'Jogo caminho. Portas: ' + '; '.join(f'{k}>{v}' for k, v in state['portas']) + f". Inicio: {state['inicio']}. Passos: {level}. "
        instruction = ['Siga uma porta por passo. Apenas a sala final.', 'Avance o numero de passos. Diga so a sala final.',
                       'Percorra as portas. Responda somente a letra final.', 'Depois desse percurso, em qual sala voce termina? Uma letra.'][form]
    elif game == 'ordem':
        values = rng.sample(list(string.ascii_uppercase), 2 * level + 1)
        state = {'lista': values, 'posicao': 1 if level == 1 else rng.randint(1, len(values)), 'inverter': level == 3}
        body = 'Jogo ordem. Lista: ' + ','.join(values) + ('. Inverta a lista.' if state['inverter'] else '.') + f" Posicao: {state['posicao']} (a primeira e 1). "
        instruction = ['Diga apenas a letra nessa posicao.', 'Qual letra ocupa a posicao pedida? Apenas ela.',
                       'Retorne somente a letra da posicao indicada.', 'Identifique o elemento solicitado e escreva uma unica letra.'][form]
    elif game == 'contagem':
        while True:
            state = {'inicio': rng.randint(0, 49), 'mudancas': [rng.choice([-5,-4,-3,-2,-1,1,2,3,4,5]) for _ in range(level)]}
            total = state['inicio']
            for delta in state['mudancas']:
                total += delta
                if total < 0: break
            else: break
        body = f"Jogo contagem. Estoque inicial: {state['inicio']}. Movimentos: " + ', '.join(f'{n:+d}' for n in state['mudancas']) + '. '
        instruction = ['Qual o estoque final? Apenas o inteiro.', 'Aplique os movimentos e diga somente o saldo.',
                       'Informe so o total apos os movimentos.', 'Quantas unidades restam depois de todas as alteracoes? So o numero.'][form]
    else: raise ValueError(game)
    semantic = {'jogo': game, 'nivel': level, 'estado': state}
    row = dict(semantic, id=digest(canonical(semantic).encode()), formulario=form, prompt=wrap(body + instruction), resposta=solve(game, state))
    assert len((row['prompt'] + row['resposta']).encode()) + 1 <= CONTEXT
    return row

def gtd(rows):
    raw = bytearray(b'GTRDATA1' + struct.pack('<I', len(rows)))
    for row in rows:
        p, a = row['prompt'].encode(), row['resposta'].encode()
        if not p or not a or len(p) + len(a) + 1 > CONTEXT: raise ValueError('Exemplo fora do contexto.')
        raw += struct.pack('<II', len(p), len(a)) + p + a
    return bytes(raw)

def prepare(folder, seed=482917, train_count=120, eval_count=12):
    folder = Path(folder); seen = set(); seen_prompts = set(); splits = {}; manifest = {'versao': 1, 'seed': seed, 'contexto_bytes': CONTEXT, 'particoes': {}}
    for index, split in enumerate(('treino', 'validacao', 'teste', 'teste_formulacao')):
        rng = random.Random(seed + index * 100003); rows = []
        count = train_count if split == 'treino' else eval_count
        for game in GAMES:
            for level in range(1, 4):
                accepted = 0
                for _ in range(count * 5000):
                    row = make_case(rng, game, level, 3 if split == 'teste_formulacao' else rng.randrange(3))
                    if row['id'] in seen or row['prompt'] in seen_prompts: continue
                    seen.add(row['id']); seen_prompts.add(row['prompt']); rows.append(row); accepted += 1
                    if accepted == count: break
                if accepted != count: raise ValueError('Espaco de jogos esgotado; reduza a contagem por grupo.')
        rng.shuffle(rows); splits[split] = rows
    files = {}
    for split, rows in splits.items():
        raw = gtd(rows); files[split+'.gtd'] = raw
        files[split+'.jsonl'] = ''.join(canonical(r)+'\n' for r in rows).encode()
        manifest['particoes'][split] = {'casos': len(rows), 'sha256_gtd': digest(raw), 'sha256_jsonl': digest(files[split+'.jsonl'])}
    for level in range(1,4):
        for split in ('treino', 'validacao'):
            name = f'{split}_nivel{level}.gtd'
            files[name] = gtd([r for r in splits[split] if r['nivel'] <= level])
    manifest['arquivos_sha256'] = {name:digest(raw) for name,raw in files.items()}
    manifest['separacao'] = 'Estados e prompts disjuntos. Formularios 0..2: treino/validacao/teste; formulario 3: teste de nova formulacao. Curriculo cumulativo por nivel.'
    files['manifesto.json'] = (json.dumps(manifest, ensure_ascii=False, indent=2)+'\n').encode()
    folder.mkdir(parents=True, exist_ok=True)
    for name, raw in files.items():
        path = folder/name
        if path.exists() and path.read_bytes() != raw: raise ValueError(f'Arquivo existente preservado: {path}. Use outra pasta.')
    for name, raw in files.items(): (folder/name).write_bytes(raw)
    return manifest

def load(folder, split):
    folder=Path(folder); manifest=json.loads((folder/'manifesto.json').read_text()); path=folder/(split+'.jsonl')
    if digest(path.read_bytes()) != manifest['arquivos_sha256'][path.name]: raise ValueError('Dados alterados desde a preparacao.')
    return [json.loads(line) for line in path.read_text().splitlines()]

def choose(rows, per_group):
    groups=defaultdict(list)
    for row in rows: groups[(row['jogo'],row['nivel'])].append(row)
    return [r for key in sorted(groups) for r in sorted(groups[key],key=lambda x:x['id'])[:per_group]]

def infer(checkpoint, prompt, device, memory):
    # Only prompt crosses the inference boundary: no state, answer or solver.
    env=dict(os.environ,GHOST_DEVICE=device,GHOST_STREAM_MS='0',OMP_NUM_THREADS='2',GHOST_KNOWLEDGE_DIR=str(memory),
             https_proxy='http://127.0.0.1:9',http_proxy='http://127.0.0.1:9')
    try:
        run=subprocess.run([str(BINARY),'gerar',str(checkpoint),prompt,'32'],cwd=APP,env=env,text=True,capture_output=True,timeout=30)
        if run.returncode: return {'resposta':'','erro':run.stderr or run.stdout,'acerto':False}
        return {'resposta':run.stdout.partition('\n')[2].strip(),'avisos':run.stderr}
    except subprocess.TimeoutExpired: return {'resposta':'','erro':'tempo limite','acerto':False}

def evaluate(folder, checkpoint, device='auto', split='teste', per_group=3):
    checkpoint=Path(checkpoint).resolve(); rows=choose(load(folder,split),per_group)
    train=load(folder,'treino'); seen={r['id'] for r in train}; prompts={r['prompt'] for r in train}
    majority=defaultdict(Counter)
    for row in train: majority[(row['jogo'],row['nivel'])][row['resposta']] += 1
    records=[]
    with tempfile.TemporaryDirectory(prefix='ghost-games-') as memory:
        for index,row in enumerate(rows):
            if row['id'] in seen or row['prompt'] in prompts: raise ValueError('Vazamento de treino para avaliacao.')
            result=infer(checkpoint,row['prompt'],device,memory)
            expected=solve(row['jogo'],row['estado'])
            if expected != row['resposta']: raise ValueError('Gabarito inconsistente.')
            frequent=sorted(majority[(row['jogo'],row['nivel'])].items(),key=lambda x:(-x[1],x[0]))[0][0]
            records.append(dict(row,**result,correta=not result.get('erro') and result['resposta']==expected,
                                referencia_frequente=frequent,acerto_frequente=frequent==expected))
            if (index+1)%18==0: print(f'{split}: {index+1}/{len(rows)}',flush=True)
    groups={}
    for game in GAMES:
        for level in range(1,4):
            group=[r for r in records if r['jogo']==game and r['nivel']==level]
            groups[f'{game}/nivel{level}']={'acertos':sum(bool(r['correta']) for r in group),'casos':len(group),
                'falhas_execucao':sum(bool(r.get('erro')) for r in group),'acertos_resposta_frequente':sum(r['acerto_frequente'] for r in group)}
    return {'data_utc':datetime.now(timezone.utc).isoformat(),'checkpoint':str(checkpoint),'sha256':digest(checkpoint.read_bytes()),
            'device':device,'particao':split,'fonte_dados_sha256':digest((Path(folder)/(split+'.jsonl')).read_bytes()),
            'metodo':'Geracao bruta pelos pesos; greedy, 32 bytes; sem chat, filtro, ferramentas ou internet; igualdade exata apos strip.',
            'acertos':sum(bool(r['correta']) for r in records),'casos':len(records),'resumo':groups,'respostas':records}

def train(folder, origin, destination, device, blocks, rate):
    folder=Path(folder).resolve(); origin=Path(origin).resolve(); destination=Path(destination).resolve()
    manifest=json.loads((folder/'manifesto.json').read_text())
    for name, expected in manifest['arquivos_sha256'].items():
        if digest((folder/name).read_bytes())!=expected: raise ValueError(f'Dados alterados: {name}')
    origin_hash=digest(origin.read_bytes()); destination.mkdir(parents=True,exist_ok=False)
    report={'origem':str(origin),'sha256_origem':origin_hash,'dados':str(folder),'device':device,'blocos_por_nivel':blocks,
            'passos_adicionais':3*blocks*100,'taxa':rate,'status':'em_execucao','inicio_utc':datetime.now(timezone.utc).isoformat(),
            'selecao':'Menor entropia cruzada de validacao no ultimo nivel; teste nao usado para selecao. Checkpoint inicial do ultimo nivel tambem concorre.'}
    (destination/'treino.json').write_text(json.dumps(report,indent=2)+'\n')
    current=origin
    try:
        for level in range(1,4):
            target=destination/f'nivel{level}.bin'
            cmd=['bash',str(APP/'scripts/ia_v4.sh'),'--device',device,'continuar',str(folder/f'treino_nivel{level}.gtd'),
                 str(folder/f'validacao_nivel{level}.gtd'),str(blocks),str(rate),str(current),str(target)]
            print(f'Curriculo: nivel {level}, {blocks*100} passos; destino {target}',flush=True)
            with (destination/f'nivel{level}.log').open('w') as log:
                process=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
                for line in process.stdout: log.write(line); log.flush(); print(line,end='',flush=True)
                if process.wait(): raise RuntimeError(f'Falha no treino do nivel {level}; consulte o log.')
            current=Path(str(target)+'.ultimo')
        selected=destination/'selecionado.bin'; shutil.copyfile(destination/'nivel3.bin',selected)
        report.update(status='concluido',selecionado=str(selected),sha256_selecionado=digest(selected.read_bytes()),
                      fim_utc=datetime.now(timezone.utc).isoformat())
    except BaseException:
        report['status']='interrompido_ou_falhou'; raise
    finally:
        report['origem_preservada']=digest(origin.read_bytes())==origin_hash
        (destination/'treino.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    return report

def main():
    parser=argparse.ArgumentParser(description=__doc__); sub=parser.add_subparsers(dest='command',required=True)
    prep=sub.add_parser('preparar'); prep.add_argument('--seed',type=int,default=482917); prep.add_argument('--por-grupo',type=int,default=120); prep.add_argument('--avaliacao-por-grupo',type=int,default=12)
    for name in ('avaliar','jogar','treinar'):
        p=sub.add_parser(name); p.add_argument('--device',choices=('auto','cpu','cuda'),default='auto')
        p.add_argument('--checkpoint',type=Path,default=APP/'checkpoints/v4/transformer_selecionado.bin')
        if name=='avaliar':
            p.add_argument('--particao',choices=('validacao','teste','teste_formulacao'),default='teste');p.add_argument('--por-grupo',type=int,default=3)
            p.add_argument('--relatorio',type=Path,default=None)
        elif name=='jogar':
            p.add_argument('--seed',type=int,default=827493);p.add_argument('--rodadas',type=int,default=6);p.add_argument('--nivel',type=int,choices=(1,2,3),default=1)
        else:
            p.add_argument('--destino',type=Path,required=True);p.add_argument('--blocos-por-nivel',type=int,default=2);p.add_argument('--taxa',type=float,default=.0003)
    for p in sub.choices.values(): p.add_argument('--dados',type=Path,default=HERE/'dados')
    args=parser.parse_args()
    if args.command=='preparar':
        if not 1<=args.por_grupo<=200 or not 1<=args.avaliacao_por_grupo<=30: parser.error('Use 1..200 exemplos de treino/grupo e 1..30 de avaliacao/grupo.')
        print(json.dumps(prepare(args.dados,args.seed,args.por_grupo,args.avaliacao_por_grupo)['particoes'],indent=2))
    elif args.command=='treinar':
        if not 1<=args.blocos_por_nivel<=50 or not .000001<=args.taxa<=.005: parser.error('Blocos: 1..50; taxa: 0.000001..0.005.')
        print(json.dumps(train(args.dados,args.checkpoint,args.destino,args.device,args.blocos_por_nivel,args.taxa),ensure_ascii=False,indent=2))
    elif args.command=='avaliar':
        if not 1<=args.por_grupo<=30: parser.error('Use 1..30 casos por grupo.')
        report=evaluate(args.dados,args.checkpoint,args.device,args.particao,args.por_grupo)
        path=args.relatorio or HERE/'relatorios'/f'{args.particao}_{stamp()}.json';path.parent.mkdir(parents=True,exist_ok=True)
        with path.open('x') as output: json.dump(report,output,ensure_ascii=False,indent=2);output.write('\n')
        print(json.dumps({k:report[k] for k in ('acertos','casos','resumo')},indent=2));print(path)
    else:
        if not 1<=args.rodadas<=100: parser.error('Use 1..100 rodadas.')
        known={r['id'] for split in ('treino','validacao','teste','teste_formulacao') for r in load(args.dados,split)}
        rng=random.Random(args.seed); correct=0; results=[]
        with tempfile.TemporaryDirectory(prefix='ghost-play-') as memory:
            for n in range(args.rodadas):
                for _ in range(10000):
                    row=make_case(rng,GAMES[n%len(GAMES)],args.nivel,rng.randrange(4))
                    if row['id'] not in known: break
                else: raise RuntimeError('Sem desafio novo disponivel.')
                known.add(row['id']);result=infer(args.checkpoint.resolve(),row['prompt'],args.device,memory)
                ok=not result.get('erro') and result['resposta']==row['resposta'];correct+=bool(ok)
                results.append(dict(row,**result,correta=ok))
                print(f"\nRodada {n+1} — {row['jogo']} / nivel {args.nivel}\n{row['prompt']}\nIA (pesos): {result['resposta']!r}\nGabarito do jogo: {row['resposta']!r}\n{'ACERTO' if ok else 'ERRO'}",flush=True)
        path=HERE/'relatorios'/f'partida_{stamp()}.json';path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(json.dumps({'seed':args.seed,'checkpoint':str(args.checkpoint),'sha256':digest(args.checkpoint.read_bytes()),'acertos':correct,'rodadas':results},ensure_ascii=False,indent=2)+'\n')
        print(f'\nPlacar: {correct}/{args.rodadas}. Jogar avalia os pesos; nao atualiza o modelo.\n{path}')

if __name__=='__main__': main()
