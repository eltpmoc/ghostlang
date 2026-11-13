# -*- coding: utf-8 -*-
"""
Capítulo 6: Álgebra Hiper-Complexa G (4D) e Tensores N-Dimensionais.
"""

def get_ch03_html():
    return r"""
<!-- CAPÍTULO 6 -->
<div class="chapter">
    <span class="chapter-badge">Capítulo 6</span>
    <h1 class="chapter-title">Álgebra Hiper-Complexa G (4D) e Tensores N-Dimensionais</h1>
    <div class="chapter-intro">
        Aprofunde-se na assinatura matemática única do GhostLang: os Números Fantasmas 4D, comutadores quânticos, propagação analítica de incerteza, tensores multidimensionais e aceleração via GhostBLAS.
    </div>

    <h2>6.1 Os Números Fantasmas G: Definição e Estrutura 4D</h2>
    <p>
        O GhostLang introduz como primitivo matemático o número hiper-complexo <strong>Fantasma G</strong>, modelado em quatro coordenadas ortogonais:
    </p>

    <div style="text-align: center; font-size: 14pt; margin: 15px 0; color: #0284c7; font-weight: bold;">
        G = x + y·i + p·g + θ²
    </div>

    <p>
        Onde:
    </p>
    <ul>
        <li><strong>x (Parte Real):</strong> Representa grandezas físicas clássicas e escalares.</li>
        <li><strong>y (Parte Imaginária i):</strong> Comportamento oscilatório e amplitudes de fase (<span class="math">i² = -1</span>).</li>
        <li><strong>p (Parte Fantasma g):</strong> Componente diferencial de incerteza quântica ou gradiente propagado.</li>
        <li><strong>θ (Incerteza Grassmanniana / Dual):</strong> Elemento nilpotente onde <span class="math">θ² = 0</span>, utilizado em derivadas automáticas e aproximações de perturbação.</li>
    </ul>

    <div class="callout callout-warning">
        <div class="callout-title">⚠️ Nota Matemática Rigorosa sobre a Álgebra G</div>
        Conforme verificado em <code>src/GhostNumberG.cpp</code>, o produto dos números <span class="math"><b>G</b></span> não é estritamente associativo em todas as combinações: para o fantasma puro <span class="math">θ = (0, 0, 1)</span>, temos <span class="math">(i · i) · θ = -θ</span>, enquanto <span class="math">i · (i · θ) = 0</span>. Esta propriedade é intencional e reflete a física de comutadores e anti-comutadores de campos fermiônicos e estados entrelaçados.
    </div>

    <h2>6.2 Criando e Manipulando Números G no Código</h2>
    <p>
        No GhostLang, o tipo <code>ghost</code> é de primeira classe e pode ser criado com literais (como <code>1.0 + 2.0i + 3.0g</code>) ou diretamente através do construtor explícito <code>ghost(x, y, p, theta2)</code>:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/NumerosFantasmasG.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// Instanciação explícita de número G: x, y (i), p (g), theta2 (θ²)</span>
    <span class="type">ghost</span> g1 = ghost(<span class="num">3.0</span>, <span class="num">4.0</span>, <span class="num">1.7</span>, <span class="num">2.0</span>);
    <span class="type">ghost</span> g2 = ghost(<span class="num">1.0</span>, <span class="num">2.0</span>, <span class="num">3.0</span>, <span class="num">4.0</span>);

    <span class="fn">println</span>(<span class="str">f"Número fantasma g1: {g1}"</span>);
    <span class="fn">println</span>(<span class="str">f"Número fantasma g2: {g2}"</span>);

    <span class="cm">// Operações algébricas nativas</span>
    <span class="type">ghost</span> soma = g1 + g2;
    <span class="type">ghost</span> produto = g1 * g2;
    <span class="fn">println</span>(<span class="str">f"Soma dos números fantasmas G: {soma}"</span>);
    <span class="fn">println</span>(<span class="str">f"Produto dos números fantasmas G: {produto}"</span>);

    <span class="cm">// Formatos de Exibição Personalizados</span>
    <span class="fn">println</span>(<span class="str">f"g1 em 4D: {g1:4d}"</span>);
    <span class="fn">println</span>(<span class="str">"g1 em 4D com to_4d(): "</span> + g1.<span class="fn">to_4d</span>());
    <span class="fn">println</span>(<span class="str">f"g1 em Matemático: {g1:math}"</span>);
    <span class="fn">println</span>(<span class="str">"g1 em Matemático com to_math(): "</span> + g1.<span class="fn">to_math</span>());
    <span class="fn">println</span>(<span class="str">f"Norma ao quadrado real: {g1.real_norm_squared()}"</span>);
}
        </pre>
    </div>

    <h3>6.2.1 Formatos de Saída em <code>print</code> e <code>println</code></h3>
    <p>
        O GhostLang oferece suporte nativo e flexível para exibir números fantasmas tanto na forma vetorial 4D quanto na forma algébrica matemática clássica:
    </p>
    <ul>
        <li><strong>Formato Padrão:</strong> <code>(x, yi, p, θ²) = (3.0000, 4.0000, 1.7000, 2.0000)</code> — representação canônica rigorosa com 4 casas decimais.</li>
        <li><strong>Formato 4D Vetorial:</strong> <code>G = (3.0, 4.0, 1.7, 2.0)</code> — exibe as 4 coordenadas <code>(x, yi, p, θ²)</code>. Acessível via especificador de f-string <code>{g:4d}</code> ou pelo método <code>g.to_4d()</code>.</li>
        <li><strong>Formato Matemático Algébrico:</strong> <code>G = 3.0 + 4.0i - 1.7p + 2.0θ²</code> — expressa o número fantasma como combinação linear de suas unidades ortogonais, com respeito rigoroso aos sinais dos termos. Acessível via f-string <code>{g:math}</code> ou pelo método <code>g.to_math()</code>.</li>
    </ul>

    <h3>6.2.2 Entrada de Dados com <code>input</code> e <code>inputln</code></h3>
    <p>
        Os comandos <code>input()</code> e <code>inputln()</code> interpretam entradas do usuário para variáveis do tipo <code>ghost</code> em qualquer uma das formas aceitas, com resolução automática de formato:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/entrada_ghost.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// 1. Entrada no formato vetorial 4D: "(3.0, 4.0, 1.7, 2.0)" ou "G = (3.0, 4.0, 1.7, 2.0)"</span>
    <span class="type">ghost</span> g1 = <span class="fn">input</span>(<span class="str">"Digite o número fantasma em 4D: "</span>);

    <span class="cm">// 2. Entrada no formato algébrico: "3.0 + 4.0i - 1.7p + 2.0θ²" ou "G = 3.0 + 4.0i - 1.7p + 2.0θ²"</span>
    <span class="type">ghost</span> g2;
    <span class="fn">inputln</span>(<span class="str">f"Digite o número fantasma algébrico: {g2}"</span>);

    <span class="fn">println</span>(<span class="str">f"g1 recebido (4D): {g1:4d}"</span>);
    <span class="fn">println</span>(<span class="str">f"g2 recebido (Matemático): {g2:math}"</span>);
}
        </pre>
    </div>

    <h2>6.3 Matrizes e Álgebra Linear de Alta Performance: <code>matrix</code> e <code>GhostMatrix</code></h2>
    <p>
        O GhostLang oferece uma arquitetura matricial de alto desempenho com suporte a duas abordagens complementares e interoperáveis: o tipo conciso <code>matrix</code> para álgebra linear numérica clássica e a classe estendida <code>GhostMatrix</code> para computação hipercomplexa 4D e inteligência artificial quântica. Ambas contam com aceleração paralela multi-thread nativa via OpenMP e kernels dedicados em GPU (CUDA).
    </p>

    <h3>6.3.1 Álgebra Linear Numérica com <code>matrix</code></h3>
    <p>
        O recurso <code>matrix</code> foi projetado para operações matemáticas rigorosas, resolução de sistemas lineares, decomposições e computação científica:
    </p>
    <ul>
        <li><strong>Instanciação Direta:</strong> <code>matrix A(linhas, colunas);</code> aloca a matriz com inicialização segura de zeros.</li>
        <li><strong>Manipulação de Células:</strong> <code>A.set(i, j, valor)</code> define o elemento na posição especificada e <code>A.get(i, j)</code> recupera seu valor, com verificação estrita de limites de dimensionalidade.</li>
        <li><strong>Inversão Numérica Rigorosa (Gauss-Jordan):</strong> O método <code>A.inverse()</code> (ou seu alias <code>A.inv()</code>) calcula a matriz inversa <span class="math">A⁻¹</span> com pivotamento parcial, detectando matrizes singulares com precisão de máquina.</li>
        <li><strong>Cálculo de Determinante:</strong> <code>A.determinant()</code> (ou <code>A.det()</code>) computa <span class="math">\det(A)</span> em tempo cúbico <span class="math">O(n³)</span>.</li>
        <li><strong>Aritmética e Multiplicação:</strong> Suporte nativo aos operadores <code>A * B</code> (produto matricial acelerado por hardware), <code>A + B</code>, <code>A - B</code>, além de multiplicação e divisão por escalares (<code>A * 2.0</code>, <code>A / 2.0</code>).</li>
        <li><strong>Exibição Tabular Formatada:</strong> <code>A.show()</code> imprime a matriz com alinhamento tabular e colchetes limpos. Interpolações diretas em f-strings como <code>println(f"{A}")</code> ou <code>println(A)</code> também são aceitas diretamente.</li>
    </ul>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/matrizes.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="cm">// Álgebra Linear Clássica e Resolução de Sistemas: matrix</span>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// 1. Instanciação de uma matriz 2x2</span>
    <span class="type">matrix</span> A(<span class="num">2</span>, <span class="num">2</span>);
    A.set(<span class="num">0</span>, <span class="num">0</span>, <span class="num">4.0</span>); A.set(<span class="num">0</span>, <span class="num">1</span>, <span class="num">7.0</span>);
    A.set(<span class="num">1</span>, <span class="num">0</span>, <span class="num">2.0</span>); A.set(<span class="num">1</span>, <span class="num">1</span>, <span class="num">6.0</span>);

    <span class="cm">// 2. Inversão matricial exata via Gauss-Jordan</span>
    <span class="type">matrix</span> A_inv = A.inverse();
    <span class="type">matrix</span> I = A * A_inv; <span class="cm">// Matriz Identidade Resultante (I) = A * A_inv</span>

    <span class="fn">println</span>(<span class="str">"Matriz A original:"</span>);
    A.show();
    <span class="fn">println</span>(<span class="str">"Matriz A inversa A^-1:"</span>);
    A_inv.show();
    <span class="fn">println</span>(<span class="str">"Matriz Identidade Resultante (I) = A * A^-1:"</span>);
    I.show();

    <span class="cm">// 3. Determinante da matriz A</span>
    <span class="type">double</span> d = A.det();
    <span class="fn">println</span>(<span class="str">f"Determinante de A: {d}"</span>);
}
        </pre>
    </div>

    <h3>6.3.2 Álgebra Hipercomplexa 4D e Redes Neurais com <code>GhostMatrix</code></h3>
    <p>
        O recurso <code>GhostMatrix</code> expande o domínio matricial para os números hipercomplexos <span class="math">G = x + y\cdot i + p\cdot g + \theta^2</span>, integrando portas quânticas, funções de ativação de redes neurais e visão computacional:
    </p>
    <ul>
        <li><strong>Elementos Hipercomplexos 4D:</strong> Cada elemento da matriz armazena as quatro componentes ortogonais (real, imaginária, gradiente fantasma e incerteza Grassmanniana).</li>
        <li><strong>Transposição Matricial:</strong> O método <code>G.transpose()</code> permuta linhas e colunas preservando a assinatura quântica dos coeficientes.</li>
        <li><strong>Funções de Ativação Vetorizadas de IA:</strong> Suporte direto a ativações não lineares sobre a matriz inteira via métodos <code>G.relu()</code>, <code>G.sigmoid()</code>, <code>G.tanh()</code>, <code>G.leaky_relu()</code>, <code>G.gelu()</code> e <code>G.softmax()</code>.</li>
        <li><strong>Visão Computacional:</strong> Métodos integrados para pooling 2D (<code>G.max_pool2d(size, stride)</code> e <code>G.avg_pool2d(size, stride)</code>) e convoluções aceleradas.</li>
        <li><strong>Persistência de Dados e Checkpoints:</strong> Leitura e escrita de datasets e pesos neurais via <code>G.loadCSV("dados.csv")</code>, <code>G.save("pesos.bin")</code> e <code>G.load("pesos.bin")</code>.</li>
    </ul>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/ghost_matrix.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="cm">// Álgebra Hipercomplexa G e Ativações de IA: GhostMatrix</span>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// 1. Instanciação com o construtor GhostMatrix</span>
    <span class="type">GhostMatrix</span> G(<span class="num">2</span>, <span class="num">2</span>);

    <span class="cm">// 2. Definindo elementos hipercomplexos 4D (x, y, p, theta2)</span>
    <span class="type">ghost</span> g1 = ghost(<span class="num">2.0</span>, <span class="num">1.0</span>, <span class="num">0.5</span>, <span class="num">0.0</span>);
    <span class="type">ghost</span> g2 = ghost(-<span class="num">3.0</span>, <span class="num">0.0</span>, <span class="num">1.2</span>, <span class="num">0.0</span>);
    <span class="type">ghost</span> g3 = ghost(<span class="num">1.5</span>, -<span class="num">0.5</span>, <span class="num">0.0</span>, <span class="num">0.0</span>);
    <span class="type">ghost</span> g4 = ghost(<span class="num">4.0</span>, <span class="num">2.0</span>, -<span class="num">1.0</span>, <span class="num">0.0</span>);

    G.set(<span class="num">0</span>, <span class="num">0</span>, g1);
    G.set(<span class="num">0</span>, <span class="num">1</span>, g2);
    G.set(<span class="num">1</span>, <span class="num">0</span>, g3);
    G.set(<span class="num">1</span>, <span class="num">1</span>, g4);

    <span class="fn">println</span>(<span class="str">"Matriz Hipercomplexa GhostMatrix G:"</span>);
    G.show();

    <span class="cm">// 3. Transposição da matriz</span>
    <span class="type">GhostMatrix</span> G_t = G.transpose();
    <span class="fn">println</span>(<span class="str">"Matriz Transposta G^T:"</span>);
    G_t.show();

    <span class="cm">// 4. Multiplicação por escalar com propagação de incerteza</span>
    <span class="type">GhostMatrix</span> G_dobro = G * <span class="num">2.0</span>;
    <span class="fn">println</span>(<span class="str">"Matriz G multiplicada pelo escalar 2.0:"</span>);
    G_dobro.show();

    <span class="cm">// 5. Função de ativação não-linear ReLU para camadas de IA</span>
    <span class="type">GhostMatrix</span> G_relu = G.relu();
    <span class="fn">println</span>(<span class="str">"Matriz apos ativacao ReLU:"</span>);
    G_relu.show();
}
        </pre>
    </div>

    <h2>6.4 Tensores N-Dimensionais e Broadcasting</h2>
    <p>
        Importando o módulo <code>libs/tensor.g</code>, você tem acesso a tensores de qualquer número de dimensões (1D, 2D, 3D, 4D), com suporte a fatiamento inteligente (slicing), permutações de eixos e broadcasting automático semelhante ao NumPy:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/tensores.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/tensor.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// Tensor 3D de shape (2, 3, 4) preenchido com zeros ou valores aleatórios</span>
    <span class="type">tensor</span> T = tensor_zeros([<span class="num">2</span>, <span class="num">3</span>, <span class="num">4</span>]);
    T.set([<span class="num">0</span>, <span class="num">1</span>, <span class="num">2</span>], <span class="num">99.5</span>);
    
    <span class="fn">println</span>(<span class="str">f"Dimensoes do Tensor: {T.shape()}"</span>);
    <span class="fn">println</span>(<span class="str">f"Valor indexado em [0, 1, 2]: {T.get([0, 1, 2])}"</span>);

    <span class="cm">// Reshape para formato 2D (6, 4)</span>
    <span class="type">tensor</span> T_2d = T.reshape([<span class="num">6</span>, <span class="num">4</span>]);
    <span class="fn">println</span>(<span class="str">f"Novo shape apos reshape: {T_2d.shape()}"</span>);
}
        </pre>
    </div>

    <!-- SEÇÃO DE ATIVIDADES PRÁTICAS DO CAPÍTULO -->
    <div class="exercises-section">
        <div class="exercises-title">
            <span>✏️</span> Laboratório Prático: Exercícios do Capítulo 6
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 3.1: Comutador de Operadores de GhostNumberG</span>
                <span class="exercise-badge badge-easy">Aquecimento</span>
            </div>
            <div class="exercise-desc">
                Crie dois números hiper-complexos <span class="math">A = (1.0, 2.0, 0.5, 0.0)</span> e <span class="math">B = (0.5, -1.0, 2.0, 0.0)</span>. Calcule o comutador quântico definido por <span class="math">[A, B] = A · B - B · A</span>. Imprima o resultado e verifique se o comutador é diferente de zero.
            </div>
            <div class="exercise-hint">Dica: Use operadores nativos de soma e produto entre objetos do tipo <code>ghost</code>.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 3.2: Solucionador de Sistemas Lineares Ax = b</span>
                <span class="exercise-badge badge-medium">Aplicação</span>
            </div>
            <div class="exercise-desc">
                Escreva um programa que configure uma matriz <span class="math">A</span> de 3 × 3 e um vetor coluna <span class="math">b</span> de 3 × 1. Resolva o sistema linear <span class="math">Ax = b</span> calculando <span class="math">x = A⁻¹ · b</span>. Em seguida, multiplique <span class="math">A · x</span> e confira se o resultado coincide numericamente com <span class="math">b</span>.
            </div>
            <div class="exercise-hint">Dica: Use <code>matrix.inverse()</code> e o operador de multiplicação <code>*</code>.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 3.3: Atenuador de Incerteza Tensorial</span>
                <span class="exercise-badge badge-hard">Desafio</span>
            </div>
            <div class="exercise-desc">
                Crie um tensor 3D de ruído com shape [4, 4, 4] contendo medições experimentais. Implemente uma função que percorra todos os elementos, extraia a componente fantasma <span class="math">p</span> de incerteza e aplique uma máscara de corte que zera qualquer incerteza inferior a um limiar <span class="math">ε = 0.01</span>.
            </div>
            <div class="exercise-hint">Dica: Utilize laços aninhados ou a iteração do tensor para aplicar o filtro.</div>
        </div>
    </div>
</div>
"""
