#!/usr/bin/env python3
"""Contexto, controles e busca publica. Rede real somente com --online."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
APP = HERE.parent
ENV = dict(os.environ, GHOST_DEVICE=os.environ.get('GHOST_TEST_DEVICE', 'cpu'),
           GHOST_STREAM_MS='0', OMP_NUM_THREADS='2')

def run(chat, env=ENV):
    with tempfile.TemporaryDirectory(prefix='ghost-dialogo-test-') as folder:
        p = subprocess.run([str(APP/'build/v3/ia'), 'chat'], cwd=APP,
                           env=dict(env, GHOST_KNOWLEDGE_DIR=folder),
                           input=chat+'\n/sair\n', text=True, capture_output=True, timeout=50, check=True)
        return p.stdout + p.stderr

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--online', action='store_true')
    args = parser.parse_args()
    out = run('/internet off\n/neural\n/stream\nOlá\nComo você não pode errar. Não entendi!\nQuem é você?\nQual é o presidente da França?')
    assert '[Conversa local] Olá!' in out and 'Posso errar, sim.' in out, out
    assert 'coordenada em GhostLang' in out and 'Comando desconhecido' not in out, out
    assert '[Sem fonte suficiente]' in out and '[Geracao neural]' not in out, out
    assert 'Prompt contem' not in out, out
    out = run('O que é GhostLang?\nNão entendi\n/limpar\nNão entendi')
    assert '```ghost' in out and 'Ainda não há um assunto anterior' in out, out
    out = run('/internet off\n/buscar presidente da França')
    assert '[Pesquisa automatica]' not in out and 'Internet desativada' in out, out
    out = run('/buscar email@exemplo.com\n/buscar https://exemplo.com\n/buscar')
    assert out.count('[Memoria] Tema inválido') == 2 and '[Pesquisa automatica]' not in out, out
    # Failed connectivity must not become a fabricated answer or terminate the chat.
    out = run('/buscar computacao quantica\nOlá', dict(ENV, HTTPS_PROXY='http://127.0.0.1:1',
              https_proxy='http://127.0.0.1:1', ALL_PROXY='http://127.0.0.1:1', NO_PROXY='', no_proxy=''))
    assert '[Internet indisponivel]' in out and '[Conversa local] Olá!' in out, out
    results = dict(dispositivo=ENV['GHOST_DEVICE'], dialogo='ok', contexto='ok', controles='ok',
                   falha_de_rede='recusa sem inventar resposta; chat continua', rede_real='nao executada')
    if args.online:
        official = run('/buscar presidente da França\n/buscar presidente da Frnaça')
        assert official.count('[Consulta à fonte oficial]') == 2, official
        assert 'https://www.elysee.fr/' in official and 'Consultado em:' in official, official
        assert 'Interpretando Frnaça como França' in official, official
        wiki = run('/buscar computação quântica')
        assert '[Pesquisa na internet' in wiki and 'https://pt.wikipedia.org/wiki/' in wiki, wiki
        assert 'não responder à pergunta inteira' in wiki, wiki
        results['rede_real'] = 'Presidencia francesa e Wikipedia: OK'
        results['fontes_observadas'] = official + '\n' + wiki
    dest = HERE/f"TESTES_DIALOGO_{ENV['GHOST_DEVICE']}.json"
    dest.write_text(json.dumps(results, ensure_ascii=False, indent=2)+'\n')
    print(json.dumps({k:v for k,v in results.items() if k != 'fontes_observadas'}, ensure_ascii=False, indent=2))

if __name__ == '__main__':
    main()
