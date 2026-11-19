"""Teste do adaptador DAP (ghost debug --dap) com uma sessão completa, como o VS Code faz.

initialize -> launch (tests/ghost/dap_alvo.g) -> setBreakpoints (linha 3, dentro de soma)
-> configurationDone -> stopped (breakpoint) -> stackTrace (soma, linha 3; main, linha 9)
-> scopes/variables (a = 2, b = 5) -> next -> stopped (linha 4) -> evaluate c = 7
-> continue -> output "y = 7" -> exited (0) -> terminated.

Uso: python tests/cli/test_dap.py <ghost.exe> <raiz>
"""
import json
import os
import queue
import subprocess
import sys
import threading

ghost, raiz = sys.argv[1], sys.argv[2]
alvo = os.path.join(raiz, 'tests', 'ghost', 'dap_alvo.g')
proc = subprocess.Popen([ghost, 'debug', '--dap'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, cwd=raiz)
mensagens = queue.Queue()
seq = 0
falhas = 0


def leitor():
    f = proc.stdout
    while True:
        cab = b''
        while not cab.endswith(b'\r\n\r\n'):
            c = f.read(1)
            if not c:
                return
            cab += c
        n = int([l for l in cab.decode().split('\r\n') if l.startswith('Content-Length')][0].split(':')[1])
        mensagens.put(json.loads(f.read(n).decode('utf-8')))


threading.Thread(target=leitor, daemon=True).start()
saida = []


def pedir(comando, **argumentos):
    global seq
    seq += 1
    corpo = json.dumps({'seq': seq, 'type': 'request', 'command': comando, 'arguments': argumentos}).encode()
    proc.stdin.write(b'Content-Length: %d\r\n\r\n' % len(corpo) + corpo)
    proc.stdin.flush()
    return esperar(lambda m: m.get('type') == 'response' and m.get('request_seq') == seq)


def esperar(cond, segundos=120):
    while True:
        try:
            m = mensagens.get(timeout=segundos)
        except queue.Empty:
            raise SystemExit('[FALHA] tempo esgotado esperando resposta do adaptador')
        if m.get('type') == 'event' and m.get('event') == 'output':
            saida.append(m['body']['output'])
        if cond(m):
            return m


def confere(ok, o_que):
    global falhas
    print(('[ok]    ' if ok else '[FALHA] ') + o_que)
    if not ok:
        falhas += 1


evento = lambda nome: (lambda m: m.get('type') == 'event' and m.get('event') == nome)

r = pedir('initialize', adapterID='ghost', linesStartAt1=True)
confere(r['success'] and r['body'].get('supportsConfigurationDoneRequest'), 'initialize')
r = pedir('launch', program=alvo, stopOnEntry=False)
confere(r['success'], 'launch compila o .g com simbolos e inicia o gdb ' + str(r.get('message', '')))
r = pedir('setBreakpoints', source={'path': alvo}, breakpoints=[{'line': 3}])
bps = r['body'].get('breakpoints', [])
confere(r['success'] and bps and bps[0].get('verified') and bps[0].get('line') == 3, 'breakpoint na linha 3 verificado')
pedir('configurationDone')
s = esperar(evento('stopped'))
confere(s['body']['reason'] == 'breakpoint', 'parou no breakpoint')
tid = s['body']['threadId']
r = pedir('stackTrace', threadId=tid)
fr = r['body']['stackFrames']
confere(fr and fr[0]['name'] == 'soma' and fr[0]['line'] == 3 and fr[0]['source']['path'].endswith('dap_alvo.g'),
        'pilha: soma em dap_alvo.g:3')
confere(len(fr) > 1 and fr[1]['name'] == 'main' and fr[1]['line'] == 9, 'pilha: main em dap_alvo.g:9')
r = pedir('scopes', frameId=fr[0]['id'])
ref = r['body']['scopes'][0]['variablesReference']
r = pedir('variables', variablesReference=ref)
vs = {v['name']: v['value'] for v in r['body']['variables']}
confere(vs.get('a') == '2' and vs.get('b') == '5', 'variaveis locais a = 2, b = 5 (%s)' % vs)
pedir('next', threadId=tid)
s = esperar(evento('stopped'))
r = pedir('stackTrace', threadId=tid)
confere(s['body']['reason'] == 'step' and r['body']['stackFrames'][0]['line'] == 4, 'next foi para a linha 4')
r = pedir('evaluate', expression='c', frameId=r['body']['stackFrames'][0]['id'])
confere(r['success'] and r['body']['result'] == '7', 'evaluate c = 7')
pedir('continue', threadId=tid)
e = esperar(evento('exited'))
confere(e['body']['exitCode'] == 0, 'programa terminou com codigo 0')
esperar(evento('terminated'))
confere(any('y = 7' in x for x in saida), 'saida do programa chegou como evento output')
pedir('disconnect')
try:
    proc.wait(timeout=20)
except subprocess.TimeoutExpired:
    proc.kill()
print('DAP: tudo certo' if not falhas else 'DAP: %d falha(s)' % falhas)
sys.exit(1 if falhas else 0)
