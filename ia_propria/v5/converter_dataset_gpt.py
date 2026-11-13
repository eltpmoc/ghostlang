import pandas as pd
import ast
import struct
from pathlib import Path

def converter():
    arquivo_csv = Path("v4/dados/train.csv")
    arquivo_gtd = Path("v4/dados/treino_gpt.gtd")
    
    print(f"Lendo {arquivo_csv}...")
    df = pd.read_csv(arquivo_csv)
    
    records = []
    MAX_CONTEXT = 1024 # Limite do nosso transformer
    
    print("Processando conversas...")
    pt_keywords = [" que ", " não ", " para ", " com ", " um ", " uma ", " os ", " as ", " de ", " do ", " da ", " em ", " no ", " na ", " como "]
    
    for idx, row in df.iterrows():
        try:
            text = str(row['conversations'])
            text = text.replace("}\n {", "}, {")
            text = text.replace("}\n{", "},{")
            text = text.replace("} {", "}, {")
            conv = ast.literal_eval(text)
            if len(conv) >= 2 and conv[0]['from'] == 'human' and conv[1]['from'] == 'gpt':
                prompt = "Usuário: " + conv[0]['value'].strip() + "\nAssistente:"
                resposta = " " + conv[1]['value'].strip()
                
                prompt = "".join(c for c in prompt if ord(c) == 9 or ord(c) == 10 or ord(c) >= 32)
                resposta = "".join(c for c in resposta if ord(c) == 9 or ord(c) == 10 or ord(c) >= 32)
                
                # Filtrar grosseiramente para português
                text_lower = prompt.lower() + resposta.lower()
                pt_keywords = [" que ", " não ", " para ", " com ", " um ", " uma ", " os ", " as ", " de ", " do ", " da ", " em ", " no ", " na ", " como "]
                is_pt = any(kw in text_lower for kw in pt_keywords)
                
                if is_pt and len(prompt.encode('utf-8')) + len(resposta.encode('utf-8')) + 1 <= MAX_CONTEXT:
                    records.append((prompt.encode('utf-8'), resposta.encode('utf-8')))
        except Exception as e:
            if len(records) == 0:
                print(f"Erro no parsing: {e} na linha {row['conversations'][:100]}")
            continue
            
    print(f"Total de registros aproveitáveis (<= {MAX_CONTEXT} bytes): {len(records)}")
    
    # Formato binário do GhostLang
    print(f"Salvando em {arquivo_gtd}...")
    raw = b"GTRDATA1" + struct.pack("<I", len(records))
    for q, a in records:
        raw += struct.pack("<II", len(q), len(a)) + q + a
        
    arquivo_gtd.write_bytes(raw)
    print("Concluído!")

if __name__ == "__main__":
    converter()
