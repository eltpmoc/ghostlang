# -*- coding: utf-8 -*-
"""
Capítulo 8: Inteligência Artificial, Redes Neurais e Autograd Reverso.
"""

def get_ch05_html():
    return r"""
<!-- CAPÍTULO 8 -->
<div class="chapter">
    <span class="chapter-badge">Capítulo 8</span>
    <h1 class="chapter-title">Inteligência Artificial, Redes Neurais e Autograd Reverso</h1>
    <div class="chapter-intro">
        Construa, treine e coloque em produção redes neurais profundas com a sintaxe declarativa do operador pipeline '>>', diferenciação automática dinâmica e inferência local de modelos de linguagem GGUF.
    </div>

    <h2>8.1 O Operador Pipeline de Camadas Neurais: <code>>></code></h2>
    <p>
        No GhostLang, redes neurais multicamadas (Perceptrons Multicamadas e além) são construídas com uma elegância sintática incomparável usando o operador <code>>></code>:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/rede_neural.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// Camadas de pesos: W1 (entrada 4 -> oculta 8), W2 (oculta 8 -> saída 1)</span>
    <span class="type">matrix</span> W1 = matrix_random(<span class="num">4</span>, <span class="num">8</span>);
    <span class="type">matrix</span> W2 = matrix_random(<span class="num">8</span>, <span class="num">1</span>);

    <span class="cm">// Encadeamento de camadas: ativação ReLU implícita entre estágios</span>
    <span class="kw">network</span> modelo = W1 &gt;&gt; W2;

    <span class="cm">// Entrada: lote de 1 amostra com 4 features</span>
    <span class="type">matrix</span> X(<span class="num">1</span>, <span class="num">4</span>);
    X.set(<span class="num">0</span>, <span class="num">0</span>, <span class="num">0.5</span>); X.set(<span class="num">0</span>, <span class="num">1</span>, <span class="num">1.2</span>); X.set(<span class="num">0</span>, <span class="num">2</span>, <span class="num">-0.3</span>); X.set(<span class="num">0</span>, <span class="num">3</span>, <span class="num">0.8</span>);

    <span class="cm">// Inferência (Forward pass)</span>
    <span class="type">matrix</span> saida = modelo.forward(X);
    <span class="fn">println</span>(<span class="str">"Predição inicial da rede neural:"</span>);
    saida.show();
}
        </pre>
    </div>

    <h2>8.2 Funções de Ativação Vetorizadas e Otimizadores</h2>
    <p>
        O GhostLang inclui uma biblioteca matemática com as funções de ativação mais utilizadas em Deep Learning moderno, com execução vetorizada e aceleração OpenMP:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/ativacoes_otimizadores.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">matrix</span> logits = matrix_random(<span class="num">1</span>, <span class="num">5</span>);
    
    <span class="cm">// Ativações com derivadas analíticas integradas</span>
    <span class="type">matrix</span> probs = activate(logits, <span class="str">"softmax"</span>);
    <span class="type">matrix</span> ativ_gelu = activate(logits, <span class="str">"gelu"</span>);

    <span class="fn">println</span>(<span class="str">"Probabilidades normalizadas (Softmax):"</span>);
    probs.show();

    <span class="cm">// Treinamento com Otimizador Adam: W => optimize(X, Y, epochs, lr, "adam")</span>
    <span class="type">matrix</span> X_treino = matrix_random(<span class="num">20</span>, <span class="num">4</span>);
    <span class="type">matrix</span> Y_alvo = matrix_random(<span class="num">20</span>, <span class="num">1</span>);

    <span class="kw">network</span> rede = W1 &gt;&gt; W2;
    rede.optimize(X_treino, Y_alvo, <span class="num">100</span>, <span class="num">0.01</span>, <span class="str">"adam"</span>);
    <span class="fn">println</span>(<span class="str">"Treinamento concluído com Adam!"</span>);
}
        </pre>
    </div>

    <h2>8.3 Diferenciação Automática Dinâmica: Autograd Reverso</h2>
    <p>
        O módulo <code>libs/autograd.g</code> implementa um motor de Reverse-Mode Automatic Differentiation de alta performance:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/autograd.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/autograd.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// Variáveis escalares com rastreamento de gradiente</span>
    <span class="type">var</span> x = var_tensor(<span class="num">3.0</span>, <span class="kw">verdadeiro</span>); <span class="cm">// requer_grad = true</span>
    <span class="type">var</span> y = var_tensor(<span class="num">4.0</span>, <span class="kw">verdadeiro</span>);

    <span class="cm">// Função de perda: f(x, y) = x^2 * y + 3*y</span>
    <span class="type">var</span> z = (x * x * y) + (<span class="num">3.0</span> * y);

    <span class="cm">// Retropropagação exata de gradientes</span>
    z.backward();

    <span class="fn">println</span>(<span class="str">f"Valor de f(x, y): {z.value()}"</span>);
    <span class="fn">println</span>(<span class="str">f"df/dx (Esperado: 2*x*y = 24.0): {x.grad()}"</span>);
    <span class="fn">println</span>(<span class="str">f"df/dy (Esperado: x^2 + 3 = 12.0): {y.grad()}"</span>);
}
        </pre>
    </div>

    <h2>8.4 Transformers Nativos e Inferência Local com Modelos GGUF</h2>
    <p>
        O GhostLang inclui o <code>GhostTransformer</code> nativo com auto-atenção causal multi-cabeça e suporte direto a modelos de linguagem no padrão GGUF da indústria (como Llama 3.2 3B):
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/llm_inference.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="fn">println</span>(<span class="str">"Carregando LLM local GGUF..."</span>);
    <span class="kw">var</span> modelo = load_gguf(<span class="str">"modelos/llama-3.2-3b-q4.gguf"</span>);

    <span class="type">string</span> prompt = <span class="str">"Explique em duas frases a vantagem da computação quântica:"</span>;
    <span class="type">string</span> resposta = modelo.generate(prompt, <span class="num">0.7</span>, <span class="num">120</span>); <span class="cm">// temp=0.7, max_tokens=120</span>

    <span class="fn">println</span>(<span class="str">"Resposta da IA:"</span>);
    <span class="fn">println</span>(resposta);
}
        </pre>
    </div>

    <!-- SEÇÃO DE ATIVIDADES PRÁTICAS DO CAPÍTULO -->
    <div class="exercises-section">
        <div class="exercises-title">
            <span>✏️</span> Laboratório Prático: Exercícios do Capítulo 8
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 5.1: Classificador de Porta XOR com Rede Neural</span>
                <span class="exercise-badge badge-easy">Aquecimento</span>
            </div>
            <div class="exercise-desc">
                Crie uma rede neural clássica com arquitetura 2 → 4 → 1 usando o operador <code>>></code>. Treine a rede sobre os 4 padrões da porta lógica XOR com o otimizador Adam durante 200 épocas e demonstre que a acurácia final atinge 100%.
            </div>
            <div class="exercise-hint">Dica: Utilize uma matriz de entrada 4 × 2 e uma matriz alvo 4 × 1.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 5.2: Otimização de Regressão com Autograd Dinâmico</span>
                <span class="exercise-badge badge-medium">Aplicação</span>
            </div>
            <div class="exercise-desc">
                Implemente uma regressão linear univariada <span class="math">y = w · x + b</span> utilizando variáveis de grafo computacional <code>var_tensor</code>. Ajuste os parâmetros <span class="math">w</span> e <span class="math">b</span> por gradiente descendente durante 50 passos sobre uma nuvem de pontos sintética, minimizando o erro quadrático médio (MSE).
            </div>
            <div class="exercise-hint">Dica: Atualize os parâmetros subtraindo <span class="math">η · grad()</span> a cada iteração.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 5.3: Convolução 2D e MaxPooling para Detecção de Bordas</span>
                <span class="exercise-badge badge-hard">Desafio</span>
            </div>
            <div class="exercise-desc">
                Defina uma matriz 8 × 8 simulando uma imagem em escala de cinza com um degrau de contraste vertical. Aplique uma convolução 2D com o filtro de Sobel vertical 3 × 3 (com pesos <code>[[-1, 0, 1], [-2, 0, 2], [-1, 0, 1]]</code>) e, em seguida, aplique uma camada de <code>max_pool2d</code> com janela 2 × 2 e passo 2. Exiba o mapa de ativação resultante.
            </div>
            <div class="exercise-hint">Dica: Utilize <code>conv2d(imagem, kernel)</code> e <code>max_pool2d(mapa, 2, 2)</code>.</div>
        </div>
    </div>
</div>
"""
