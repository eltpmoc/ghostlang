import struct
from pathlib import Path

def ler_gtd(caminho):
    dados = Path(caminho).read_bytes()
    if not dados.startswith(b"GTRDATA1"):
        raise ValueError(f"Formato inválido em {caminho}")
    num_records = struct.unpack("<I", dados[8:12])[0]
    return dados[12:], num_records

def mesclar():
    gtd1 = "v4/dados/treino_real.gtd"
    gtd2 = "v4/dados/treino_gpt.gtd"
    saida = "v4/dados/treino_mesclado.gtd"
    
    print(f"Lendo {gtd1}...")
    corpo1, num1 = ler_gtd(gtd1)
    
    print(f"Lendo {gtd2}...")
    corpo2, num2 = ler_gtd(gtd2)
    
    total = num1 + num2
    print(f"Total de registros combinados: {total}")
    
    print(f"Salvando em {saida}...")
    Path(saida).write_bytes(b"GTRDATA1" + struct.pack("<I", total) + corpo1 + corpo2)
    print("Concluído!")

if __name__ == "__main__":
    mesclar()
