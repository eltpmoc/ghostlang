# -*- coding: utf-8 -*-
"""
Módulo mestre de composição do Livro Oficial GhostLang.
Agrega todos os capítulos, apêndices e estilos em um manuscrito HTML completo.
"""

from scripts.book_chapters.styles import BOOK_CSS
from scripts.book_chapters.preface import get_preface_html
from scripts.book_chapters.ch01_fundamentos import get_ch01_html
from scripts.book_chapters.ch02_controle_oop import get_ch02_html
from scripts.book_chapters.ch03_algebra_tensores import get_ch03_html
from scripts.book_chapters.ch04_quantica import get_ch04_html
from scripts.book_chapters.ch05_ia_deeplearning import get_ch05_html
from scripts.book_chapters.ch06_qml_hibrido import get_ch06_html
from scripts.book_chapters.ch07_stdlib_toolchain import get_ch07_html
from scripts.book_chapters.appendices import get_appendices_html

def get_book_html():
    sections = [
        get_preface_html(),
        get_ch01_html(),
        get_ch02_html(),
        get_ch03_html(),
        get_ch04_html(),
        get_ch05_html(),
        get_ch06_html(),
        get_ch07_html(),
        get_appendices_html()
    ]
    
    body_content = "\n".join(sections)
    
    full_html = f"""<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="utf-8">
    <title>GhostLang — Guia Oficial do Programador</title>
    <style>
{BOOK_CSS}
    </style>
</head>
<body>
{body_content}
</body>
</html>
"""
    return full_html
