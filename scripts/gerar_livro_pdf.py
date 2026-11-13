# -*- coding: utf-8 -*-
"""
Gerador do Livro Oficial do GhostLang em PDF.
Compila um documento HTML diagramado com padrão editorial profissional
e converte em PDF via Chromium headless (Chrome/Edge).
"""

import os
import subprocess
import sys

# Garante que a raiz do repositório esteja no PYTHONPATH
sys.path.insert(0, os.path.abspath("."))

def main():
    docs_dir = os.path.abspath("docs")
    pdf_output = os.path.join(docs_dir, "GhostLang_Guia_Oficial_do_Programador.pdf")
    html_output = os.path.join(docs_dir, "GhostLang_Guia_Oficial_do_Programador.html")
    
    print("================================================================")
    print("  Gerador Editorial do Livro Oficial do GhostLang (PDF)")
    print("================================================================")
    print("1. Montando o manuscrito HTML completo com design editorial...")

    # Geraremos um HTML riquíssimo, estruturado com CSS Paged Media e destaque de código.
    # O conteúdo será escrito diretamente em blocos modulares para garantir riqueza e profundidade.
    
    from scripts.book_content import get_book_html
    html_content = get_book_html()
    
    with open(html_output, "w", encoding="utf-8") as f:
        f.write(html_content)
    
    print(f"2. Manuscrito HTML salvo em: {html_output} ({len(html_content)} caracteres)")
    print("3. Localizando motor Chromium para renderizacao de alta fidelidade...")
    
    browsers = [
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
        r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"
    ]
    
    browser_exe = None
    for b in browsers:
        if os.path.exists(b):
            browser_exe = b
            break
            
    if not browser_exe:
        print("ERRO: Nenhum navegador Chromium (Chrome ou Edge) encontrado no sistema.")
        sys.exit(1)
        
    print(f"   Motor selecionado: {browser_exe}")
    print("4. Renderizando PDF (isso pode levar alguns instantes)...")

    # Remove PDF pré-existente para garantir que foi recriado nesta execução
    if os.path.exists(pdf_output):
        try:
            os.remove(pdf_output)
        except Exception as e:
            print(f"Aviso: nao foi possivel remover PDF antigo (pode estar aberto): {e}")
    
    import tempfile
    temp_profile = tempfile.mkdtemp(prefix="ghost_chrome_profile_")

    cmd = [
        browser_exe,
        "--headless=new",
        "--disable-gpu",
        "--no-pdf-header-footer",
        f"--user-data-dir={temp_profile}",
        "--allow-file-access-from-files",
        "--run-all-compositor-stages-before-draw",
        f"--print-to-pdf={pdf_output}",
        html_output
    ]
    
    res = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    
    if res.returncode == 0 and os.path.exists(pdf_output):
        size_mb = os.path.getsize(pdf_output) / (1024 * 1024)
        print(f"5. SUCESSO! Livro PDF gerado com sucesso!")
        print(f"   Arquivo: {pdf_output}")
        print(f"   Tamanho: {size_mb:.2f} MB")
        print("================================================================")
    else:
        print("ERRO na geracao do PDF:")
        print("Stdout:", res.stdout)
        print("Stderr:", res.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
