# -*- coding: utf-8 -*-
"""
Capa, Folha de Rosto, Ficha Técnica e Sumário do Livro Oficial GhostLang.
"""

def get_preface_html():
    return """
<!-- CAPA DO LIVRO -->
<div class="cover-page">
    <div>
        <div class="cover-badge-container">
            <span class="cover-badge">Linguagem Oficial v1.5.0</span>
            <span class="cover-badge gold">100% Consolidado</span>
            <span class="cover-badge">Quantum & AI Native</span>
        </div>
        <h1 class="cover-title">GHOSTLANG</h1>
        <p class="cover-subtitle">Guia Definitivo do Programador — Da Sintaxe Fundamental à Computação Quântica, Álgebra Hiper-Complexa G e Inteligência Artificial</p>
    </div>

    <div class="cover-art">
        <svg width="460" height="200" viewBox="0 0 460 200" fill="none" xmlns="http://www.w3.org/2000/svg">
            <circle cx="80" cy="100" r="40" stroke="#38bdf8" stroke-width="2.5" stroke-dasharray="4 4" fill="rgba(56,189,248,0.06)"/>
            <circle cx="80" cy="100" r="5" fill="#38bdf8"/>
            <text x="70" y="105" fill="#38bdf8" font-family="sans-serif" font-size="12" font-weight="bold">|ψ⟩</text>
            
            <line x1="120" y1="100" x2="190" y2="60" stroke="#818cf8" stroke-width="2"/>
            <line x1="120" y1="100" x2="190" y2="140" stroke="#818cf8" stroke-width="2"/>
            
            <!-- Nó Rede Neural 1 -->
            <rect x="190" y="40" width="80" height="40" rx="8" fill="#1e1b4b" stroke="#c084fc" stroke-width="2"/>
            <text x="210" y="65" fill="#f8fafc" font-family="monospace" font-size="11" font-weight="bold">W1 &gt;&gt; Q</text>

            <!-- Nó Rede Neural 2 -->
            <rect x="190" y="120" width="80" height="40" rx="8" fill="#1e1b4b" stroke="#38bdf8" stroke-width="2"/>
            <text x="205" y="145" fill="#f8fafc" font-family="monospace" font-size="11" font-weight="bold">GhostQubit</text>

            <line x1="270" y1="60" x2="340" y2="100" stroke="#c084fc" stroke-width="2"/>
            <line x1="270" y1="140" x2="340" y2="100" stroke="#c084fc" stroke-width="2"/>

            <!-- Nó Saída / Tensor -->
            <circle cx="370" cy="100" r="35" fill="rgba(192,132,252,0.12)" stroke="#f43f5e" stroke-width="2.5"/>
            <circle cx="370" cy="100" r="6" fill="#f43f5e"/>
            <text x="350" y="105" fill="#fb7185" font-family="monospace" font-size="12" font-weight="bold">G 4D</text>
        </svg>
    </div>

    <div class="cover-features">
        <div class="cover-feature-item"><span>✓</span> Transpilação C++17 Nativa com -O3</div>
        <div class="cover-feature-item"><span>✓</span> Qubits, Portas e Circuitos Quânticos</div>
        <div class="cover-feature-item"><span>✓</span> Deep Learning Nativo com Operador &gt;&gt;</div>
        <div class="cover-feature-item"><span>✓</span> Álgebra Hiper-Complexa G e Tensores ND</div>
        <div class="cover-feature-item"><span>✓</span> Autograd Reverso Clássico-Quântico</div>
        <div class="cover-feature-item"><span>✓</span> Aceleração GPU CUDA, OpenMP e Vulkan</div>
    </div>

    <div class="cover-footer">
        <div>
            <div class="cover-author-name">Equipe de Engenharia GhostLang</div>
            <div>Edição Oficial 2026 • Windows, Linux e macOS</div>
        </div>
        <div style="text-align: right;">
            <div style="color: #38bdf8; font-weight: bold;">Manual e Guia Prático</div>
            <div>github.com/eltpmoc/ghostlang</div>
        </div>
    </div>
</div>

<!-- FOLHA DE ROSTO -->
<div class="title-page">
    <div style="font-size: 14pt; color: #0284c7; font-weight: bold; letter-spacing: 2px; text-transform: uppercase; margin-bottom: 20px;">
        Documentação Oficial e Livro Técnico de Referência
    </div>
    <h1 style="font-size: 32pt; font-weight: 900; color: #0f172a; margin-bottom: 10px;">GHOSTLANG</h1>
    <h2 style="font-size: 16pt; font-weight: 500; color: #475569; border: none; margin-bottom: 40px;">
        Guia Definitivo do Programador: Da Sintaxe ao Universo Quântico e IA
    </h2>
    <div style="width: 120px; height: 3px; background: #0284c7; margin: 0 auto 50px auto;"></div>
    
    <div style="margin-top: 60px; font-size: 10pt; color: #64748b; line-height: 1.8;">
        <strong>Autor:</strong> Engenharia e Comunidade GhostLang<br>
        <strong>Versão Coberta:</strong> 1.5.0 (Status Oficial: 100% Consolidado - 520/520 pts)<br>
        <strong>Padrão de Compilação:</strong> C++17 ISO / LLVM Clang / MSVC / GCC 11+<br>
        <strong>Aceleração de Hardware:</strong> NVIDIA CUDA (Compute 6.1 a 8.9), OpenMP Multi-Core, Apple Metal, Vulkan<br>
        <strong>Repositório do Projeto:</strong> <code>https://github.com/eltpmoc/ghostlang</code>
    </div>
</div>

<!-- PREFÁCIO -->
<div class="chapter">
    <span class="chapter-badge">Prefácio</span>
    <h1 class="chapter-title">A Filosofia do GhostLang</h1>
    <div class="chapter-intro">
        "Por que criar uma nova linguagem de programação em um mundo saturado de opções? Porque a computação clássica, a computação quântica e a inteligência artificial não podem mais viver em mundos isolados."
    </div>

    <p>
        Durante décadas, a ciência da computação operou sob uma clara divisão de trabalho. O desenvolvimento de sistemas de alta performance pertencia ao C e C++; a análise de dados e prototipagem de redes neurais pertencia ao Python; e os simuladores de computação quântica eram mantidos como bibliotecas externas complexas, difíceis de instalar e desacopladas dos compiladores nativos.
    </p>

    <p>
        O <strong>GhostLang</strong> nasceu com a missão ousada de romper essas barreiras. Criada do zero com foco em <strong>três pilares inegociáveis</strong>:
    </p>

    <ul>
        <li><strong>Desempenho Nativo Extremo:</strong> Programas GhostLang são analisados em uma Árvore Sintática Abstrata (AST) rigorosa e transpilados diretamente para código C++17 ultra-otimizado com suporte a OpenMP e CUDA, gerando binários estáticos sem qualquer dependência de máquinas virtuais lentas ou interpretadores pesados.</li>
        <li><strong>Computação Quântica como Cidadã de Primeira Classe:</strong> Qubits, registradores quânticos (<code>qreg</code>), circuitos variacionais (<code>circuit</code>), matrizes de densidade e medições probabilísticas não são pacotes de terceiros. Eles fazem parte da própria gramática e do runtime C++ do GhostLang.</li>
        <li><strong>Inteligência Artificial e Álgebra Hiper-Complexa Nativas:</strong> O operador de pipeline de camadas neurais <code>>></code>, diferenciação automática reversa (Autograd), números 4D de álgebra fantasma <span class="math"><b>G</b></span>, e tensores N-dimensionais estão embutidos no coração da linguagem.</li>
    </ul>

    <div class="callout callout-tip">
        <div class="callout-title">💡 Como estudar com este livro</div>
        Cada capítulo foi cuidadosamente estruturado em três níveis de aprendizado:
        <strong>1. Fundamentação Teórica e Sintaxe Exata</strong>;
        <strong>2. Códigos Exemplo Canônicos</strong>; e
        <strong>3. Laboratório Prático de Exercícios</strong> com desafios graduais (Aquecimento, Aplicação e Desafio Avançado). Não hesite em abrir o terminal, compilar com <code>ghost run</code> e experimentar no REPL!
    </div>
</div>

<!-- SUMÁRIO DO LIVRO -->
<div class="toc-container">
    <h2 class="toc-title">Sumário Geral</h2>
    
    <div class="toc-item">
        <span class="toc-chapter-title">Capítulo 1: Introdução, Instalação e Arquitetura da Toolchain</span>
        <span class="toc-page-num">Cap. 1</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Capítulo 2: Sintaxe Fundamental, Tipos Primitivos e Variáveis</span>
        <span class="toc-page-num">Cap. 2</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Capítulo 3: Controle de Fluxo Estruturado, Coleções e Exceções</span>
        <span class="toc-page-num">Cap. 3</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Capítulo 4: Funções, Expressões Lambdas, Closures e Concorrência</span>
        <span class="toc-page-num">Cap. 4</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Capítulo 5: Programação Orientada a Objetos, Herança e Módulos</span>
        <span class="toc-page-num">Cap. 5</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Capítulo 6: Álgebra Hiper-Complexa G (4D) e Tensores N-Dimensionais</span>
        <span class="toc-page-num">Cap. 6</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Capítulo 7: Computação Quântica Nativa (Qubits, Portas, Circuitos e Algoritmos)</span>
        <span class="toc-page-num">Cap. 7</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Capítulo 8: Inteligência Artificial, Redes Neurais e Autograd Reverso</span>
        <span class="toc-page-num">Cap. 8</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Capítulo 9: Machine Learning Quântico (QML), VQE, QAOA e TransformerG</span>
        <span class="toc-page-num">Cap. 9</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Capítulo 10: Biblioteca Padrão, Rede HTTP RESTful, SQLite e GUI</span>
        <span class="toc-page-num">Cap. 10</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Capítulo 11: A Toolchain Oficial (CLI ghost, REPL, Debugger e CTest)</span>
        <span class="toc-page-num">Cap. 11</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Apêndice A: Laboratório Prático — Soluções Comentadas dos Desafios</span>
        <span class="toc-page-num">Apênd. A</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Apêndice B: Tabela Periódica Completa das Portas Quânticas e Métodos qreg</span>
        <span class="toc-page-num">Apênd. B</span>
    </div>
    <div class="toc-item">
        <span class="toc-chapter-title">Apêndice C: Cheat Sheet de Sintaxe Rápida e Comandos</span>
        <span class="toc-page-num">Apênd. C</span>
    </div>
</div>
"""
