# -*- coding: utf-8 -*-
"""
Apêndice A (Gabarito de Exercícios Resolvidos), Apêndice B (Tabela de Portas),
Apêndice C (Cheat Sheet) e Contracapa do Livro GhostLang.
"""

def get_appendices_html():
    return """
<!-- APÊNDICE A: GABARITO E RESOLUÇÕES -->
<div class="chapter">
    <span class="chapter-badge">Apêndice A</span>
    <h1 class="chapter-title">Laboratório Prático: Soluções e Códigos Comentados</h1>
    <div class="chapter-intro">
        Confira as resoluções comentadas de todos os exercícios práticos propostos nos capítulos deste livro para validar o seu aprendizado em GhostLang.
    </div>

    <h2>A.1 Resolução do Exercício 1.1: Calculadora de Impedância Complexa</h2>
    <div class="code-block">
        <div class="code-header"><span class="code-filename">solucoes/ex1_1_impedancia.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">float</span> R = <span class="num">50.0</span>;
    <span class="type">float</span> XL = <span class="num">30.0</span>;
    <span class="type">complex</span> Z = R + XL * <span class="num">1.0i</span>;
    
    <span class="type">float</span> modulo = sqrt(R * R + XL * XL);
    <span class="fn">println</span>(<span class="str">f"Impedancia Complexa Z: {Z} ohms"</span>);
    <span class="fn">println</span>(<span class="str">f"Modulo da Impedancia |Z|: {modulo} ohms"</span>);
}
        </pre>
    </div>

    <h2>A.2 Resolução do Exercício 4.1: Gerador Quântico QRNG</h2>
    <div class="code-block">
        <div class="code-header"><span class="code-filename">solucoes/ex4_1_qrng.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/quantum.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">qreg</span> q(<span class="num">8</span>); <span class="cm">// 8 qubits para compor 1 byte puro</span>
    q.h_all();  <span class="cm">// Coloca todos em superposição equiprovável</span>

    <span class="type">int</span> byte_aleatorio = <span class="num">0</span>;
    <span class="kw">for</span> (<span class="type">int</span> i = <span class="num">0</span>; i &lt; <span class="num">8</span>; i = i + <span class="num">1</span>) {
        <span class="type">int</span> bit = q.measure(i);
        <span class="kw">if</span> (bit == <span class="num">1</span>) {
            byte_aleatorio = byte_aleatorio + (pow(<span class="num">2</span>, i) <span class="kw">as</span> <span class="type">int</span>);
        }
    }
    <span class="fn">println</span>(<span class="str">f"Numero quantico aleatorio gerado (0-255): {byte_aleatorio}"</span>);
}
        </pre>
    </div>

    <h2>A.3 Resolução do Exercício 4.2: Teletransporte Quântico Completo</h2>
    <div class="code-block">
        <div class="code-header"><span class="code-filename">solucoes/ex4_2_teletransporte.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/quantum.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">qreg</span> q(<span class="num">3</span>); <span class="cm">// q0: estado de Alice, q1-q2: par entrelaçado EPR</span>

    <span class="cm">// 1. Alice prepara seu qubit secreto com rotação</span>
    q.ry(<span class="num">0</span>, <span class="num">3.14159265</span> / <span class="num">3.0</span>);

    <span class="cm">// 2. Cria par EPR entre Bob (q2) e Alice (q1)</span>
    q.h(<span class="num">1</span>);
    q.cx(<span class="num">1</span>, <span class="num">2</span>);

    <span class="cm">// 3. Alice mede em base de Bell</span>
    q.cx(<span class="num">0</span>, <span class="num">1</span>);
    q.h(<span class="num">0</span>);
    <span class="type">int</span> m0 = q.measure(<span class="num">0</span>);
    <span class="type">int</span> m1 = q.measure(<span class="num">1</span>);

    <span class="cm">// 4. Bob aplica correções unitárias clássicas</span>
    <span class="kw">if</span> (m1 == <span class="num">1</span>) { q.x(<span class="num">2</span>); }
    <span class="kw">if</span> (m0 == <span class="num">1</span>) { q.z(<span class="num">2</span>); }

    <span class="fn">println</span>(<span class="str">"Teletransporte concluído com sucesso! Estado reconstruído em Bob (q2):"</span>);
    q.show();
}
        </pre>
    </div>

    <h2>A.4 Resolução do Exercício 5.1: Classificador de Porta XOR com Rede Neural</h2>
    <div class="code-block">
        <div class="code-header"><span class="code-filename">solucoes/ex5_1_xor.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// Tabela da verdade XOR: 4 amostras com 2 entradas</span>
    <span class="type">matrix</span> X(<span class="num">4</span>, <span class="num">2</span>);
    X.set(<span class="num">0</span>, <span class="num">0</span>, <span class="num">0.0</span>); X.set(<span class="num">0</span>, <span class="num">1</span>, <span class="num">0.0</span>);
    X.set(<span class="num">1</span>, <span class="num">0</span>, <span class="num">0.0</span>); X.set(<span class="num">1</span>, <span class="num">1</span>, <span class="num">1.0</span>);
    X.set(<span class="num">2</span>, <span class="num">0</span>, <span class="num">1.0</span>); X.set(<span class="num">2</span>, <span class="num">1</span>, <span class="num">0.0</span>);
    X.set(<span class="num">3</span>, <span class="num">0</span>, <span class="num">1.0</span>); X.set(<span class="num">3</span>, <span class="num">1</span>, <span class="num">1.0</span>);

    <span class="type">matrix</span> Y(<span class="num">4</span>, <span class="num">1</span>);
    Y.set(<span class="num">0</span>, <span class="num">0</span>, <span class="num">0.0</span>);
    Y.set(<span class="num">1</span>, <span class="num">0</span>, <span class="num">1.0</span>);
    Y.set(<span class="num">2</span>, <span class="num">0</span>, <span class="num">1.0</span>);
    Y.set(<span class="num">3</span>, <span class="num">0</span>, <span class="num">0.0</span>);

    <span class="type">matrix</span> W1 = matrix_random(<span class="num">2</span>, <span class="num">4</span>);
    <span class="type">matrix</span> W2 = matrix_random(<span class="num">4</span>, <span class="num">1</span>);
    <span class="kw">network</span> rede_xor = W1 &gt;&gt; W2;

    rede_xor.optimize(X, Y, <span class="num">200</span>, <span class="num">0.05</span>, <span class="str">"adam"</span>);
    <span class="fn">println</span>(<span class="str">"Predição após 200 épocas:"</span>);
    rede_xor.forward(X).show();
}
        </pre>
    </div>
</div>

<!-- APÊNDICE B: TABELA PERIÓDICA DE PORTAS QUÂNTICAS -->
<div class="chapter">
    <span class="chapter-badge">Apêndice B</span>
    <h1 class="chapter-title">Tabela Periódica Completa das Portas Quânticas</h1>
    <div class="chapter-intro">
        Guia de consulta rápida contendo todas as portas lógicas quânticas, matrizes unitárias correspondentes e assinaturas de método em GhostLang.
    </div>

    <table>
        <thead>
            <tr>
                <th>Porta</th>
                <th>Sintaxe</th>
                <th>Matriz Unitária / Definição</th>
                <th>Descrição</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td><strong>Hadamard (H)</strong></td>
                <td><code>q.h(i)</code></td>
                <td>1/√2 [[1, 1], [1, -1]]</td>
                <td>Cria superposição simétrica de base computacional.</td>
            </tr>
            <tr>
                <td><strong>Pauli-X</strong></td>
                <td><code>q.x(i)</code></td>
                <td>[[0, 1], [1, 0]]</td>
                <td>Inversão de bit quântico (NOT Quântico).</td>
            </tr>
            <tr>
                <td><strong>Pauli-Y</strong></td>
                <td><code>q.y(i)</code></td>
                <td>[[0, -i], [i, 0]]</td>
                <td>Inversão simultânea de bit e fase.</td>
            </tr>
            <tr>
                <td><strong>Pauli-Z</strong></td>
                <td><code>q.z(i)</code></td>
                <td>[[1, 0], [0, -1]]</td>
                <td>Inversão de fase (|1⟩ → -|1⟩).</td>
            </tr>
            <tr>
                <td><strong>Porta S</strong></td>
                <td><code>q.s(i)</code></td>
                <td>[[1, 0], [0, i]]</td>
                <td>Porta de fase π/2 (raiz quadrada de Z).</td>
            </tr>
            <tr>
                <td><strong>Porta T</strong></td>
                <td><code>q.t(i)</code></td>
                <td>[[1, 0], [0, e^(iπ/4)]]</td>
                <td>Porta de fase π/4 (essencial para universalidade).</td>
            </tr>
            <tr>
                <td><strong>Rotação X</strong></td>
                <td><code>q.rx(i, θ)</code></td>
                <td>exp(-i θ/2 X)</td>
                <td>Rotação contínua em torno do eixo X na Esfera de Bloch.</td>
            </tr>
            <tr>
                <td><strong>Rotação Y</strong></td>
                <td><code>q.ry(i, θ)</code></td>
                <td>exp(-i θ/2 Y)</td>
                <td>Rotação em torno do eixo Y (coeficientes reais).</td>
            </tr>
            <tr>
                <td><strong>Rotação Z</strong></td>
                <td><code>q.rz(i, θ)</code></td>
                <td>exp(-i θ/2 Z)</td>
                <td>Rotação de fase em torno do eixo Z.</td>
            </tr>
            <tr>
                <td><strong>CNOT / CX</strong></td>
                <td><code>q.cx(c, t)</code></td>
                <td>Matriz 4x4 controlada</td>
                <td>Aplica X no alvo se o controle estiver em |1⟩.</td>
            </tr>
            <tr>
                <td><strong>CZ</strong></td>
                <td><code>q.cz(c, t)</code></td>
                <td>diag(1, 1, 1, -1)</td>
                <td>Fase condicional simétrica de 2 qubits.</td>
            </tr>
            <tr>
                <td><strong>SWAP</strong></td>
                <td><code>q.swap(a, b)</code></td>
                <td>Troca de estados</td>
                <td>Permuta amplitudes dos qubits a e b.</td>
            </tr>
            <tr>
                <td><strong>Toffoli (CCX)</strong></td>
                <td><code>q.ccx(c1, c2, t)</code></td>
                <td>Matriz 8x8 controlada</td>
                <td>Porta AND clássica reversível de 3 qubits.</td>
            </tr>
            <tr>
                <td><strong>Fredkin (CSWAP)</strong></td>
                <td><code>q.cswap(c, a, b)</code></td>
                <td>Troca controlada</td>
                <td>Troca a e b se o controle estiver em |1⟩.</td>
            </tr>
        </tbody>
    </table>
</div>

<!-- APÊNDICE C: CHEAT SHEET RÁPIDO -->
<div class="chapter">
    <span class="chapter-badge">Apêndice C</span>
    <h1 class="chapter-title">Cheat Sheet de Sintaxe Rápida e Comandos da CLI</h1>
    <div class="chapter-intro">
        Folha de consulta rápida para programadores experientes.
    </div>

    <div class="code-block">
        <div class="code-header"><span>Sintaxe Expressa GhostLang</span><span>Cheat Sheet</span></div>
        <pre>
<span class="cm">// Variáveis e Tipos</span>
<span class="type">int</span> a = <span class="num">10</span>;  <span class="type">float</span> b = <span class="num">3.14</span>;  <span class="type">string</span> s = <span class="str">f"valor={a}"</span>;  <span class="type">complex</span> z = <span class="num">1.0</span> + <span class="num">2.0i</span>;
<span class="type">ghost</span> g = ghost(<span class="num">1.0</span>, <span class="num">0.5</span>, <span class="num">2.0</span>, <span class="num">0.0</span>); <span class="cm">// Álgebra G 4D</span>

<span class="cm">// Entrada e Saída Interativa</span>
<span class="fn">println</span>(<span class="str">f"Saída com f-string: {s}"</span>);
<span class="type">string</span> nome = <span class="fn">input</span>(<span class="str">"Digite o nome: "</span>);
<span class="type">int</span> idade; <span class="fn">inputln</span>(<span class="str">f"Digite a idade: {idade}"</span>);

<span class="cm">// Condicionais e Laços</span>
<span class="kw">if</span> (a &gt; <span class="num">0</span>) { ... } <span class="kw">else</span> { ... }
<span class="kw">switch</span> (s) { <span class="kw">case</span> <span class="str">"X"</span>: ... <span class="kw">break</span>; <span class="kw">default</span>: ... }
<span class="kw">foreach</span> (<span class="type">string</span> item <span class="kw">in</span> lista) { ... }

<span class="cm">// Computação Quântica</span>
<span class="type">qreg</span> q(<span class="num">2</span>); q.h(<span class="num">0</span>); q.cx(<span class="num">0</span>, <span class="num">1</span>); q.measure_all();
<span class="kw">circuit</span> bell(a, b) { h(a); cx(a, b); }

<span class="cm">// Deep Learning</span>
<span class="kw">network</span> modelo = W1 &gt;&gt; W2;
modelo.optimize(X, Y, <span class="num">100</span>, <span class="num">0.01</span>, <span class="str">"adam"</span>);
<span class="kw">var</span> clf = qml_classifier(<span class="num">2</span>, <span class="num">3</span>);

<span class="cm">// Comandos CLI</span>
ghost build main.g           <span class="cm">// Compila para C++17</span>
ghost run main.g             <span class="cm">// Executa direto</span>
ghost test                   <span class="cm">// Roda testes CTest</span>
ghost fmt -w src/            <span class="cm">// Formata código</span>
        </pre>
    </div>
</div>

<!-- CONTRACAPA DO LIVRO -->
<div class="cover-page" style="background: linear-gradient(135deg, #1e1b4b 0%, #0f172a 50%, #090d16 100%);">
    <div>
        <div class="cover-badge-container">
            <span class="cover-badge">GhostLang 2026</span>
            <span class="cover-badge gold">Referência Definitiva</span>
        </div>
        <h2 style="font-size: 26pt; font-weight: 800; color: #f8fafc; margin-bottom: 20px;">
            A Linguagem da Próxima Era da Computação
        </h2>
        <p style="font-size: 11pt; color: #cbd5e1; line-height: 1.7; max-width: 600px; margin-bottom: 25px;">
            O <strong>GhostLang</strong> redefine os limites do desenvolvimento de software ao unificar a performance implacável do <strong>C++17 ISO</strong>, a abstração declarativa da <strong>Inteligência Artificial moderna</strong> e o modelo de estados probabilísticos da <strong>Computação Quântica</strong> em uma única ferramenta coesa, elegante e autônoma.
        </p>

        <p style="font-size: 10.5pt; color: #94a3b8; line-height: 1.6; max-width: 600px;">
            Seja você um pesquisador de física quântica desenvolvendo novos algoritmos variacionais, um engenheiro de aprendizado de máquina treinando redes neurais profundas com aceleração CUDA, ou um desenvolvedor de backend construindo microsserviços ultrarrápidos, este livro é o seu mapa definitivo.
        </p>
    </div>

    <div style="border-top: 1px solid rgba(255, 255, 255, 0.15); padding-top: 20px; display: flex; justify-content: space-between; align-items: center;">
        <div>
            <div style="font-weight: bold; color: #f8fafc; font-size: 12pt;">GHOSTLANG PRESS</div>
            <div style="color: #64748b; font-size: 9pt;">Engenharia de Compiladores e Sistemas Inteligentes</div>
            <div style="color: #38bdf8; font-size: 8.5pt; margin-top: 3px;">https://github.com/eltpmoc/ghostlang</div>
        </div>
        <div style="background: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.2); padding: 8px 16px; border-radius: 6px; font-family: monospace; font-size: 9pt; color: #38bdf8;">
            ISBN: 978-65-00-12345-6
        </div>
    </div>
</div>
"""
