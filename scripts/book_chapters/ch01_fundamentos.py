# -*- coding: utf-8 -*-
"""
Capítulo 1 e Capítulo 2: Fundamentos, Instalação, Sintaxe, Tipos e Operadores.
"""

def get_ch01_html():
    return r"""
<!-- CAPÍTULO 1 -->
<div class="chapter">
    <span class="chapter-badge">Capítulo 1</span>
    <h1 class="chapter-title">Introdução, Instalação e Arquitetura da Toolchain</h1>
    <div class="chapter-intro">
        Compreenda a anatomia do GhostLang: como o compilador transforma código-fonte de alto nível em binários nativos C++17 ultra-otimizados com suporte a OpenMP e CUDA.
    </div>

    <h2>1.1 Como o GhostLang Funciona</h2>
    <p>
        Diferente de linguagens interpretadas tradicionais que dependem de uma máquina virtual pesada (como JVM ou CPython), o GhostLang opera sob um modelo de <strong>transpilação de alta fidelidade para C++17 ISO</strong>.
    </p>

    <div class="code-block">
        <div class="code-header">
            <span class="code-filename">Pipeline de Compilação do GhostLang</span>
            <span>Fluxo Arquitetural</span>
        </div>
        <pre>
Código Fonte (.g) 
  ↓ [Lexer (Tokenização & Comentários)]
Fluxo de Tokens
  ↓ [Parser & GhostAst (Construção da Árvore Sintática Abstrata)]
AST Unificada
  ↓ [GhostTypes (Verificação Semântica Estrita - 102 Regras)]
AST Validada & Tipada
  ↓ [GhostCodegen (Emissão de C++17 + Runtime Nativo)]
Código C++17 Otimizado (com #include &lt;GhostMatrix.hpp&gt;, &lt;GhostQuantum.hpp&gt;)
  ↓ [g++ / clang++ / MSVC (-O3 -fopenmp -static)]
Binário Executável Nativo Autônomo (.exe / ELF)
        </pre>
    </div>

    <h2>1.2 Instalação e Estrutura de Diretórios</h2>
    <p>
        O GhostLang é fornecido como uma distribuição portátil ou compilável via CMake. Na raiz do projeto, encontramos a seguinte organização canônica:
    </p>

    <table>
        <thead>
            <tr>
                <th>Diretório</th>
                <th>Finalidade</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td><code>bin/</code></td>
                <td>Binários executáveis principais (<code>ghost.exe</code>, <code>ghost_api</code>, <code>ghost_ask</code>).</td>
            </tr>
            <tr>
                <td><code>include/</code></td>
                <td>Cabeçalhos C++ do runtime (<code>GhostQuantum.hpp</code>, <code>GhostMatrix.hpp</code>, <code>GhostHybrid.hpp</code>).</td>
            </tr>
            <tr>
                <td><code>libs/</code> e <code>modules/</code></td>
                <td>Bibliotecas da biblioteca padrão em GhostLang (<code>fs.g</code>, <code>http.g</code>, <code>json.g</code>, <code>tempo.g</code>).</td>
            </tr>
            <tr>
                <td><code>tests/</code></td>
                <td>Suíte de testes de regressão (167 testes CTest, testes golden e oráculos da AST).</td>
            </tr>
            <tr>
                <td><code>dashboard/</code></td>
                <td>Painel de métricas e status oficial da linguagem (<code>ghostlang-status.json</code>).</td>
            </tr>
        </tbody>
    </table>

    <h2>1.3 Seu Primeiro Programa: "Olá, Universo Quântico!"</h2>
    <p>
        Abra o seu editor favorito e salve o seguinte arquivo como <code>ola_mundo.g</code>:
    </p>

    <div class="code-block">
        <div class="code-header">
            <span class="code-filename">exemplos/ola_mundo.g</span>
            <span>GhostLang Source</span>
        </div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/quantum.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">string</span> nome = <span class="str">"Pioneiro Quântico"</span>;
    <span class="type">int</span> ano = <span class="num">2026</span>;
    
    <span class="fn">println</span>(<span class="str">f"Bem-vindo ao GhostLang, {nome}! Ano de consolidação: {ano}."</span>);
    
    <span class="cm">// Criando um qubit em superposição instantânea</span>
    <span class="type">qreg</span> q(<span class="num">1</span>);
    q.h(<span class="num">0</span>); <span class="cm">// Porta Hadamard</span>
    
    <span class="fn">println</span>(<span class="str">"Estado do qubit |ψ⟩:"</span>);
    q.show();
}
        </pre>
    </div>

    <p>
        Para compilar e executar diretamente em um único passo, use a CLI do GhostLang:
    </p>

    <div class="code-block">
        <div class="code-header"><span>Terminal</span><span>Bash / PowerShell</span></div>
        <pre>ghost run ola_mundo.g</pre>
    </div>

    <div class="callout callout-tip">
        <div class="callout-title">💡 Dica de Compilação Estática</div>
        Por padrão no Windows, <code>ghost build programa.g</code> gera um executável com linkagem estática total (<code>-x</code>), ou seja, você pode copiar o <code>programa.exe</code> resultante para qualquer computador Windows sem precisar instalar nenhuma DLL adicional!
    </div>
</div>

<!-- CAPÍTULO 2 -->
<div class="chapter">
    <span class="chapter-badge">Capítulo 2</span>
    <h1 class="chapter-title">Sintaxe Fundamental, Tipos Primitivos e Operadores</h1>
    <div class="chapter-intro">
        Domine os blocos de construção da linguagem: tipagem estática com inferência, números complexos de dupla precisão nativos e interpolação de strings moderna.
    </div>

    <h2>2.1 Tipos de Dados Primitivos</h2>
    <p>
        O GhostLang possui tipagem estática e fortemente vinculada, prevenindo falhas de memória e garantindo que o compilador C++ gere as instruções mais rápidas de máquina.
    </p>

    <table>
        <thead>
            <tr>
                <th>Tipo</th>
                <th>Descrição</th>
                <th>Exemplo</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td><code>int</code></td>
                <td>Inteiro de 64 bits com sinal.</td>
                <td><code>int valor = 42;</code></td>
            </tr>
            <tr>
                <td><code>float</code></td>
                <td>Ponto flutuante de precisão dupla (IEEE 754 64-bit).</td>
                <td><code>float pi = 3.14159265;</code></td>
            </tr>
            <tr>
                <td><code>string</code></td>
                <td>Sequência de caracteres UTF-8 nativa.</td>
                <td><code>string texto = "GhostLang";</code></td>
            </tr>
            <tr>
                <td><code>bool</code></td>
                <td>Booleano lógico bilíngue (<code>true</code>/<code>false</code> ou <code>verdadeiro</code>/<code>falso</code>).</td>
                <td><code>bool ativo = verdadeiro;</code></td>
            </tr>
            <tr>
                <td><code>complex</code></td>
                <td>Número complexo de dupla precisão com sufixo literal <code>i</code>.</td>
                <td><code>complex z = 3.0 + 4.0i;</code></td>
            </tr>
            <tr>
                <td><code>ghost</code></td>
                <td>Número hipercomplexo quântico 4D (x + y·i + p·g + θ²).</td>
                <td><code>ghost g = ghost(3.0, 4.0, 1.7, 2.0);</code></td>
            </tr>
            <tr>
                <td><code>matrix</code></td>
                <td>Matriz e Álgebra Linear clássica/numérica de alta performance.</td>
                <td><code>matrix A(2, 2);</code></td>
            </tr>
            <tr>
                <td><code>GhostMatrix</code></td>
                <td>Matriz hipercomplexa 4D, tensores e camadas neurais de IA.</td>
                <td><code>GhostMatrix G(2, 2);</code></td>
            </tr>
            <tr>
                <td><code>json</code></td>
                <td>Objeto JSON estruturado de primeira classe com serialização e parsing.</td>
                <td><code>json obj = {"nome": "Ghost"};</code></td>
            </tr>
            <tr>
                <td><code>void</code></td>
                <td>Ausência de retorno em funções de comando.</td>
                <td><code>function void executar() { }</code></td>
            </tr>
        </tbody>
    </table>

    <h2>2.2 Números Complexos Nativos</h2>
    <p>
        Em muitas linguagens, números complexos exigem a importação de bibliotecas lentas. No GhostLang, o número imaginário <code>i</code> é um literal nativo do Lexer. Você pode somar, subtrair, multiplicar e dividir números complexos diretamente:
    </p>

    <div class="code-block">
        <div class="code-header">
            <span class="code-filename">exemplos/complexos.g</span>
            <span>GhostLang Source</span>
        </div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">complex</span> z1 = <span class="num">1.0</span> + <span class="num">2.0i</span>;
    <span class="type">complex</span> z2 = <span class="num">3.0</span> - <span class="num">1.0i</span>;
    
    <span class="type">complex</span> soma = z1 + z2;       <span class="cm">// 4.0 + 1.0i</span>
    <span class="type">complex</span> produto = z1 * z2;    <span class="cm">// (1*3 - 2*(-1)) + (1*(-1) + 2*3)i = 5.0 + 5.0i</span>
    
    <span class="fn">println</span>(<span class="str">f"z1 = {z1}"</span>);
    <span class="fn">println</span>(<span class="str">f"z2 = {z2}"</span>);
    <span class="fn">println</span>(<span class="str">f"z1 + z2 = {soma}"</span>);
    <span class="fn">println</span>(<span class="str">f"z1 * z2 = {produto}"</span>);
}
        </pre>
    </div>

    <h2>2.3 Interpolação de Texto (f-strings)</h2>
    <p>
        O GhostLang oferece interpolação de strings de alta performance em tempo de compilação com a sintaxe <code>f"..."</code>. As expressões dentro das chaves <code>{expressao}</code> são avaliadas e convertidas diretamente para string sem alocações desnecessárias. Para imprimir uma chave literal, basta duplicá-la: <code>{{</code> ou <code>}}</code>.
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/fstrings.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">float</span> raio = <span class="num">5.0</span>;
    <span class="type">float</span> area = <span class="num">3.14159</span> * raio * raio;
    <span class="fn">println</span>(<span class="str">f"Circulo de raio {raio} tem area = {area} m2."</span>);
    <span class="fn">println</span>(<span class="str">f"JSON simulado: {{\"raio\": {raio}, \"area\": {area}}}"</span>);
}
        </pre>
    </div>

    <h2>2.4 Entrada de Dados Interativa: <code>input</code> e <code>inputln</code></h2>
    <p>
        Para interagir com o usuário via terminal ou capturar parâmetros em tempo de execução, o GhostLang oferece suporte nativo e de alta performance à leitura de dados pelo console com descarte automático de quebras de linha residuais:
    </p>

    <h3>2.4.1 Atribuição Direta com Retorno Tipado (<code>input</code>)</h3>
    <p>
        A função <code>input(prompt)</code> opera como uma expressão com retorno dinamicamente conversível, atribuindo o texto digitado diretamente ao tipo estático da variável (<code>string</code>, <code>int</code>, <code>float</code>, <code>double</code> ou <code>bool</code>):
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/entrada_input.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">string</span> nome = <span class="fn">input</span>(<span class="str">"Digite o seu nome completo: "</span>);
    <span class="type">int</span> idade = <span class="fn">input</span>(<span class="str">"Digite a sua Idade: "</span>);
    <span class="type">float</span> valor = <span class="fn">input</span>(<span class="str">"Digite o valor do produto: "</span>);

    <span class="fn">println</span>(<span class="str">f"Cliente cadastrado: {nome}, {idade} anos, R$ {valor:.2f}"</span>);
}
        </pre>
    </div>

    <h3>2.4.2 Declaração Prévia e Vinculação Direta com f-string (<code>inputln</code>)</h3>
    <p>
        A instrução de comando <code>inputln(f"Prompt: {variavel}")</code> permite projetar o prompt e vincular a variável receptora diretamente dentro das chaves <code>{variavel}</code> da f-string:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/entrada_inputln.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">string</span> nome;
    <span class="fn">inputln</span>(<span class="str">f"Digite o seu nome completo: {nome}"</span>);

    <span class="type">int</span> idade;
    <span class="fn">inputln</span>(<span class="str">f"Digite a sua Idade: {idade}"</span>);

    <span class="type">float</span> valor;
    <span class="fn">inputln</span>(<span class="str">f"Digite o valor do produto: {valor}"</span>);

    <span class="fn">println</span>(<span class="str">f"Dados capturados: {nome} | {idade} anos | R$ {valor:.2f}"</span>);
}
        </pre>
    </div>

    <h2>2.5 Tabela de Operadores e Precedência</h2>
    <p>
        Os operadores do GhostLang seguem uma ordem estrita e bem definida de precedência matemática:
    </p>

    <table>
        <thead>
            <tr>
                <th>Categoria</th>
                <th>Operadores</th>
                <th>Associatividade</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td>Primários</td>
                <td><code>()</code>, <code>[]</code>, <code>.</code>, chamadas de método</td>
                <td>Esquerda para Direita</td>
            </tr>
            <tr>
                <td>Unários</td>
                <td><code>+</code>, <code>-</code>, <code>!</code>, <code>~</code>, <code>++</code>, <code>--</code></td>
                <td>Direita para Esquerda</td>
            </tr>
            <tr>
                <td>Exponenciação / Potência</td>
                <td><code>**</code> (ou função <code>potencia</code> / <code>pow</code>)</td>
                <td>Direita para Esquerda</td>
            </tr>
            <tr>
                <td>Multiplicativos</td>
                <td><code>*</code>, <code>/</code>, <code>%</code></td>
                <td>Esquerda para Direita</td>
            </tr>
            <tr>
                <td>Aditivos</td>
                <td><code>+</code>, <code>-</code></td>
                <td>Esquerda para Direita</td>
            </tr>
            <tr>
                <td>Bitwise / Deslocamento</td>
                <td><code>&lt;&lt;</code>, <code>&gt;&gt;</code>, <code>&amp;</code>, <code>|</code>, <code>^</code> (XOR bit a bit)</td>
                <td>Esquerda para Direita</td>
            </tr>
            <tr>
                <td>Pipeline Neural</td>
                <td><code>&gt;&gt;</code> (encadeamento de camadas em redes)</td>
                <td>Esquerda para Direita</td>
            </tr>
            <tr>
                <td>Relacionais</td>
                <td><code>&lt;</code>, <code>&lt;=</code>, <code>&gt;</code>, <code>&gt;=</code></td>
                <td>Esquerda para Direita</td>
            </tr>
            <tr>
                <td>Igualdade</td>
                <td><code>==</code>, <code>!=</code></td>
                <td>Esquerda para Direita</td>
            </tr>
            <tr>
                <td>Lógicos</td>
                <td><code>e</code> (ou <code>&amp;&amp;</code>), <code>ou</code> (ou <code>||</code>)</td>
                <td>Esquerda para Direita</td>
            </tr>
            <tr>
                <td>Atribuição</td>
                <td><code>=</code>, <code>+=</code>, <code>-=</code>, <code>*=</code>, <code>/=</code>, <code>**=</code></td>
                <td>Direita para Esquerda</td>
            </tr>
        </tbody>
    </table>

    <h2>2.6 Operações Matemáticas Clássicas e Avançadas (libs/math.g)</h2>
    <p>
        O GhostLang possui suporte nativo integral à matemática analítica, clássica e computacional de alta precisão.
        A linguagem integra operadores de ponta diretamente na sintaxe do compilador, além de fornecer a biblioteca especialista <code>libs/math.g</code>.
    </p>

    <h3>2.6.1 Exponenciação e Potenciação (x^n)</h3>
    <p>
        A exponenciação (<span class="math">x<sup>n</sup></span>, lê-se <em>"x elevado a n"</em>) utiliza o operador universal <code>**</code>, possuindo alta precedência e associatividade da direita para a esquerda:
    </p>
    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/potencia.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">int</span> base_int = <span class="num">2</span>;
    <span class="type">int</span> exp_int = <span class="num">10</span>;
    <span class="type">int</span> resultado = base_int ** exp_int; <span class="cm">// 2^10 = 1024 (inteiro exato)</span>
    <span class="type">double</span> fracao = <span class="num">4.0</span> ** -<span class="num">0.5</span>;        <span class="cm">// 4^(-0.5) = 0.5 (flutuante)</span>
    <span class="fn">println</span>(<span class="str">f"2 elevado a 10 = {resultado}"</span>);
    <span class="fn">println</span>(<span class="str">f"4 elevado a -0.5 = {fracao}"</span>);
}
        </pre>
    </div>

    <h3>2.6.2 Raiz Quadrada e Raiz n-ésima Universal (<sup>n</sup>&radic;x)</h3>
    <p>
        Para o cálculo de raízes, o GhostLang oferece tanto as funções elementares <code>sqrt(x)</code> (raiz quadrada, idêntica ao padrão de C++) e <code>cbrt(x)</code> (raiz cúbica), quanto a função universal para <strong>raiz n-ésima</strong> <code>raiz_n(x, n)</code> (também disponível como <code>nth_root(x, n)</code>).
    </p>
    <p>
        Onde o índice da raiz <span class="math">n</span> pode ser <strong>qualquer número natural maior do que zero</strong> (<span class="math">n &ge; 1</span>):
    </p>
    <ul>
        <li><strong>Radicando positivo (<span class="math">x &ge; 0</span>):</strong> calcula <span class="math"><sup>n</sup>&radic;x = x<sup>1/n</sup></span> com rigor IEEE 754 em precisão dupla (<code>double</code>).</li>
        <li><strong>Radicando negativo (<span class="math">x &lt; 0</span>) com índice ímpar:</strong> calcula a raiz real exata no corpo dos reais: <span class="math"><sup>3</sup>&radic;(-8) = -2</span>, <span class="math"><sup>5</sup>&radic;(-32) = -2</span>.</li>
        <li><strong>Raiz de números negativos e corpo dos Complexos (<code>complex</code>):</strong> Como o GhostLang possui suporte nativo ao tipo <code>complex</code>, <strong>é plenamente possível calcular raízes de números negativos e raízes de índice par!</strong> O GhostLang disponibiliza as funções especializadas <code>csqrt(z)</code> (raiz quadrada complexa) e <code>raiz_n_complexa(z, n)</code> (ou <code>nth_root_c(z, n)</code> / sobrecarga <code>raiz_n(complex z, n)</code>). Ao passar um número negativo, o cálculo opera no corpo dos complexos retornando o ramo principal:
            <br>&bull; <span class="math">&radic;(-4) = 2i</span> &rarr; <code>(0, 2)</code>
            <br>&bull; <span class="math">&radic;(-1) = 1i</span> &rarr; <code>(0, 1)</code>
            <br>&bull; <span class="math">&radic;(-16) = 4i</span> &rarr; <code>(0, 4)</code>
            <br>&bull; <span class="math"><sup>4</sup>&radic;(-16) = &radic;2 + &radic;2&middot;i</span> &rarr; <code>(1.41421, 1.41421)</code>
            <br>&bull; Raízes de números complexos arbitrários: <code>sqrt(3.0 + 4.0i)</code> &rarr; <code>(2, 1)</code> (<span class="math">2 + 1i</span>).
        </li>
        <li><strong>Validação estrita de domínio:</strong> caso <span class="math">n &le; 0</span>, lança erro informando que o índice deve ser um número natural maior do que zero (<span class="math">n &ge; 1</span>). No domínio estritamente real (<code>raiz_n(double, int)</code> com índice par e radicando negativo), orienta o desenvolvedor a utilizar o tipo nativo <code>complex</code>.</li>
    </ul>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/raizes.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/math.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// 1. Raizes no dominio real</span>
    <span class="type">double</span> raiz2 = <span class="fn">sqrt</span>(<span class="num">144.0</span>);             <span class="cm">// Raiz quadrada: 12.0</span>
    <span class="type">double</span> raiz3 = <span class="fn">cbrt</span>(<span class="num">125.0</span>);             <span class="cm">// Raiz cubica: 5.0</span>
    <span class="type">double</span> raiz10 = <span class="fn">raiz_n</span>(<span class="num">1024.0</span>, <span class="num">10</span>);      <span class="cm">// Raiz decima de 1024: 2.0</span>
    <span class="type">double</span> raiz5_neg = <span class="fn">nth_root</span>(-<span class="num">32.0</span>, <span class="num">5</span>);   <span class="cm">// Raiz quinta de -32: -2.0</span>
    <span class="fn">println</span>(<span class="str">f"Raiz decima de 1024 = {raiz10}"</span>);
    <span class="fn">println</span>(<span class="str">f"Raiz quinta de -32 = {raiz5_neg}"</span>);

    <span class="cm">// 2. Raizes de numeros negativos com complex nativo</span>
    <span class="type">complex</span> raiz_neg4 = <span class="fn">csqrt</span>(-<span class="num">4.0</span>);         <span class="cm">// Raiz quadrada de -4: 2i -> (0, 2)</span>
    <span class="type">complex</span> raiz_neg1 = <span class="fn">csqrt</span>(-<span class="num">1.0</span>);         <span class="cm">// Raiz quadrada de -1: 1i -> (0, 1)</span>
    <span class="type">complex</span> raiz_cplx = <span class="fn">sqrt</span>(<span class="num">3.0</span> + <span class="num">4.0i</span>);    <span class="cm">// Raiz de complexo -> (2, 1)</span>
    <span class="type">complex</span> raiz4_neg16 = <span class="fn">raiz_n_complexa</span>(-<span class="num">16.0</span>, <span class="num">4</span>); <span class="cm">// Raiz 4a de -16: (1.41421, 1.41421)</span>
    <span class="type">complex</span> raiz2_neg9 = <span class="fn">raiz_n</span>(-<span class="num">9.0</span> + <span class="num">0.0i</span>, <span class="num">2</span>);     <span class="cm">// Sobrecarga complexa -> (0, 3)</span>
    <span class="fn">println</span>(<span class="str">f"csqrt(-4.0) = {raiz_neg4}"</span>);
    <span class="fn">println</span>(<span class="str">f"csqrt(-1.0) = {raiz_neg1}"</span>);
    <span class="fn">println</span>(<span class="str">f"raiz_n_complexa(-16, 4) = {raiz4_neg16}"</span>);
    <span class="fn">println</span>(<span class="str">f"raiz_n(-9+0i, 2) = {raiz2_neg9}"</span>);
}
        </pre>
    </div>

    <h3>2.6.3 Panorama Completo da Biblioteca libs/math.g</h3>
    <table>
        <thead>
            <tr>
                <th>Domínio Matemático</th>
                <th>Funções e Fórmulas Disponíveis</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td><strong>Constantes Fundamentais</strong></td>
                <td><code>const_pi()</code> (&pi;), <code>const_tau()</code> (2&pi;), <code>const_e()</code> (e), <code>const_phi()</code> (&phi;), <code>const_sqrt2()</code> (&radic;2), <code>const_ln2()</code>, <code>const_ln10()</code></td>
            </tr>
            <tr>
                <td><strong>Aritmética e Teoria dos Números</strong></td>
                <td><code>soma</code>, <code>subtracao</code>, <code>multiplicacao</code>, <code>divisao</code>, <code>resto_divisao</code>, <code>mdc(a,b)</code>, <code>mmc(a,b)</code>, <code>eh_primo(n)</code>, <code>proximo_primo(n)</code>, <code>fatorial(n)</code>, <code>permutacao(n,k)</code>, <code>combinacao(n,k)</code></td>
            </tr>
            <tr>
                <td><strong>Potenciação e Raízes</strong></td>
                <td><code>potencia(base, exp)</code> (ou <code>**</code>), <code>raiz_quadrada(x)</code> (ou <code>sqrt</code>), <code>raiz_cubica(x)</code> (ou <code>cbrt</code>), <code>raiz_n(x, n)</code> / <code>nth_root(x, n)</code>, <code>csqrt(z)</code>, <code>raiz_n_complexa(z, n)</code> / <code>nth_root_c(z, n)</code>, <code>hipotenusa(a,b)</code></td>
            </tr>
            <tr>
                <td><strong>Exponenciais e Logaritmos</strong></td>
                <td><code>exponencial(x)</code> (e<sup>x</sup>), <code>exponencial2(x)</code> (2<sup>x</sup>), <code>logaritmo_natural(x)</code> (ln x), <code>logaritmo_base10(x)</code> (log<sub>10</sub>), <code>logaritmo_base2(x)</code> (log<sub>2</sub>), <code>logaritmo_base(x, b)</code> (log<sub>b</sub>)</td>
            </tr>
            <tr>
                <td><strong>Trigonometria e Hiperbólicas</strong></td>
                <td><code>seno</code>, <code>cosseno</code>, <code>tangente</code>, <code>cotangente</code>, <code>secante</code>, <code>cossecante</code>, <code>arco_seno</code>, <code>arco_cosseno</code>, <code>arco_tangente</code>, <code>arco_tangente2(y, x)</code>, <code>seno_hiperbolico</code>, <code>cosseno_hiperbolico</code>, <code>tangente_hiperbolica</code>, <code>graus_para_radianos</code>, <code>radianos_para_graus</code></td>
            </tr>
            <tr>
                <td><strong>Funções Especiais e Ativações</strong></td>
                <td><code>funcao_gama(x)</code> (&Gamma;), <code>log_gama(x)</code>, <code>funcao_beta(a, b)</code>, <code>funcao_erro(x)</code> (erf), <code>funcao_erro_complementar(x)</code> (erfc), <code>sigmoid(x)</code>, <code>relu(x)</code>, <code>gelu(x)</code></td>
            </tr>
        </tbody>
    </table>

    <h2>2.7 Estatística e Ciência de Dados (libs/stats.g)</h2>
    <p>
        Para análise quantitativa de dados, modelos preditivos e avaliação de amostragem quântica, a biblioteca <code>libs/stats.g</code> oferece um ferramental estatístico completo:
    </p>

    <table>
        <thead>
            <tr>
                <th>Categoria Estatística</th>
                <th>Funções e Métricas Disponíveis</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td><strong>Tendência Central</strong></td>
                <td><code>media(xs)</code>, <code>mediana(xs)</code>, <code>moda(xs)</code>, <code>media_ponderada(xs, pesos)</code>, <code>media_geometrica(xs)</code>, <code>media_harmonica(xs)</code>, <code>media_aparada(xs, p)</code></td>
            </tr>
            <tr>
                <td><strong>Dispersão e Extremos</strong></td>
                <td><code>minimo(xs)</code>, <code>maximo(xs)</code>, <code>amplitude(xs)</code>, <code>variancia(xs)</code> (amostral N-1), <code>variancia_populacional(xs)</code> (N), <code>desvio_padrao(xs)</code>, <code>desvio_padrao_populacional(xs)</code>, <code>erro_padrao(xs)</code>, <code>coeficiente_variacao(xs)</code>, <code>desvio_medio_absoluto(xs)</code></td>
            </tr>
            <tr>
                <td><strong>Posição e Forma</strong></td>
                <td><code>quantil(xs, q)</code>, <code>percentil(xs, p)</code>, <code>quartis(xs)</code> [Q1, Q2, Q3], <code>intervalo_interquartil(xs)</code> (IQR), <code>assimetria(xs)</code> (Skewness Fisher-Pearson), <code>curtose(xs)</code> (Kurtosis), <code>z_scores(xs)</code>, <code>escalonar_min_max(xs)</code></td>
            </tr>
            <tr>
                <td><strong>Bivariada e Regressão</strong></td>
                <td><code>covariancia(xs, ys)</code>, <code>covariancia_populacional(xs, ys)</code>, <code>correlacao_pearson(xs, ys)</code>, <code>regressao_linear(xs, ys)</code> retornando <code>[inclinacao, intercepto, r2]</code></td>
            </tr>
            <tr>
                <td><strong>Probabilidade e Amostragem</strong></td>
                <td><code>normal_pdf(x, mu, sigma)</code>, <code>normal_cdf(x, mu, sigma)</code>, <code>poisson_pmf(k, lambda)</code>, <code>binomial_pmf(k, n, p)</code>, <code>lcg_uniform(seed)</code></td>
            </tr>
        </tbody>
    </table>

    <!-- SEÇÃO DE ATIVIDADES PRÁTICAS DO CAPÍTULO -->
    <div class="exercises-section">
        <div class="exercises-title">
            <span>✏️ </span> Laboratório Prático: Exercícios do Capítulo 1 & 2
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 1.1: Calculadora de Impedância Complexa</span>
                <span class="exercise-badge badge-easy">Aquecimento</span>
            </div>
            <div class="exercise-desc">
                Crie um programa <code>impedancia.g</code> que receba a resistência <span class="math">R</span> = 50.0 Ω e a reatância indutiva <span class="math">X<sub>L</sub></span> = 30.0 Ω. Forme o número complexo de impedância <span class="math">Z = R + X<sub>L</sub> · i</span>, calcule seu módulo <span class="math">|Z| = √(R² + X<sub>L</sub>²)</span> usando <code>hypot</code> ou <code>sqrt</code> e exiba o resultado usando f-strings.
            </div>
            <div class="exercise-hint">Dica: Use literais de ponto flutuante explícitos e multiplique por <code>1.0i</code>.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 1.2: Conversor Bilíngue de Lógica Booleana</span>
                <span class="exercise-badge badge-medium">Aplicação</span>
            </div>
            <div class="exercise-desc">
                Escreva um programa que teste a equivalência das palavras-chave booleanas em GhostLang. Crie variáveis usando <code>verdadeiro</code> e <code>falso</code>, combine-as usando os operadores <code>e</code>, <code>ou</code> e <code>nao</code>, e imprima a tabela verdade em formato texto tabular.
            </div>
            <div class="exercise-hint">Dica: O compilador aceita indistintamente <code>true/false</code> ou <code>verdadeiro/falso</code>.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 1.3: Normalizador de Amplitudes Quânticas</span>
                <span class="exercise-badge badge-hard">Desafio</span>
            </div>
            <div class="exercise-desc">
                Dado um estado de superposição quântica arbitrário <span class="math">|ψ⟩ = α|0⟩ + β|1⟩</span>, com <span class="math">α = 3.0 + 1.0i</span> e <span class="math">β = 2.0 - 2.0i</span>, calcule a norma total <span class="math">N = √(|α|² + |β|²)</span> e calcule os coeficientes normalizados <span class="math">α' = α / N</span> e <span class="math">β' = β / N</span>, demonstrando que <span class="math">|α'|² + |β'|² = 1.0</span>.
            </div>
            <div class="exercise-hint">Dica: Em números complexos, o quadrado da magnitude de <span class="math">x + yi</span> é <span class="math">x² + y²</span>.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 1.4: Raiz n-ésima e Crescimento Alométrico</span>
                <span class="exercise-badge badge-medium">Aplicação Matemática</span>
            </div>
            <div class="exercise-desc">
                Escreva um programa <code>escalas.g</code> que importe <code>libs/math.g</code> e calcule o raio característico de um cristal dado seu volume hiperdimensional <span class="math">V = 65536.0</span> na dimensão <span class="math">n = 8</span>, através da fórmula <span class="math">R = <sup>8</sup>&radic;V</span> (utilizando <code>raiz_n(V, 8)</code>). Em seguida, compare com a potenciação direta <code>V ** (1.0 / 8.0)</code> e verifique a igualdade.
            </div>
            <div class="exercise-hint">Dica: 65536 = 2^16, logo a raiz oitava deve resultar exatamente em 4.0.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 1.5: Análise Estatística de Telemetria Quântica</span>
                <span class="exercise-badge badge-hard">Desafio de Dados</span>
            </div>
            <div class="exercise-desc">
                Utilizando <code>libs/stats.g</code>, crie uma lista com 10 medições de fidelidade de porta quântica: <code>[0.98, 0.95, 0.99, 0.94, 0.97, 0.99, 0.96, 0.98, 0.99, 0.95]</code>. Calcule e exiba a média, a mediana, os quartis <span class="math">[Q_1, Q_2, Q_3]</span>, o intervalo interquartil (<span class="math">IQR</span>), a variância amostral e o desvio padrão.
            </div>
            <div class="exercise-hint">Dica: Use <code>quartis(dados)</code> e acesse <code>q[0]</code>, <code>q[1]</code> e <code>q[2]</code>.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 1.6: Sistema Interativo de Cadastro com input e inputln</span>
                <span class="exercise-badge badge-medium">Aplicação Interativa</span>
            </div>
            <div class="exercise-desc">
                Crie um programa <code>cadastro_interativo.g</code> que utilize os dois formatos de entrada de dados do GhostLang: primeiro leia o nome do operador e a frequência de amostragem em GHz via atribuição com <code>input(...)</code>; em seguida, declare previamente as variáveis de potência e número de disparos e utilize <code>inputln(f"Prompt: {var}")</code> para preenchê-las. No final, exiba um relatório completo usando interpolação moderna de f-strings.
            </div>
            <div class="exercise-hint">Dica: O runtime do GhostLang gerencia o buffer do console, permitindo alternar leituras numéricas e textuais consecutivas com segurança.</div>
        </div>
    </div>
</div>
"""
