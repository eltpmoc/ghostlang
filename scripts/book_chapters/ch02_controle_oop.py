# -*- coding: utf-8 -*-
"""
Capítulo 3, 4 e 5: Controle de Fluxo, Funções, Lambdas, Concorrência, OOP e Módulos.
"""

def get_ch02_html():
    return """
<!-- CAPÍTULO 3 -->
<div class="chapter">
    <span class="chapter-badge">Capítulo 3</span>
    <h1 class="chapter-title">Controle de Fluxo, Coleções e Tratamento de Exceções</h1>
    <div class="chapter-intro">
        Explore estruturas de repetição otimizadas, o laço de iteração <code>foreach</code>, coleções nativas dinâmicas e o mecanismo robusto de captura de erros <code>try/catch/finally</code>.
    </div>

    <h2>3.1 Estruturas Condicionais: <code>if / else</code> e <code>switch / case</code></h2>
    <p>
        O GhostLang adota controle de fluxo estruturado em estilo C/C++, com condicionais <code>if / else</code> e seleção multi-ramo de alta performance via <code>switch / case</code> com cláusula <code>default</code>:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/controle.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">avaliar_porta</span>(<span class="type">string</span> porta) {
    <span class="kw">switch</span> (porta) {
        <span class="kw">case</span> <span class="str">"H"</span>:
            <span class="fn">println</span>(<span class="str">"Hadamard: Cria superposicao equilibrada."</span>);
            <span class="kw">break</span>;
        <span class="kw">case</span> <span class="str">"X"</span>:
            <span class="fn">println</span>(<span class="str">"Pauli-X: Inversor de bit quântico (NOT)."</span>);
            <span class="kw">break</span>;
        <span class="kw">case</span> <span class="str">"CX"</span>:
            <span class="fn">println</span>(<span class="str">"CNOT: Entrelaçamento de 2 qubits."</span>);
            <span class="kw">break</span>;
        <span class="kw">default</span>:
            <span class="fn">println</span>(<span class="str">f"Porta desconhecida ou customizada: {porta}"</span>);
    }
}
        </pre>
    </div>

    <h2>3.2 Laços de Repetição: <code>while</code>, <code>for</code> e <code>foreach</code></h2>
    <p>
        Para iterações clássicas com contador ou condição prévia, usamos <code>for</code> e <code>while</code>; para percorrer coleções, listas nativas ou até mesmo registradores quânticos <code>qreg</code>, o laço <code>foreach (... in ...)</code> oferece a abordagem mais concisa, legível e segura:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/lacos.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// 1. Laço tradicional com acumulador</span>
    <span class="type">int</span> total = <span class="num">0</span>;
    <span class="kw">for</span> (<span class="type">int</span> i = <span class="num">1</span>; i &lt;= <span class="num">10</span>; i = i + <span class="num">1</span>) {
        total = total + i;
    }
    <span class="fn">println</span>(<span class="str">f"Soma de 1 a 10 = {total}"</span>);

    <span class="cm">// 2. Coleções dinâmicas e foreach</span>
    <span class="type">list</span>&lt;<span class="type">string</span>&gt; nomes = [<span class="str">"Qubit-0"</span>, <span class="str">"Qubit-1"</span>, <span class="str">"Qubit-2"</span>];
    nomes.append(<span class="str">"Qubit-3"</span>);
    
    <span class="kw">foreach</span> (<span class="type">string</span> item <span class="kw">in</span> nomes) {
        <span class="fn">println</span>(<span class="str">f"Processando nó: {item}"</span>);
    }
}
        </pre>
    </div>

    <h2>3.3 Tratamento de Exceções: <code>try / catch / finally</code></h2>
    <p>
        Operações que possam falhar (como divisão por zero, leitura de arquivos inexistentes ou índices fora do limite) devem ser protegidas com blocos estruturados de exceção. O bloco <code>finally</code> é sempre executado, garantindo liberação de recursos mesmo em caso de erro:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/erros.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">float</span> <span class="fn">dividir_seguro</span>(<span class="type">float</span> a, <span class="type">float</span> b) {
    <span class="kw">try</span> {
        <span class="kw">if</span> (b == <span class="num">0.0</span>) {
            <span class="kw">throw</span> <span class="str">"Erro de Divisão: Denominador não pode ser nulo!"</span>;
        }
        <span class="kw">return</span> a / b;
    } <span class="kw">catch</span> (<span class="type">string</span> erro) {
        <span class="fn">println</span>(<span class="str">f"Interceptado: {erro}"</span>);
        <span class="kw">return</span> <span class="num">0.0</span>;
    } <span class="kw">finally</span> {
        <span class="fn">println</span>(<span class="str">"Operação aritmética finalizada."</span>);
    }
}
        </pre>
    </div>
</div>

<!-- CAPÍTULO 4 -->
<div class="chapter">
    <span class="chapter-badge">Capítulo 4</span>
    <h1 class="chapter-title">Funções de Primeira Classe, Lambdas e Concorrência</h1>
    <div class="chapter-intro">
        Aprenda a tratar funções como valores, criar funções anônimas (lambdas) com captura léxica de escopo e disparar threads concorrentes com sincronização via mutex.
    </div>

    <h2>4.1 Funções de Primeira Classe e Expressões Lambda</h2>
    <p>
        Em GhostLang, funções são tratadas como cidadãs de primeira classe: podem ser passadas como argumentos, retornadas por outras funções ou armazenadas em variáveis. As expressões lambda possuem sintaxe dual, oferecendo máxima expressividade:
    </p>
    <ul>
        <li><strong>Notação Matemática com Pipes (padrão moderno):</strong> <code>|parametros| =&gt; expressao</code> (para zero parâmetros: <code>|| =&gt; expressao</code>).</li>
        <li><strong>Notação Clássica com Parênteses:</strong> <code>(parametros) =&gt; expressao</code> (para zero parâmetros: <code>() =&gt; expressao</code>).</li>
    </ul>
    <p>
        Os parâmetros suportam tanto <strong>tipagem estática explícita</strong> (como <code>|int x|</code>) quanto <strong>inferência automática polimórfica</strong> (como <code>|x|</code>). As coleções <code>list&lt;T&gt;</code> expõem métodos funcionais nativos de ordem superior (<code>map</code>, <code>filter</code>, <code>reduce</code> e <code>for_each</code>):
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/lambdas.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="cm">// 1. Lambdas atribuídas a variáveis (tipada e inferida)</span>
    <span class="kw">var</span> dobrar = |<span class="type">int</span> x| =&gt; x * <span class="num">2</span>;
    <span class="kw">var</span> quadrado = |x| =&gt; x * x;
    <span class="fn">println</span>(<span class="str">f"O dobro de 7 e: {dobrar(7)}"</span>);
    <span class="fn">println</span>(<span class="str">f"O quadrado de 9 e: {quadrado(9)}"</span>);

    <span class="cm">// 2. Pipeline funcional sobre coleções</span>
    <span class="type">list</span>&lt;<span class="type">int</span>&gt; valores = [<span class="num">1</span>, <span class="num">2</span>, <span class="num">3</span>, <span class="num">4</span>, <span class="num">5</span>, <span class="num">6</span>];
    <span class="type">list</span>&lt;<span class="type">int</span>&gt; pares = valores.filter(|x| =&gt; x % <span class="num">2</span> == <span class="num">0</span>);
    <span class="type">list</span>&lt;<span class="type">int</span>&gt; valoresDobrados = pares.map(|x| =&gt; x * <span class="num">2</span>);
    
    <span class="fn">println</span>(<span class="str">f"Numeros pares: {pares}"</span>);
    <span class="fn">println</span>(<span class="str">f"Valores dobrados: {valoresDobrados}"</span>);
}
        </pre>
    </div>

    <h2>4.2 Concorrência Real com Threads e Mutex</h2>
    <p>
        Ao contrário de interpretadores que possuem travas globais (GIL), o GhostLang compila para threads reais do sistema operacional através do módulo nativo <code>std::threads</code>:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/threads_mutex.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/threads.g"</span>;

<span class="type">int</span> contador_global = <span class="num">0</span>;
<span class="type">mutex</span> trava_memoria;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">incrementar_trabalho</span>(<span class="type">int</span> id, <span class="type">int</span> repeticoes) {
    <span class="kw">for</span> (<span class="type">int</span> i = <span class="num">0</span>; i &lt; repeticoes; i = i + <span class="num">1</span>) {
        trava_memoria.lock();
        contador_global = contador_global + <span class="num">1</span>;
        trava_memoria.unlock();
    }
    <span class="fn">println</span>(<span class="str">f"Thread {id} concluiu seu lote."</span>);
}

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">thread</span> t1 = spawn(incrementar_trabalho, <span class="num">1</span>, <span class="num">5000</span>);
    <span class="type">thread</span> t2 = spawn(incrementar_trabalho, <span class="num">2</span>, <span class="num">5000</span>);

    t1.join();
    t2.join();

    <span class="fn">println</span>(<span class="str">f"Valor final sincronizado: {contador_global} (Esperado: 10000)"</span>);
}
        </pre>
    </div>
</div>

<!-- CAPÍTULO 5 -->
<div class="chapter">
    <span class="chapter-badge">Capítulo 5</span>
    <h1 class="chapter-title">Programação Orientada a Objetos, Herança e Módulos</h1>
    <div class="chapter-intro">
        Modele sistemas complexos com classes completas, métodos construtores, encapsulamento e importação de módulos com resolução estrita de dependências.
    </div>

    <h2>5.1 Classes, Construtores e Encapsulamento</h2>
    <p>
        Em GhostLang, as classes são cidadãs de primeira classe com suporte a atributos privados e métodos públicos, construtores formais e resolução estática de chamada:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/classes.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">class</span> <span class="type">SensorQuantico</span> {
    <span class="kw">private</span> <span class="type">string</span> id_sensor;
    <span class="kw">private</span> <span class="type">float</span> temperatura_kelvin;
    <span class="kw">public</span> <span class="type">bool</span> calibrado;

    <span class="kw">public</span> <span class="fn">SensorQuantico</span>(<span class="type">string</span> id, <span class="type">float</span> temp) {
        this.id_sensor = id;
        this.temperatura_kelvin = temp;
        this.calibrado = <span class="kw">false</span>;
    }

    <span class="kw">public</span> <span class="type">void</span> <span class="fn">calibrar</span>() {
        <span class="kw">if</span> (this.temperatura_kelvin &lt;= <span class="num">0.05</span>) {
            this.calibrado = <span class="kw">true</span>;
            <span class="fn">println</span>(<span class="str">f"Sensor {this.id_sensor}: Calibracao criogenica BEM-SUCEDIDA!"</span>);
        } <span class="kw">else</span> {
            <span class="fn">println</span>(<span class="str">f"Sensor {this.id_sensor}: Temp alta ({this.temperatura_kelvin}K)."</span>);
        }
    }
}
        </pre>
    </div>

    <h2>5.2 Herança Simples e Polimorfismo Dinâmico</h2>
    <p>
        Uma classe pode estender o comportamento de outra utilizando a palavra-chave <code>extends</code>, aproveitando os métodos da classe base e sobrecarregando comportamentos específicos:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/heranca.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">class</span> <span class="type">DispositivoHardware</span> {
    <span class="kw">public</span> <span class="type">string</span> fabricante;
    
    <span class="kw">public</span> <span class="fn">DispositivoHardware</span>(<span class="type">string</span> fab) {
        this.fabricante = fab;
    }
    
    <span class="kw">public</span> <span class="type">void</span> <span class="fn">descrever</span>() {
        <span class="fn">println</span>(<span class="str">f"Dispositivo de controle fab. por {this.fabricante}."</span>);
    }
}

<span class="kw">class</span> <span class="type">QPU</span> <span class="kw">extends</span> <span class="type">DispositivoHardware</span> {
    <span class="kw">public</span> <span class="type">int</span> contagem_qubits;

    <span class="kw">public</span> <span class="fn">QPU</span>(<span class="type">string</span> fab, <span class="type">int</span> n_qubits) : <span class="kw">super</span>(fab) {
        this.contagem_qubits = n_qubits;
    }

    <span class="kw">public</span> <span class="type">void</span> <span class="fn">descrever</span>() {
        <span class="fn">println</span>(<span class="str">f"QPU {this.fabricante} com {this.contagem_qubits} qubits físicos."</span>);
    }
}
        </pre>
    </div>

    <!-- SEÇÃO DE ATIVIDADES PRÁTICAS DO CAPÍTULO -->
    <div class="exercises-section">
        <div class="exercises-title">
            <span>✏️</span> Laboratório Prático: Exercícios dos Capítulos 3, 4 e 5
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 2.1: Validador de Telemetria com Exceção</span>
                <span class="exercise-badge badge-easy">Aquecimento</span>
            </div>
            <div class="exercise-desc">
                Crie uma função <code>validar_fidelidade(float f)</code> que verifique se uma fidelidade quântica está no intervalo [0.0, 1.0]. Se estiver fora, lance uma exceção textual com <code>throw</code>. No programa principal, teste valores válidos e inválidos dentro de um bloco <code>try/catch</code>.
            </div>
            <div class="exercise-hint">Dica: Trate o valor capturado na cláusula <code>catch (string erro)</code> exibindo uma mensagem de alerta.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 2.2: Pipeline Funcional com Lambdas</span>
                <span class="exercise-badge badge-medium">Aplicação</span>
            </div>
            <div class="exercise-desc">
                Crie uma lista com 8 medições de energia molecular. Usando expressões lambda e métodos encadeados, filtre apenas as energias negativas (<span class="math">E</span> &lt; 0.0), calcule o módulo de cada valor filtrado e ordene a lista resultante.
            </div>
            <div class="exercise-hint">Dica: Utilize métodos encadeados <code>lista.filter(|x| =&gt; ...).map(|x| =&gt; ...)</code>.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 2.3: Fila Concorrente de Trabalhos Quânticos</span>
                <span class="exercise-badge badge-hard">Desafio</span>
            </div>
            <div class="exercise-desc">
                Modele uma classe <code>FilaCircuitos</code> que armazene nomes de circuitos para execução. Duas threads paralelas devem retirar tarefas da fila e executá-las de forma segura sem condição de corrida, usando uma trava <code>mutex</code> para proteger a remoção de itens da lista.
            </div>
            <div class="exercise-hint">Dica: Bloqueie o mutex antes de verificar o tamanho e retirar o elemento da fila.</div>
        </div>
    </div>
</div>
"""
