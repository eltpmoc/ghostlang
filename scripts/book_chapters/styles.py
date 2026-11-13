# -*- coding: utf-8 -*-
"""
Estilos CSS profissionais para o livro GhostLang (CSS Paged Media e Design Editorial).
"""

BOOK_CSS = """
@page {
    size: A4;
    margin: 20mm 18mm 20mm 18mm;
    @top-right {
        content: "GhostLang — Guia Oficial do Programador";
        font-size: 8pt;
        color: #94a3b8;
        font-family: 'Segoe UI', system-ui, sans-serif;
    }
    @bottom-center {
        content: counter(page);
        font-size: 9pt;
        font-weight: bold;
        color: #64748b;
        font-family: 'Segoe UI', system-ui, sans-serif;
    }
}

@page:first {
    margin: 0;
    @top-right { content: none; }
    @bottom-center { content: none; }
}

* {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
}

body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    font-size: 11pt;
    line-height: 1.65;
    color: #1e293b;
    background-color: #ffffff;
}

/* Capa do Livro */
.cover-page {
    page-break-before: always;
    page-break-after: always;
    width: 100vw;
    height: 100vh;
    min-height: 297mm;
    background: linear-gradient(135deg, #090d16 0%, #0f172a 45%, #1e1b4b 100%);
    color: #f8fafc;
    padding: 25mm 20mm;
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    position: relative;
    overflow: hidden;
}

.cover-badge-container {
    display: flex;
    gap: 8px;
    margin-bottom: 25px;
}

.cover-badge {
    background: rgba(56, 189, 248, 0.15);
    border: 1px solid #38bdf8;
    color: #38bdf8;
    padding: 4px 12px;
    border-radius: 20px;
    font-size: 8.5pt;
    font-weight: 600;
    letter-spacing: 0.5px;
    text-transform: uppercase;
}

.cover-badge.gold {
    background: rgba(245, 158, 11, 0.15);
    border-color: #f59e0b;
    color: #fbbf24;
}

.cover-title {
    font-size: 42pt;
    font-weight: 900;
    letter-spacing: -1.5px;
    line-height: 1.05;
    background: linear-gradient(90deg, #ffffff 0%, #38bdf8 50%, #c084fc 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    margin-bottom: 15px;
}

.cover-subtitle {
    font-size: 15pt;
    font-weight: 400;
    color: #94a3b8;
    line-height: 1.4;
    max-width: 580px;
    margin-bottom: 30px;
}

.cover-art {
    margin: 20px 0;
    text-align: center;
}

.cover-art svg {
    max-width: 100%;
    filter: drop-shadow(0 0 25px rgba(56, 189, 248, 0.3));
}

.cover-features {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 15px;
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(255, 255, 255, 0.1);
    padding: 16px;
    border-radius: 12px;
    backdrop-filter: blur(8px);
    margin-bottom: 25px;
}

.cover-feature-item {
    font-size: 9pt;
    color: #cbd5e1;
    display: flex;
    align-items: center;
    gap: 8px;
}

.cover-feature-item span {
    color: #38bdf8;
    font-weight: bold;
}

.cover-footer {
    border-top: 1px solid rgba(255, 255, 255, 0.15);
    padding-top: 15px;
    display: flex;
    justify-content: space-between;
    align-items: flex-end;
    font-size: 9pt;
    color: #64748b;
}

.cover-author-name {
    color: #f1f5f9;
    font-size: 11pt;
    font-weight: bold;
}

/* Folha de Rosto e Sumário */
.title-page {
    page-break-before: always;
    page-break-after: always;
    padding-top: 40mm;
    text-align: center;
}

.toc-container {
    page-break-before: always;
    page-break-after: always;
    padding: 10mm 0;
}

.toc-title {
    font-size: 24pt;
    font-weight: 800;
    color: #0f172a;
    border-bottom: 3px solid #0284c7;
    padding-bottom: 8px;
    margin-bottom: 25px;
}

.toc-item {
    display: flex;
    justify-content: space-between;
    align-items: baseline;
    padding: 7px 0;
    border-bottom: 1px dotted #cbd5e1;
    font-size: 10.5pt;
}

.toc-chapter-title {
    font-weight: 700;
    color: #1e293b;
}

.toc-page-num {
    font-weight: bold;
    color: #0284c7;
}

/* Estrutura de Capítulos */
.chapter {
    page-break-before: always;
    padding-top: 10mm;
    margin-bottom: 30px;
}

.chapter-badge {
    display: inline-block;
    background: #e0f2fe;
    color: #0369a1;
    font-weight: 800;
    font-size: 9pt;
    text-transform: uppercase;
    padding: 3px 10px;
    border-radius: 6px;
    letter-spacing: 1px;
    margin-bottom: 10px;
}

.chapter-title {
    font-size: 24pt;
    font-weight: 800;
    color: #0f172a;
    line-height: 1.15;
    margin-bottom: 15px;
    letter-spacing: -0.5px;
}

.chapter-intro {
    font-size: 11pt;
    font-style: italic;
    color: #475569;
    background: #f8fafc;
    border-left: 4px solid #38bdf8;
    padding: 12px 16px;
    margin-bottom: 25px;
    border-radius: 0 8px 8px 0;
}

/* Seções e Cabeçalhos */
h2 {
    font-size: 16pt;
    font-weight: 700;
    color: #0f172a;
    margin-top: 25px;
    margin-bottom: 12px;
    padding-bottom: 5px;
    border-bottom: 1.5px solid #e2e8f0;
    page-break-after: avoid;
}

h3 {
    font-size: 12.5pt;
    font-weight: 600;
    color: #1e293b;
    margin-top: 18px;
    margin-bottom: 8px;
    page-break-after: avoid;
}

p {
    font-size: 11pt;
    line-height: 1.65;
    margin-bottom: 12px;
    text-align: justify;
}

ul, ol {
    font-size: 11pt;
    line-height: 1.65;
    margin-bottom: 14px;
    padding-left: 22px;
}

li {
    font-size: 11pt;
    margin-bottom: 6px;
}

/* Blocos de Código */
.code-block {
    background: #0f172a;
    border-radius: 8px;
    margin: 15px 0 20px 0;
    overflow: hidden;
    break-inside: avoid;
    box-shadow: 0 4px 12px rgba(15, 23, 42, 0.15);
    border: 1px solid #1e293b;
}

.code-header {
    background: #1e293b;
    color: #94a3b8;
    padding: 6px 14px;
    font-size: 9.5pt;
    font-family: Consolas, "Fira Code", monospace;
    display: flex;
    justify-content: space-between;
    border-bottom: 1px solid #334155;
}

.code-filename {
    color: #38bdf8;
    font-weight: bold;
}

pre {
    padding: 12px 14px;
    font-family: Consolas, "Fira Code", "Courier New", monospace;
    font-size: 11pt;
    line-height: 1.5;
    color: #f8fafc;
    overflow-x: auto;
    margin: 0;
}

code {
    font-family: Consolas, "Fira Code", "Courier New", monospace;
    font-size: 11pt;
}

p code, li code, td code, .exercise-desc code, .exercise-hint code, .callout code {
    font-family: Consolas, "Fira Code", "Courier New", monospace;
    font-size: 10pt;
    background: #f1f5f9;
    color: #0f172a;
    padding: 1px 5px;
    border-radius: 4px;
    border: 1px solid #e2e8f0;
}

/* Destaque de Sintaxe */
.kw { color: #38bdf8; font-weight: bold; }
.type { color: #fbbf24; font-weight: 600; }
.fn { color: #c084fc; font-weight: 600; }
.str { color: #34d399; }
.num { color: #f97316; }
.cm { color: #64748b; font-style: italic; }
.op { color: #f43f5e; }

/* Callouts Editoriais */
.callout {
    padding: 12px 16px;
    border-radius: 8px;
    margin: 16px 0;
    break-inside: avoid;
    font-size: 11pt;
    line-height: 1.6;
}

.callout-title {
    font-weight: 700;
    margin-bottom: 4px;
    display: flex;
    align-items: center;
    gap: 6px;
}

.callout-tip {
    background: #f0fdf4;
    border-left: 4px solid #22c55e;
    color: #14532d;
}

.callout-quantum {
    background: #eff6ff;
    border-left: 4px solid #3b82f6;
    color: #1e3a8a;
}

.callout-ai {
    background: #faf5ff;
    border-left: 4px solid #a855f7;
    color: #581c87;
}

.callout-warning {
    background: #fffbeb;
    border-left: 4px solid #f59e0b;
    color: #78350f;
}

/* Tabelas */
table {
    width: 100%;
    border-collapse: collapse;
    margin: 15px 0 20px 0;
    font-size: 9pt;
    break-inside: avoid;
}

th {
    background: #0f172a;
    color: #f8fafc;
    padding: 8px 10px;
    text-align: left;
    font-weight: 600;
}

td {
    padding: 7px 10px;
    border-bottom: 1px solid #e2e8f0;
}

tr:nth-child(even) td {
    background-color: #f8fafc;
}

/* Seções de Atividades Práticas */
.exercises-section {
    background: #f8fafc;
    border: 1.5px solid #cbd5e1;
    border-radius: 10px;
    padding: 18px 20px;
    margin-top: 25px;
    break-inside: avoid;
}

.exercises-title {
    font-size: 13pt;
    font-weight: 800;
    color: #0f172a;
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 12px;
    border-bottom: 2px solid #38bdf8;
    padding-bottom: 6px;
}

.exercise-item {
    margin-bottom: 16px;
    padding-bottom: 12px;
    border-bottom: 1px dashed #e2e8f0;
}

.exercise-item:last-child {
    border-bottom: none;
    margin-bottom: 0;
    padding-bottom: 0;
}

.exercise-header {
    display: flex;
    justify-content: space-between;
    margin-bottom: 4px;
}

.exercise-name {
    font-weight: 700;
    color: #1e293b;
    font-size: 11.5pt;
}

.exercise-badge {
    font-size: 8pt;
    font-weight: bold;
    padding: 2px 8px;
    border-radius: 12px;
    text-transform: uppercase;
}

.badge-easy { background: #dcfce7; color: #15803d; }
.badge-medium { background: #fef3c7; color: #b45309; }
.badge-hard { background: #fee2e2; color: #b91c1c; }

.exercise-desc {
    font-size: 11pt;
    line-height: 1.6;
    color: #334155;
    margin-bottom: 6px;
}

.exercise-hint {
    font-size: 10pt;
    color: #64748b;
    font-style: italic;
}

/* Tipografia Matemática Elegante */
.math {
    font-family: 'Cambria Math', 'Times New Roman', Georgia, serif;
    font-style: italic;
    color: #0f172a;
}

.math-num {
    font-family: 'Cambria Math', 'Times New Roman', Georgia, serif;
    font-style: normal;
    color: #0f172a;
}
"""
