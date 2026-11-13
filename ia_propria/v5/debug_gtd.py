import struct
from pathlib import Path

def debug():
    p = Path("v4/dados/treino_real.gtd")
    raw = p.read_bytes()
    if raw[:8] != b"GTRDATA1":
        print("Magic invalido")
        return
    count = struct.unpack("<I", raw[8:12])[0]
    print(f"Total exemplos: {count}")
    pos = 12
    max_len = 0
    for i in range(count):
        if pos >= len(raw):
            print("EOF prematuro")
            break
        np, na = struct.unpack("<II", raw[pos:pos+8])
        pos += 8
        if np+na+1 > max_len:
            max_len = np+na+1
        if np+na+1 > 1024:
            print(f"Exemplo {i} excede 1024! np={np}, na={na}, len={np+na+1}")
            break
        pos += np + na
    print(f"Max length: {max_len}")

debug()
