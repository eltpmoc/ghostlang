#!/usr/bin/env python3
"""Pesquisa automatica, persistencia entre processos e uso offline."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
HERE = Path(__file__).resolve().parent
APP = HERE.parent
DEVICE = os.environ.get("GHOST_TEST_DEVICE", "cpu")

def chat(lines, folder, offline_network=False):
    env = dict(os.environ, GHOST_KNOWLEDGE_DIR=str(folder), GHOST_STREAM_MS="0",
               GHOST_DEVICE=DEVICE, OMP_NUM_THREADS="2")
    if offline_network:
        env.update(HTTPS_PROXY="http://127.0.0.1:1", https_proxy="http://127.0.0.1:1",
                   ALL_PROXY="http://127.0.0.1:1", NO_PROXY="", no_proxy="")
    result = subprocess.run([str(APP/"build/v3/ia"), "chat"], cwd=APP, env=env,
                            input=lines+"\n/sair\n", capture_output=True, text=True, timeout=50, check=True)
    return result.stdout + result.stderr

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--online", action="store_true")
    args = parser.parse_args()
    report = dict(dispositivo=DEVICE, rede_real="nao executada")
    with tempfile.TemporaryDirectory(prefix="ghost-memoria-test-") as temp:
        folder = Path(temp)
        out = chat("Olá\nQuem é você?\nO que é GhostLang?\nNão entendi", folder, True)
        assert "[Pesquisa automatica]" not in out and chr(96)*3+"ghost" in out, out
        assert not list(folder.glob("*.gkm"))
        for private in ["Qual é meu diagnóstico?", "Explique minha senha secreta",
                        "O que é https://exemplo.com?", "Quem é paciente Maria?",
                        "Qual é o email nome@exemplo.com?", "Leia /home/usuario/segredo"]:
            out = chat(private, folder, True)
            assert "[Pesquisa automatica]" not in out, out
        out = chat("/internet off\nO que é computação quântica?", folder, True)
        assert "[Pesquisa automatica]" not in out and "Tema ausente da memória" in out, out
        out = chat("O que é computação quântica?\nOlá", folder, True)
        assert "[Pesquisa automatica]" in out and "[Internet indisponivel]" in out, out
        assert "[Conversa local] Olá!" in out and not list(folder.glob("*.gkm")), out
        report.update(decisao_local="ok", temas_pessoais="nao enviados nos casos testados",
                      falha_de_rede="nao salva falhas; chat continua")
        if args.online:
            first = chat("O que é computação quântica?\n/memoria", folder)
            assert "[Pesquisa automatica]" in first and "Conhecimento salvo" in first, first
            files = list(folder.glob("*.gkm"))
            assert len(files) == 1
            before = files[0].read_bytes()
            second = chat("/internet off\nExplique a computação quântica.\n/limpar\n/memoria", folder, True)
            assert "[Memoria local]" in second and "https://pt.wikipedia.org/wiki/" in second, second
            assert "[Pesquisa automatica]" not in second and "1 registros locais" in second, second
            assert files[0].read_bytes() == before
            third = chat("Como funciona a computação quântica?", folder, True)
            assert "[Memoria local]" in third and "[Pesquisa automatica]" not in third, third
            fourth = chat("Quem é o presidente da França?", folder)
            assert "[Consulta à fonte oficial]" in fourth and "Conhecimento salvo" in fourth, fourth
            assert len(list(folder.glob("*.gkm"))) == 2
            typo = chat("/internet off\nQual é o presidente da Frnaça?", folder, True)
            assert "[Memoria local]" in typo and "[Pesquisa automatica]" not in typo, typo
            report.update(rede_real="Wikipedia e Presidencia francesa: OK", persistencia="entre processos: OK",
                          reformulacoes="mesmo tema sem nova busca", memoria_offline="OK",
                          fontes_observadas=first+"\n"+second+"\n"+fourth)
    path = HERE/f"TESTES_MEMORIA_{DEVICE}.json"
    path.write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n")
    print(json.dumps({k:v for k,v in report.items() if k != "fontes_observadas"}, ensure_ascii=False, indent=2))
if __name__ == "__main__":
    main()
