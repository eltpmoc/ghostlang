#!/usr/bin/env python3
import copy
import json
from pathlib import Path
import random
import tempfile
import unittest
import jogos
import painel

class PainelTest(unittest.TestCase):
    def row(self):
        row=jogos.make_case(random.Random(1),'copia',1,0)
        row.update(correta=True,referencia_frequente='ZZ',acerto_frequente=False)
        return row
    def report(self,row):
        return {'checkpoint':'modelo.bin','sha256':'a'*64,'acertos':1,'particao':'teste','data_utc':'2026-01-01',
                'fonte_dados_sha256':'b'*64,'metodo':'pesos','respostas':[row]}
    def test_score_is_recomputed(self):
        row=self.row(); row['correta']=False
        with self.assertRaises(ValueError): painel.normalize_rows([row])
        row=self.row(); row['resposta']='ERRADO'
        with self.assertRaises(ValueError): painel.normalize_rows([row])
        row['correta']=False
        self.assertFalse(painel.normalize_rows([row])[0]['correct'])
    def test_state_and_duplicate_validation(self):
        row=self.row(); row['estado']['fita']='ALTERADO'
        with self.assertRaises(ValueError): painel.normalize_rows([row])
        row=self.row()
        with self.assertRaises(ValueError): painel.normalize_rows([row,row])
    def test_same_seed_is_not_extra_evidence(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder); row=self.row(); match={'checkpoint':'modelo.bin','sha256':'a'*64,'acertos':1,'seed':1,'rodadas':[row]}
            for n in range(3): (root/f'match{n}.json').write_text(json.dumps(match))
            distinct=copy.deepcopy(match); distinct['sha256']='b'*64
            (root/'other-model.json').write_text(json.dumps(distinct))
            data=painel.collect(root)
            self.assertEqual(data['match_files'],4);self.assertEqual(len(data['matches']),2)
            self.assertEqual(sorted(x['repeats'] for x in data['matches']),[1,3])
    def test_partial_samples_remain_separate(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder); first=self.row(); second=jogos.make_case(random.Random(2),'copia',1,0);second['correta']=True
            one=self.report(first);two=copy.deepcopy(one);two['respostas'].append(second);two['acertos']=2
            (root/'one.json').write_text(json.dumps(one));(root/'two.json').write_text(json.dumps(two))
            data=painel.collect(root)
            self.assertEqual(sorted(len(r['rows']) for r in data['reports']),[1,2])
    def test_html_escapes_report_content(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);row=self.row();attack='</script><script>window.injetado=true</script>'
            row['prompt']=attack
            (root/'input.json').write_text(json.dumps(self.report(row)))
            path=root/'panel.html';painel.build(root,path);html=path.read_text()
            self.assertNotIn(attack,html);self.assertIn('\\u003c/script\\u003e',html)
    def test_bad_reports_are_visible(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);bad=self.report(self.row());bad['acertos']=90
            (root/'bad.json').write_text(json.dumps(bad));data=painel.collect(root)
            self.assertFalse(data['reports']);self.assertEqual(data['skipped'][0]['file'],'bad.json')

if __name__=='__main__': unittest.main()
