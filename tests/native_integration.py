#!/usr/bin/env python3
"""Integra compilador, selecao de hardware e compatibilidade de checkpoints."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parent.parent
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--build',default='build_native')
    parser.add_argument('--cuda',action='store_true')
    args=parser.parse_args()
    compiler=(ROOT/args.build/'ghost').resolve()
    def run(command,*,env=None,success=True,cwd=ROOT,input=None):
        result=subprocess.run(command,cwd=cwd,env=dict(os.environ,OMP_NUM_THREADS='4',**(env or {})),input=input,text=True,capture_output=True,timeout=120)
        if (result.returncode==0)!=success:
            raise AssertionError(f'{command}: {result.returncode}\n{result.stdout}\n{result.stderr}')
        return result.stdout+result.stderr
    with tempfile.TemporaryDirectory(prefix="ghost test '") as directory:
        folder=Path(directory)
        binary=folder/'program with spaces'
        run([str(compiler),str(ROOT/'tests/ghost/native_textmodel.g'),'-o',str(binary)],cwd=folder)
        output=run([str(binary),'ok'],env={'GHOST_DEVICE':'cpu'})
        assert 'OK: textmodel' in output and 'CPU;' in output
        hidden=run([str(binary),'ok'],env={'GHOST_DEVICE':'auto','CUDA_VISIBLE_DEVICES':''})
        assert 'CPU;' in hidden
        run([str(binary),'ok'],env={'GHOST_DEVICE':'cuda','CUDA_VISIBLE_DEVICES':''},success=False)
        run([str(binary),'ok'],env={'GHOST_DEVICE':'invalid'},success=False)
        if args.cuda:
            assert 'CUDA:' in run([str(binary),'ok'],env={'GHOST_DEVICE':'cuda'})
        source=folder/'bad.g'
        source.write_text('function void main() { indefinido(); }\n')
        run([str(compiler),str(source),'-o',str(folder/'bad')],success=False)
        source.write_text('pragma cpu;\nprint(device_info());\n')
        run([str(compiler),str(source),'-o',str(binary)])
        assert 'CPU;' in run([str(binary)],env={'GHOST_DEVICE':'cuda'})
        source.write_text('pragma gpu;\nprint(device_info());\n')
        run([str(compiler),str(source),'-o',str(binary)])
        run([str(binary)],env={'CUDA_VISIBLE_DEVICES':''},success=False)
        source.write_text('pragma auto;\nprint(device_info());\n')
        run([str(compiler),str(source),'-o',str(binary)])
        assert 'CPU;' in run([str(binary)],env={'CUDA_VISIBLE_DEVICES':''})
        checkpoint=ROOT/'ia_propria/checkpoints/v2/dialogo.bin'
        if checkpoint.exists():
            source.write_text('function void main(string args[]) { textmodel ia; ia.load(args[0]); ia.info(); print(ia.generate("Usuário: Olá!\\nAssistente:", 40)); }\n')
            run([str(compiler),str(source),'-o',str(binary)])
            assert '677' in run([str(binary),str(checkpoint)],env={'GHOST_DEVICE':'cpu'})
        for legacy in ['syntax.g','test_algebra.g','test_4d.g']:
            run([str(compiler),str(ROOT/'tests/ghost'/legacy),'-o',str(binary)])
            run([str(binary)],env={'GHOST_DEVICE':'cpu'})
    print('OK: compilacao fora da raiz/caminhos especiais, erros, pragmas, fallback, checkpoint V2 e exemplos existentes.')
if __name__=='__main__': main()
