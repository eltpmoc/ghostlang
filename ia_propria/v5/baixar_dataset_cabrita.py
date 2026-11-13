#!/usr/bin/env python3
import subprocess
import sys
import json
import struct
import hashlib
from pathlib import Path

def install_and_import(package):
    try:
        import datasets
    except ImportError:
        print(f"Instalando {package} temporariamente...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", package, "pyarrow"])
    finally:
        globals()[package] = __import__(package)

def prompt_format(q, inp=""):
    if inp:
        return f"Usuário: {q}\n{inp}\nAssistente:"
    return f"Usuário: {q}\nAssistente:"

def main():
    folder = Path(__file__).resolve().parent / "dados"
    folder.mkdir(parents=True, exist_ok=True)
    
    install_and_import('datasets')
    from datasets import load_dataset
    
    print("Baixando dataset 'dominguesm/alpaca-data-pt-br' do Hugging Face (52k instruções em Português)...")
    try:
        dataset = load_dataset("dominguesm/alpaca-data-pt-br", split="train")
    except Exception as e:
        print(f"Erro ao baixar do Hugging Face: {e}")
        return

    # Partir do treino ampliado para não perder as regras anteriores
    base_file = folder / "treino_ampliado.jsonl"
    if not base_file.exists():
        base_file = folder / "treino.jsonl"
        
    rows = [json.loads(x) for x in base_file.read_text(encoding="utf-8").splitlines()] if base_file.exists() else []
    seen = {r["prompt"] for r in rows}
    
    # Processar o dataset baixado
    adicionados = 0
    ignorados_tamanho = 0
    
    for item in dataset:
        instruction = item.get("instruction", "").strip()
        inp = item.get("input", "").strip()
        output = item.get("output", "").strip()
        
        p = prompt_format(instruction, inp)
        a = output
        
        # O limite da V4 é 384 bytes
        if p not in seen:
            if len((p + a).encode('utf-8')) + 1 <= 384:
                seen.add(p)
                rows.append(dict(categoria="alpaca_pt", prompt=p, resposta=a))
                adicionados += 1
            else:
                ignorados_tamanho += 1

    print(f"Dataset baixado! Adicionados: {adicionados} registros (Ignorados pelo limite de 384 bytes: {ignorados_tamanho})")
    
    # Salvar em GTD e JSONL
    raw = b"GTRDATA1" + struct.pack("<I", len(rows))
    for r in rows:
        p = r["prompt"].encode('utf-8')
        a = r["resposta"].encode('utf-8')
        raw += struct.pack("<II", len(p), len(a)) + p + a
        
    path_gtd = folder / "treino_real.gtd"
    path_gtd.write_bytes(raw)
    
    path_jsonl = folder / "treino_real.jsonl"
    path_jsonl.write_text("".join(json.dumps(r, ensure_ascii=False) + "\n" for r in rows), encoding="utf-8")
    
    manifest = dict(
        registros=len(rows),
        sha256=hashlib.sha256(raw).hexdigest(),
        base="treino_ampliado.jsonl",
        alteracao="Adicionado dataset Alpaca-PT (dominguesm/alpaca-data-pt-br) filtrado para o limite de 384 bytes"
    )
    
    (folder / "alpaca_pt_manifesto.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("Arquivos treino_real.gtd e treino_real.jsonl gerados!")

if __name__ == "__main__":
    main()
