#!/usr/bin/env python3
import json
from pathlib import Path
import random
import struct
import tempfile
import unittest
from unittest.mock import patch
import jogos

class JogosTest(unittest.TestCase):
    def test_gabaritos_independentes(self):
        self.assertEqual(jogos.solve('copia', {'fita':'ZX'}), 'ZX')
        self.assertEqual(jogos.solve('memoria', {'caixas':[['A','ZX'],['B','QQ']], 'atualizacoes':[['A','RT'],['B','TT']], 'alvo':'A'}), 'RT')
        self.assertEqual(jogos.solve('regras', {'regras':[['azul','B']], 'segunda_regra':[['B','Z']], 'cor':'azul'}), 'Z')
        self.assertEqual(jogos.solve('caminho', {'portas':[['A','B'],['B','C'],['C','A']], 'inicio':'A', 'passos':2}), 'C')
        self.assertEqual(jogos.solve('ordem', {'lista':['A','D','C','Z'], 'inverter':True,'posicao':2}), 'C')
        self.assertEqual(jogos.solve('contagem', {'inicio':10,'mudancas':[5,-2,-4]}), '9')

    def test_geracao_limites_e_consistencia(self):
        rng=random.Random(671)
        for game in jogos.GAMES:
            for level in (1,2,3):
                for form in range(4):
                    for _ in range(10):
                        row=jogos.make_case(rng,game,level,form)
                        self.assertLessEqual(len((row['prompt']+row['resposta']).encode())+1,384)
                        self.assertEqual(row['resposta'],jogos.solve(game,row['estado']))
                        if game=='contagem': self.assertGreaterEqual(int(row['resposta']),0)

    def test_particoes_curriculo_e_formato_nativo(self):
        with tempfile.TemporaryDirectory() as root:
            folder=Path(root)/'dados';other=Path(root)/'outros'
            a=jogos.prepare(folder,17,8,2);b=jogos.prepare(other,17,8,2)
            self.assertEqual(a,b)
            ids=set();prompts=set()
            for split in ('treino','validacao','teste','teste_formulacao'):
                rows=jogos.load(folder,split)
                for r in rows:
                    self.assertNotIn(r['id'],ids);ids.add(r['id'])
                    self.assertNotIn(r['prompt'],prompts);prompts.add(r['prompt'])
                    self.assertEqual(r['formulario']==3,split=='teste_formulacao')
                raw=(folder/(split+'.gtd')).read_bytes();self.assertEqual(raw[:8],b'GTRDATA1')
                count,=struct.unpack_from('<I',raw,8);self.assertEqual(count,len(rows));pos=12
                for r in rows:
                    nq,na=struct.unpack_from('<II',raw,pos);pos+=8
                    self.assertEqual(raw[pos:pos+nq].decode(),r['prompt']);pos+=nq
                    self.assertEqual(raw[pos:pos+na].decode(),r['resposta']);pos+=na
                self.assertEqual(pos,len(raw))
            for level in (1,2,3):
                raw=(folder/f'treino_nivel{level}.gtd').read_bytes()
                self.assertEqual(struct.unpack_from('<I',raw,8)[0],6*8*level)
            original=(folder/'treino.gtd').read_bytes()
            with self.assertRaises(ValueError): jogos.prepare(folder,99,8,2)
            self.assertEqual((folder/'treino.gtd').read_bytes(),original)
            (folder/'teste.jsonl').write_text('{}\n')
            with self.assertRaises(ValueError): jogos.load(folder,'teste')

    def test_modelo_recebe_apenas_prompt(self):
        class Result: returncode=0; stdout='CPU; politica=cpu\nZX\n'; stderr=''
        with patch.object(jogos.subprocess,'run',return_value=Result()) as run:
            result=jogos.infer(Path('/tmp/checkpoint.bin'),'PERGUNTA','cpu','/tmp/memoria')
            self.assertEqual(result['resposta'],'ZX')
            self.assertEqual(run.call_args.args[0][1:],['gerar','/tmp/checkpoint.bin','PERGUNTA','32'])

if __name__=='__main__': unittest.main()
