#!/usr/bin/env python3
"""Gera um painel HTML local, sem servidor ou dependencias, a partir dos resultados reais."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import jogos

HERE=Path(__file__).resolve().parent

def normalize_rows(rows):
    output=[]; seen=set()
    for row in rows:
        if row['jogo'] not in jogos.GAMES or row['nivel'] not in (1,2,3): raise ValueError('jogo/nivel invalido')
        semantic={key:row[key] for key in ('jogo','nivel','estado')}
        if row['id']!=jogos.digest(jogos.canonical(semantic).encode()): raise ValueError('ID nao corresponde ao estado')
        expected=jogos.solve(row['jogo'],row['estado']); actual=row['resposta']; prompt=row['prompt']
        if not isinstance(actual,str) or not isinstance(prompt,str): raise ValueError('resposta/prompt invalido')
        signature=jogos.digest(jogos.canonical([row['id'],prompt,expected]).encode())
        if signature in seen: raise ValueError('caso duplicado no mesmo relatorio')
        seen.add(signature)
        ok=not bool(row.get('erro')) and actual.strip()==expected
        if row.get('correta') is not ok: raise ValueError('placar inconsistente com a resposta real')
        reference=row.get('referencia_frequente')
        output.append({'id':row['id'],'signature':signature,'game':row['jogo'],'level':row['nivel'],
                       'prompt':prompt,'expected':expected,'actual':actual,'correct':ok,'error':row.get('erro',''),
                       'reference':reference,'reference_correct':reference==expected if reference is not None else None})
    return output

def collect(folder):
    reports=[]; matches={}; skipped=[]; observed=0
    for path in sorted(Path(folder).glob('*.json')):
        try:
            if path.stat().st_size>16*1024*1024: raise ValueError('arquivo maior que 16 MiB')
            raw=json.loads(path.read_text())
            if not isinstance(raw,dict): continue
            if 'respostas' not in raw and 'rodadas' not in raw: continue
            rows=normalize_rows(raw.get('respostas',raw.get('rodadas')))
            if not rows: raise ValueError('relatorio vazio')
            if raw.get('acertos')!=sum(r['correct'] for r in rows): raise ValueError('total de acertos inconsistente')
            checkpoint=raw['sha256']
            if not isinstance(checkpoint,str) or len(checkpoint)!=64 or any(c not in '0123456789abcdef' for c in checkpoint): raise ValueError('hash do modelo invalido')
            entry={'file':path.name,'checkpoint':raw['checkpoint'],'sha':checkpoint,'rows':rows,
                   'date':raw.get('data_utc',''),'device':raw.get('device',raw.get('dispositivo','nao registrado')),
                   'digest':hashlib.sha256(path.read_bytes()).hexdigest()}
            if 'respostas' in raw:
                entry.update(partition=raw['particao'],dataset=raw['fonte_dados_sha256'],method=raw['metodo'])
                if entry['partition'] not in ('teste','teste_formulacao','validacao'): raise ValueError('particao desconhecida')
                reports.append(entry)
            else:
                observed+=1
                key=(checkpoint,tuple(sorted(r['signature'] for r in rows)))
                if key not in matches:
                    matches[key]=dict(entry,repeats=0,seeds=[],files=[],variant=False)
                previous=matches[key]
                old={r['signature']:r['actual'] for r in previous['rows']}
                if old!={r['signature']:r['actual'] for r in rows}: previous['variant']=True
                previous['repeats']+=1;previous['files'].append(path.name)
                if raw.get('seed') not in previous['seeds']: previous['seeds'].append(raw.get('seed'))
        except (ValueError,KeyError,TypeError,OSError) as error:
            skipped.append({'file':path.name,'reason':str(error)})
    reports.sort(key=lambda r:(r['date'],r['file']))
    return {'generated':datetime.now(timezone.utc).isoformat(),'reports':reports,'matches':list(matches.values()),
            'match_files':observed,'skipped':skipped,'games':list(jogos.GAMES)}

def build(folder, destination):
    data=collect(folder)
    if not data['reports']: raise ValueError('Nenhum relatorio de avaliacao valido. Execute jogos.py avaliar primeiro.')
    payload=json.dumps(data,ensure_ascii=False).replace('&','\\u0026').replace('<','\\u003c').replace('>','\\u003e').replace('\u2028','\\u2028').replace('\u2029','\\u2029')
    html=(HERE/'painel.html').read_text().replace('__GHOST_DATA__',payload)
    destination=Path(destination);destination.parent.mkdir(parents=True,exist_ok=True)
    destination.write_text(html)
    return data

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--relatorios',type=Path,default=HERE/'relatorios')
    parser.add_argument('--saida',type=Path,default=HERE/'painel_progresso.html')
    args=parser.parse_args();data=build(args.relatorios,args.saida)
    print(f"Painel: {args.saida.resolve()}\nAvaliacoes: {len(data['reports'])}; partidas: {data['match_files']} arquivos, {len(data['matches'])} conjuntos/modelos distintos.")
    if data['skipped']: print('Relatorios rejeitados: '+json.dumps(data['skipped'],ensure_ascii=False))
    print('Abra o HTML no navegador. Gere novamente depois de novas avaliacoes; abrir o painel nao treina nem mede o modelo.')
if __name__=='__main__': main()
