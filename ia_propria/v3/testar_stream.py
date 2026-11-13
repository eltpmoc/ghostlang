#!/usr/bin/env python3
"""Regressoes de streaming real, ritmo, cobertura e controles do chat."""
import codecs
import json
import os
from pathlib import Path
import select
import subprocess
import time
import tempfile

HERE=Path(__file__).resolve().parent
APP=HERE.parent
BIN=APP/'build/v3/ia'
DEVICE=os.environ.get('GHOST_TEST_DEVICE','cpu')
ENV=dict(os.environ,GHOST_DEVICE=DEVICE,GHOST_STREAM_MS='0',GHOST_TEXT_TEMP='0',
         GHOST_TEXT_NO_REPEAT='0',GHOST_TEXT_REPETITION='1',OMP_NUM_THREADS='2')

def run(text):
    with tempfile.TemporaryDirectory(prefix='ghost-stream-test-') as folder:
        p=subprocess.run([str(BIN),'chat'],input='/internet off\n'+text,text=True,
                         stdout=subprocess.PIPE,stderr=subprocess.STDOUT,cwd=APP,
                         env=dict(ENV,GHOST_KNOWLEDGE_DIR=folder),timeout=40,check=True)
        return p.stdout

def timed_response(mode, question, delay=3):
    p=subprocess.Popen([str(BIN),'chat'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                       cwd=APP,env=ENV)
    p.stdin.write((f'{mode}\n/velocidade {delay}\n{question}\n/sair\n').encode());p.stdin.close()
    raw=b''; first=None; end=None; chunks=0
    marker=b'[Geracao neural] ' if mode=='/neural' else b'[Conversa local] '
    decoder=codecs.getincrementaldecoder('utf-8')('strict')
    deadline=time.monotonic()+30
    try:
        while time.monotonic()<deadline:
            ready,_,_=select.select([p.stdout],[],[],1)
            if not ready:
                if p.poll() is not None: break
                continue
            piece=os.read(p.stdout.fileno(),4096)
            if not piece: break
            decoder.decode(piece)
            raw+=piece
            now=time.monotonic()
            pos=raw.find(marker)
            if pos>=0:
                if first is None:
                    first=now
                    assert p.poll() is None,'Primeiro trecho so apareceu depois do fim'
                chunks+=1
                if b'Voce> ' in raw[pos+len(marker):]:
                    end=now
        decoder.decode(b'',final=True)
        assert p.wait(timeout=3)==0
    finally:
        if p.poll() is None: p.kill();p.wait()
    assert first is not None and end is not None,raw.decode()
    elapsed=end-first
    assert elapsed>0.15, f'Resposta nao foi exibida gradualmente: {elapsed}'
    assert chunks>=3, f'Poucos trechos: {chunks}'
    return elapsed,chunks

def main():
    for country in ['França','Frnaça']:
        out=run(f'/neural\nQual é o presidente da {country}?\n/sair\n')
        assert '[Sem fonte suficiente]' in out,out
        assert '[Geracao neural]' not in out,out
        assert 'embeddings' not in out and 'Prompt contem' not in out,out
    out=run('/neural\n/stream off\nQuem é você?\n/stream on\nQuem é você?\n/sair\n')
    parts=out.split('[Conversa local] ')[1:]
    assert len(parts)==2,out
    assert parts[0].split('Voce> ')[0]==parts[1].split('Voce> ')[0],out
    out=run('/neural\nQUEM É VOCÊ?\n/sair\n')
    assert 'Prompt contem' not in out and '[Conversa local]' in out,out
    for invalid in ['-1','101','nan','1.5','']:
        out=run('/velocidade '+invalid+'\n/sair\n')
        assert 'Use /velocidade N' in out,out
    out=run('/stream off\n/fontes\nDescreva GhostLang e crie um exemplo.\n/sair\n')
    assert '```ghost' in out and 'O dobro de 5 e 10' in out,out
    timings={}
    for mode,question in [('/neural','O que é GhostLang?'),('/fontes','Olá!')]:
        seconds,chunks=timed_response(mode,question,8)
        timings[mode]=dict(segundos=round(seconds,3),trechos_recebidos=chunks)
    report=dict(dispositivo=DEVICE,franca_e_erro_digitacao='sem rede e sem memoria: recusa antes da geracao',
                streaming_e_resposta_completa='texto identico',saudacao_e_identidade='controle local sem inferencia neural',
                utf8='valido',controles='ok',tempos=timings)
    (HERE/f'TESTES_STREAM_{DEVICE}.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(report,ensure_ascii=False,indent=2))
if __name__=='__main__': main()
