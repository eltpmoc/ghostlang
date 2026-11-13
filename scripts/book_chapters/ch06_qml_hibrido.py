# -*- coding: utf-8 -*-
"""
Capítulo 9: Machine Learning Quântico (QML), VQE, QAOA e TransformerG.
"""

def get_ch06_html():
    return r"""
<!-- CAPÍTULO 9 -->
<div class="chapter">
    <span class="chapter-badge">Capítulo 9</span>
    <h1 class="chapter-title">Machine Learning Quântico (QML) e Redes Híbridas</h1>
    <div class="chapter-intro">
        O pináculo da inovação no GhostLang: camadas neurais quânticas intercaladas em redes clássicas, derivadas exatas via Parameter-Shift, VQE químico, QAOA e o TransformerG quântico.
    </div>

    <h2>9.1 A Regra de Parameter-Shift: Derivadas Exatas de Circuitos</h2>
    <p>
        Diferente do cálculo numérico comum por diferenças finitas que sofre com instabilidades numéricas, circuitos quânticos variacionais admitem gradientes analíticos exatos através da <strong>Regra de Parameter-Shift</strong>:
    </p>

    <div style="text-align: center; font-size: 13pt; margin: 15px 0; color: #0284c7; font-weight: bold;">
        ∂⟨O⟩ / ∂θ = ( ⟨O⟩_(θ + π/2) - ⟨O⟩_(θ - π/2) ) / 2
    </div>

    <p>
        O GhostLang calcula esses deslocamentos de fase diretamente no runtime C++ em <code>src/GhostQML.cpp</code> com precisão superior a 10⁻⁶, permitindo treinar portas unitárias parametrizadas (<span class="math">R<sub>x</sub>, R<sub>y</sub>, R<sub>z</sub></span>) com o otimizador Adam ou SGD com a mesma fluidez de uma rede neural clássica.
    </p>

    <h2>9.2 Redes Híbridas com <code>GhostQLayer</code>: <code>W1 >> Q >> W2</code></h2>
    <p>
        Uma das funcionalidades mais poderosas do GhostLang é a capacidade de inserir uma <strong>camada de processamento quântico</strong> diretamente entre camadas densas clássicas:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/rede_hibrida.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/quantum.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// Camada 1: Densa clássica (2 features -> 4 nós de projeção)</span>
    <span class="type">matrix</span> W1 = matrix_random(<span class="num">2</span>, <span class="num">4</span>);

    <span class="cm">// Camada 2: Quântica com 4 qubits e 2 camadas de entrelaçamento ansatz</span>
    <span class="type">qlayer</span> Q(<span class="num">4</span>, <span class="num">2</span>);

    <span class="cm">// Camada 3: Densa clássica (4 medições <Z> -> 1 saída de probabilidade)</span>
    <span class="type">matrix</span> W2 = matrix_random(<span class="num">4</span>, <span class="num">1</span>);

    <span class="cm">// Encadeamento Híbrido Completo</span>
    <span class="kw">network</span> rede_hibrida = W1 &gt;&gt; Q &gt;&gt; W2;

    <span class="cm">// Treinamento ponta a ponta com Autograd Híbrido Unificado</span>
    <span class="type">matrix</span> X_circulo = carregar_dataset_circulo();
    <span class="type">matrix</span> Y_circulo = carregar_rotulos();

    <span class="fn">println</span>(<span class="str">"Treinando Rede Híbrida Clássico-Quântica..."</span>);
    rede_hibrida.optimize(X_circulo, Y_circulo, <span class="num">60</span>, <span class="num">0.02</span>, <span class="str">"adam"</span>);
    <span class="fn">println</span>(<span class="str">"Convergência atingida: 100% no problema não-linear do círculo!"</span>);
}
        </pre>
    </div>

    <h2>9.3 Classificador Quântico Supervisionado Nativo</h2>
    <p>
        Para fluxos de Machine Learning Quântico direto, a classe <code>GhostQML</code> oferece métodos de alto nível para inserção de dados, treinamento em lote paralelizado com OpenMP e persistência em disco:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/classificador_qml.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// Classificador com 2 qubits e 3 camadas de rotações treináveis</span>
    <span class="kw">var</span> clf = qml_classifier(<span class="num">2</span>, <span class="num">3</span>);

    <span class="cm">// Alimentação de amostras de treino</span>
    clf.add_sample(<span class="num">0</span>, <span class="num">0.1</span>, <span class="num">0.2</span>);
    clf.add_sample(<span class="num">0</span>, <span class="num">0.2</span>, <span class="num">0.3</span>);
    clf.add_sample(<span class="num">1</span>, <span class="num">0.8</span>, <span class="num">0.9</span>);
    clf.add_sample(<span class="num">1</span>, <span class="num">0.9</span>, <span class="num">0.7</span>);

    clf.fit(<span class="num">50</span>, <span class="num">0.05</span>); <span class="cm">// 50 épocas com taxa 0.05</span>

    <span class="fn">println</span>(<span class="str">f"Acurácia final de treino: {clf.accuracy() * 100}%"</span>);
    <span class="type">int</span> classe_predita = clf.predict(<span class="num">0.85</span>, <span class="num">0.88</span>);
    <span class="fn">println</span>(<span class="str">f"Predição para novo ponto (0.85, 0.88): Classe {classe_predita}"</span>);
}
        </pre>
    </div>

    <h2>9.4 Algoritmos Variacionais: VQE Molecular e QAOA Combinatório</h2>
    <p>
        O GhostLang resolve problemas de química quântica (VQE) e otimização combinatória (QAOA) com bibliotecas nativas:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/vqe_qaoa.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/quantum.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// 1. VQE para a molécula de H2 com erro < 3e-6 Hartree</span>
    <span class="kw">var</span> vqe = qml_vqe(<span class="str">"hamiltonianos/h2.json"</span>);
    vqe.minimize(<span class="num">100</span>, <span class="num">0.01</span>);
    <span class="fn">println</span>(<span class="str">f"Energia do estado fundamental do H2: {vqe.ground_energy()} Ha"</span>);

    <span class="cm">// 2. QAOA p=3 para MaxCut em grafos</span>
    <span class="kw">var</span> grafo = qaoa_graph();
    grafo.add_edge(<span class="num">0</span>, <span class="num">1</span>); grafo.add_edge(<span class="num">1</span>, <span class="num">2</span>); grafo.add_edge(<span class="num">2</span>, <span class="num">0</span>);
    <span class="kw">var</span> solucao = grafo.solve_maxcut(<span class="num">3</span>); <span class="cm">// 3 camadas p</span>
    <span class="fn">println</span>(<span class="str">f"Corte máximo encontrado: {solucao.cut_value}"</span>);
}
        </pre>
    </div>

    <h2>9.5 O TransformerG: Atenção Quântica com <code>GhostQubit</code></h2>
    <p>
        A joia da coroa da inteligência artificial no GhostLang é o <strong>TransformerG</strong>: os pesos da rede não são simples números reais, mas números complexos e amplitudes quânticas (<code>GhostQubit</code>). As cabeças de atenção realizam o produto escalar complexo e preservam a incerteza de fase (<span class="math">θ²</span>) durante todo o treinamento, permitindo capturar dependências semânticas e relações de interferência que modelos clássicos ignoram.
    </p>

    <!-- SEÇÃO DE ATIVIDADES PRÁTICAS DO CAPÍTULO -->
    <div class="exercises-section">
        <div class="exercises-title">
            <span>✏️</span> Laboratório Prático: Exercícios do Capítulo 9
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 6.1: Verificador da Regra de Parameter-Shift</span>
                <span class="exercise-badge badge-easy">Aquecimento</span>
            </div>
            <div class="exercise-desc">
                Crie um circuito simples de 1 qubit com uma porta de rotação <span class="math">R<sub>y</sub>(θ)</span> seguida pela medição do observável <span class="math">Z</span>, onde o valor esperado teórico é <span class="math">⟨Z⟩ = cos(θ)</span>. Calcule o gradiente de <span class="math">⟨Z⟩</span> em relação a <span class="math">θ = π/4</span> usando a regra de Parameter-Shift e compare com a derivada analítica <span class="math">-sin(π/4)</span>.
            </div>
            <div class="exercise-hint">Dica: Calcule <span class="math">⟨Z⟩</span> em <span class="math">θ + π/2</span> e <span class="math">θ - π/2</span> e divida a diferença por 2.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 6.2: Kernel Quântico ZZ para Separação de Dados</span>
                <span class="exercise-badge badge-medium">Aplicação</span>
            </div>
            <div class="exercise-desc">
                Instancie o feature map quântico ZZ para dois pontos bidimensionais <span class="math">x</span> = [0.2, 0.4] e <span class="math">y</span> = [0.8, 0.9]. Calcule o produto interno em espaço de Hilbert <span class="math">|⟨ϕ(x) | ϕ(y)⟩|²</span> e mostre que pontos próximos resultam em fidelidade próxima a 1.0 e pontos distantes têm fidelidade próxima a 0.0.
            </div>
            <div class="exercise-hint">Dica: Utilize a função <code>quantum_kernel_zz(x, y)</code> de <code>include/GhostQML.hpp</code>.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 6.3: Otimizador Multiverso Quantum-Inspired</span>
                <span class="exercise-badge badge-hard">Desafio</span>
            </div>
            <div class="exercise-desc">
                Implemente uma rotina que explore simultaneamente 4 universos de hiperparâmetros com o comando <code>hypothesis</code>. Em cada universo, treine uma rede neural híbrida variando a taxa de aprendizado e colapse ao final para o universo com o menor erro utilizando <code>collapse_min()</code>.
            </div>
            <div class="exercise-hint">Dica: Avalie a função de custo ao final de cada ramo do bloco hypothesis.</div>
        </div>
    </div>
</div>
"""
