# -*- coding: utf-8 -*-
"""
Capítulo 7: Computação Quântica Nativa de Primeira Classe.
"""

def get_ch04_html():
    return r"""
<!-- CAPÍTULO 7 -->
<div class="chapter">
    <span class="chapter-badge">Capítulo 7</span>
    <h1 class="chapter-title">Computação Quântica Nativa de Primeira Classe</h1>
    <div class="chapter-intro">
        Mergulhe no paradigma quântico do GhostLang: registradores qreg, portas unitárias parametrizadas, sub-circuitos reutilizáveis, algoritmos de Grover e QFT, matrizes de densidade e interoperabilidade com OpenQASM 3.0.
    </div>

    <h2>7.1 O Tipo Nativo <code>qreg</code> e o Modelo Statevector</h2>
    <p>
        No GhostLang, o registrador quântico <code>qreg</code> representa um estado quântico puro de <span class="math">N</span> qubits:
    </p>

    <div style="text-align: center; font-size: 13pt; margin: 15px 0; color: #0284c7; font-weight: bold;">
        |ψ⟩ = ∑ αᵢ |i⟩, &nbsp;&nbsp; com ∑ |αᵢ|² = 1.0, &nbsp;&nbsp; αᵢ ∈ ℂ
    </div>

    <p>
        A simulação statevector opera diretamente em memória contígua em C++ com precisão dupla. A partir de 13 qubits, a aceleração OpenMP paraleliza automaticamente os laços de produto tensorial, permitindo simular com velocidade até 28 qubits (268 milhões de amplitudes complexas).
    </p>

    <div class="callout callout-quantum">
        <div class="callout-title">🔬 Convenção Fundamental de Kets no GhostLang</div>
        O qubit 0 é sempre o <strong>bit menos significativo</strong> (à direita). Um estado com <span class="math">q₀ = 1</span> e <span class="math">q₁ = 0</span> é escrito como <span class="math">|01⟩</span>. Da mesma forma, em strings de observáveis de Pauli como <code>"XZ"</code>, o operador <span class="math">X</span> atua sobre o qubit 1 e o operador <span class="math">Z</span> sobre o qubit 0.
    </div>

    <h2>7.2 O Circuito de Bell: Entrelaçamento Quântico Básico</h2>
    <p>
        Vejamos como gerar o par de Bell <span class="math">|Φ⁺⟩ = (1/√2)(|00⟩ + |11⟩)</span>:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/bell.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/quantum.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">qreg</span> q(<span class="num">2</span>);        <span class="cm">// Inicializa |00></span>
    q.seed(<span class="num">42</span>);       <span class="cm">// Semente pseudo-aleatória reprodutível</span>

    q.h(<span class="num">0</span>);           <span class="cm">// Cria superposição: (|00> + |01>) / sqrt(2)</span>
    q.cx(<span class="num">0</span>, <span class="num">1</span>);       <span class="cm">// CNOT: entrelaça q0 e q1</span>

    <span class="fn">println</span>(<span class="str">"Estado do par de Bell gerado:"</span>);
    q.show();         <span class="cm">// Saída: 0.7071|00> + 0.7071|11></span>

    <span class="fn">println</span>(<span class="str">"Desenho do Circuito em ASCII:"</span>);
    <span class="fn">println</span>(q.draw());

    <span class="fn">println</span>(<span class="str">"Valor esperado de correlação &lt;ZZ&gt;:"</span>);
    <span class="fn">println</span>(q.expect(<span class="str">"ZZ"</span>)); <span class="cm">// Retorna exatamente 1.0 (correlação perfeita)</span>

    <span class="cm">// Medição e colapso de 1000 repetições (shots)</span>
    <span class="fn">println</span>(<span class="str">"Histograma de 1000 disparos:"</span>);
    <span class="fn">println</span>(q.histogram(<span class="num">1000</span>));
}
        </pre>
    </div>

    <h2>7.3 Sub-circuitos Declarativos com a Sintaxe <code>circuit { }</code></h2>
    <p>
        O GhostLang oferece uma sintaxe declarativa própria para composição modular de blocos quânticos. No corpo de um <code>circuit</code>, as portas são invocadas diretamente sem prefixo de objeto:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/circuit_ghz.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="cm">// Sub-circuito para criar estado de Greenberger-Horne-Zeilinger (GHZ) em N qubits</span>
<span class="kw">circuit</span> <span class="fn">estado_ghz</span>(<span class="type">int</span> n) {
    h(<span class="num">0</span>);
    <span class="kw">for</span> (<span class="type">int</span> i = <span class="num">0</span>; i &lt; n - <span class="num">1</span>; i = i + <span class="num">1</span>) {
        cx(i, i + <span class="num">1</span>);
    }
}

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">qreg</span> q(<span class="num">5</span>);
    q.estado_ghz(<span class="num">5</span>); <span class="cm">// Aplica o circuito no registrador q</span>
    
    <span class="fn">println</span>(<span class="str">"Estado GHZ de 5 Qubits (|00000> + |11111>) / sqrt(2):"</span>);
    q.show();
}
        </pre>
    </div>

    <h2>7.4 Algoritmo de Busca Quântica de Grover</h2>
    <p>
        O GhostLang possui suporte nativo para primitivas de oráculo quântico (<code>phase_flip</code>) e o operador de difusão de Grover (<code>diffuser()</code>), permitindo encontrar um elemento em uma base desordenada de <span class="math">N = 2ⁿ</span> estados com complexidade <span class="math">𝒪(√N)</span>:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/grover.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/quantum.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">int</span> n = <span class="num">3</span>;               <span class="cm">// 3 qubits -> 8 estados possíveis (0 a 7)</span>
    <span class="type">int</span> alvo_secreto = <span class="num">6</span>;    <span class="cm">// Queremos localizar o número 6 (|110>)</span>

    <span class="type">qreg</span> q(n);
    q.h_all();               <span class="cm">// Superposição uniforme sobre os 8 estados</span>

    <span class="cm">// Número ótimo de iterações: aproximadamente (pi / 4) * sqrt(8) ≈ 2</span>
    <span class="kw">for</span> (<span class="type">int</span> passo = <span class="num">1</span>; passo &lt;= <span class="num">2</span>; passo = passo + <span class="num">1</span>) {
        q.phase_flip(alvo_secreto); <span class="cm">// Oráculo: inverte a fase do estado procurado</span>
        q.diffuser();              <span class="cm">// Difusor de Grover: amplifica a amplitude em torno da média</span>
    }

    <span class="fn">println</span>(<span class="str">f"Probabilidade de medir o alvo {alvo_secreto}: {q.prob_state(alvo_secreto) * 100}%"</span>);
    <span class="fn">println</span>(<span class="str">"Histograma de 500 medições:"</span>);
    <span class="fn">println</span>(q.histogram(<span class="num">500</span>));
}
        </pre>
    </div>

    <h2>7.5 Matrizes de Densidade e Exportação OpenQASM 3.0</h2>
    <p>
        Para modelar sistemas abertos com ruído ou entrelaçamento parcial, usamos <code>GhostDensityMatrix</code>. Além disso, circuitos podem ser exportados diretamente para execução em hardware real (IBM Quantum, IonQ, Rigetti) usando OpenQASM 3.0:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/qasm_export.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">qreg</span> q(<span class="num">2</span>);
    q.h(<span class="num">0</span>);
    q.rz(<span class="num">0</span>, <span class="num">1.570796</span>); <span class="cm">// Rz(pi/2)</span>
    q.cx(<span class="num">0</span>, <span class="num">1</span>);

    <span class="type">string</span> qasm_code = q.to_qasm();
    <span class="fn">println</span>(<span class="str">"Código OpenQASM 3.0 gerado:"</span>);
    <span class="fn">println</span>(qasm_code);
}
        </pre>
    </div>

    <!-- SEÇÃO DE ATIVIDADES PRÁTICAS DO CAPÍTULO -->
    <div class="exercises-section">
        <div class="exercises-title">
            <span>✏️</span> Laboratório Prático: Exercícios do Capítulo 7
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 4.1: Gerador de Números Aleatórios Quânticos (QRNG)</span>
                <span class="exercise-badge badge-easy">Aquecimento</span>
            </div>
            <div class="exercise-desc">
                Escreva um programa <code>qrng.g</code> que aloque um registrador com 8 qubits, aplique portas Hadamard em todos eles com <code>h_all()</code> e execute <code>measure_all()</code> para produzir um byte aleatório puro (0 a 255) baseado no princípio de incerteza da mecânica quântica.
            </div>
            <div class="exercise-hint">Dica: Converta o mapa de medição ou os bits medidos para um inteiro clássico.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 4.2: Protocolo de Teletransporte Quântico Completo</span>
                <span class="exercise-badge badge-medium">Aplicação</span>
            </div>
            <div class="exercise-desc">
                Implemente o protocolo de Teletransporte de Bennett et al. (1993) em 3 qubits: o qubit 0 armazena o estado a ser teleportado (aplique uma rotação <span class="math">R<sub>y</sub>(0, π/3)</span> nele), os qubits 1 e 2 formam o par entrelaçado. Alice mede seus dois qubits e Bob aplica as correções <span class="math">X</span> e <span class="math">Z</span> necessárias. Ao final, confira se o estado do qubit 2 é idêntico ao estado inicial.
            </div>
            <div class="exercise-hint">Dica: Use medições condicionais com <code>measure(0)</code> e <code>measure(1)</code>.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 4.3: Transformada Quântica de Fourier (QFT) de 4 Qubits</span>
                <span class="exercise-badge badge-hard">Desafio</span>
            </div>
            <div class="exercise-desc">
                Construa manualmente a decomposição da Transformada Quântica de Fourier para 4 qubits utilizando portas Hadamard e portas de fase controladas <span class="math">CP(c, t, φ)</span> com ângulos <span class="math">π/2</span>, <span class="math">π/4</span> e <span class="math">π/8</span>, finalizando com as trocas SWAP. Compare os resultados de amplitude com a chamada nativa <code>q.qft()</code>.
            </div>
            <div class="exercise-hint">Dica: Lembre-se de aplicar a porta SWAP entre os qubits extremos ao final.</div>
        </div>
    </div>
</div>
"""
