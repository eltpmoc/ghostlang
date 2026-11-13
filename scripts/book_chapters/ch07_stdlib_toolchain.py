# -*- coding: utf-8 -*-
"""
Capítulo 10 e Capítulo 11: Biblioteca Padrão, Rede, Web, Banco de Dados e Toolchain Oficial.
"""

def get_ch07_html():
    return r"""
<!-- CAPÍTULO 10 -->
<div class="chapter">
    <span class="chapter-badge">Capítulo 10</span>
    <h1 class="chapter-title">Biblioteca Padrão, Rede HTTP RESTful, SQLite e GUI</h1>
    <div class="chapter-intro">
        Conheça as baterias inclusas do GhostLang: I/O de disco ultra-rápido, banco de dados relacional SQLite nativo, servidor HTTP puro em C++ e cliente HTTP para microsserviços.
    </div>

    <h2>10.1 I/O de Arquivos e Serialização JSON</h2>
    <p>
        Operações de sistema de arquivos e parsing de payloads JSON são executadas diretamente sem dependências externas:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/io_json.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/fs.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="type">string</span> caminho = <span class="str">"config.json"</span>;
    <span class="type">string</span> payload = <span class="str">"{\"modelo\": \"VQC-4\", \"qubits\": 4, \"shots\": 1024}"</span>;

    <span class="cm">// Escrita direta no disco</span>
    writefile(caminho, payload);
    <span class="fn">println</span>(<span class="str">"Arquivo de configuração salvo com sucesso."</span>);

    <span class="cm">// Leitura e desserialização JSON nativa</span>
    <span class="type">string</span> conteudo = readfile(caminho);
    <span class="kw">var</span> cfg = json_parse(conteudo);
    
    <span class="fn">println</span>(<span class="str">f"Modelo: {cfg[\"modelo\"]}"</span>);
    <span class="fn">println</span>(<span class="str">f"Qubits alocados: {cfg[\"qubits\"]}"</span>);
}
        </pre>
    </div>

    <h2>10.2 Banco de Dados SQLite Relacional Embutido (GhostDB)</h2>
    <p>
        O GhostLang traz o motor SQLite embutido diretamente no runtime C++, permitindo criar bancos em memória (<code>:memory:</code>) ou em arquivo local com suporte a transações ACID completas:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/sqlite_db.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="kw">import</span> <span class="str">"libs/db.g"</span>;

<span class="kw">function</span> <span class="type">void</span> <span class="fn">main</span>() {
    <span class="kw">var</span> db = db_open(<span class="str">"telemetria.db"</span>);

    <span class="cm">// Criação de tabela</span>
    db.exec(<span class="str">"CREATE TABLE IF NOT EXISTS execucoes (id INTEGER PRIMARY KEY, circuito TEXT, fidelidade REAL);"</span>);

    <span class="cm">// Inserção parametrizada</span>
    db.exec(<span class="str">"INSERT INTO execucoes (circuito, fidelidade) VALUES ('Bell-State', 0.998);"</span>);
    db.exec(<span class="str">"INSERT INTO execucoes (circuito, fidelidade) VALUES ('Grover-3Q', 0.965);"</span>);

    <span class="cm">// Consulta com retorno estruturado</span>
    <span class="kw">var</span> linhas = db.query(<span class="str">"SELECT * FROM execucoes WHERE fidelidade &gt; 0.98;"</span>);
    <span class="kw">foreach</span> (<span class="kw">var</span> reg <span class="kw">in</span> linhas) {
        <span class="fn">println</span>(<span class="str">f"ID: {reg[\"id\"]} | Circuito: {reg[\"circuito\"]} | Fidelidade: {reg[\"fidelidade\"]}"</span>);
    }
}
        </pre>
    </div>

    <h2>10.3 Servidor HTTP Nativo e APIs de Produção</h2>
    <p>
        O GhostLang não precisa de Python, Flask ou NodeJS para disponibilizar modelos na web: um servidor HTTP multithreaded de alta performance faz parte do núcleo:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">exemplos/servidor_http.g</span><span>GhostLang Source</span></div>
        <pre>
<span class="cm">// 1. Instancia o servidor TCP na porta 8080</span>
<span class="type">server</span> app = <span class="num">8080</span>;

<span class="cm">// 2. Handler de endpoint REST</span>
<span class="kw">function</span> <span class="type">string</span> <span class="fn">get_status</span>(<span class="type">string</span> req) {
    <span class="kw">return</span> <span class="str">"{\"status\": \"online\", \"engine\": \"GhostLang HTTP Native Engine\", \"versao\": \"1.5.0\"}"</span>;
}

<span class="kw">function</span> <span class="type">string</span> <span class="fn">post_inferencia</span>(<span class="type">string</span> req) {
    <span class="cm">// Executa predição quântica instantânea e retorna JSON</span>
    <span class="kw">return</span> <span class="str">"{\"predicao\": 0.994, \"classe\": 1}"</span>;
}

<span class="cm">// 3. Roteamento de rotas e escuta de requisições</span>
app.route(<span class="str">"/status"</span>, get_status);
app.route(<span class="str">"/inferir"</span>, post_inferencia);

<span class="fn">println</span>(<span class="str">"Servidor GhostLang ouvindo na porta 8080..."</span>);
app.listen();
        </pre>
    </div>
</div>

<!-- CAPÍTULO 11 -->
<div class="chapter">
    <span class="chapter-badge">Capítulo 11</span>
    <h1 class="chapter-title">A Toolchain Oficial (CLI ghost, REPL, Debugger e CTest)</h1>
    <div class="chapter-intro">
        Maximize sua produtividade como desenvolvedor GhostLang: comandos unificados de compilação, o REPL dinâmico para experimentação rápida, depuração com símbolos DWARF e testes automatizados.
    </div>

    <h2>11.1 O Guia de Comandos da CLI <code>ghost</code></h2>
    <p>
        O binário único <code>ghost</code> gerencia todo o ciclo de vida do seu software:
    </p>

    <table>
        <thead>
            <tr>
                <th>Comando</th>
                <th>Ação Executada</th>
                <th>Exemplo de Uso</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td><code>ghost build</code></td>
                <td>Compila arquivo <code>.g</code> para C++17 e gera executável.</td>
                <td><code>ghost build main.g -o app.exe</code></td>
            </tr>
            <tr>
                <td><code>ghost run</code></td>
                <td>Compila em memória temporária e executa imediatamente.</td>
                <td><code>ghost run algoritmo.g</code></td>
            </tr>
            <tr>
                <td><code>ghost test</code></td>
                <td>Executa suíte de testes com asserções automatizadas.</td>
                <td><code>ghost test tests/</code></td>
            </tr>
            <tr>
                <td><code>ghost fmt</code></td>
                <td>Formata canonicamente o código-fonte (estilo padrão).</td>
                <td><code>ghost fmt -w src/*.g</code></td>
            </tr>
            <tr>
                <td><code>ghost check</code></td>
                <td>Executa verificação estática de tipos sem gerar binário.</td>
                <td><code>ghost check codigo.g</code></td>
            </tr>
            <tr>
                <td><code>ghost ast</code></td>
                <td>Exibe a Árvore Sintática Abstrata do programa em JSON.</td>
                <td><code>ghost ast circuito.g</code></td>
            </tr>
            <tr>
                <td><code>ghost debug</code></td>
                <td>Compila com símbolos DWARF <code>-g -O0</code> para depuração.</td>
                <td><code>ghost debug main.g</code></td>
            </tr>
            <tr>
                <td><code>ghost lsp</code></td>
                <td>Inicia o Language Server Protocol sobre stdio para IDEs.</td>
                <td><code>ghost lsp</code></td>
            </tr>
            <tr>
                <td><code>ghost pm</code></td>
                <td>Gerenciador de pacotes GhostPM (<code>init</code>, <code>add</code>, <code>install</code>).</td>
                <td><code>ghost pm add quantum_algorithms</code></td>
            </tr>
        </tbody>
    </table>

    <h2>11.2 O Console Interativo REPL (Read-Eval-Print Loop)</h2>
    <p>
        Executando <code>ghost</code> sem nenhum argumento na linha de comando, você abre instantaneamente o terminal interativo:
    </p>

    <div class="code-block">
        <div class="code-header"><span class="code-filename">Sessão Interativa REPL</span><span>Terminal</span></div>
        <pre>
GhostLang 1.5.0 (C++17 Statevector Engine, OpenMP)
Digite :help para comandos de controle ou Ctrl+C para sair.

ghost> qreg q(1);
ghost> q.h(0);
ghost> q.show();
0.7071|0> + 0.7071|1>
ghost> q.measure(0);
1
ghost> :type q
qreg (1 qubit statevector)
ghost> :quit
Até logo!
        </pre>
    </div>

    <!-- SEÇÃO DE ATIVIDADES PRÁTICAS DO CAPÍTULO -->
    <div class="exercises-section">
        <div class="exercises-title">
            <span>✏️</span> Laboratório Prático: Exercícios dos Capítulos 10 & 11
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 7.1: Servidor de Micro-Serviço de Hashes Criptográficos</span>
                <span class="exercise-badge badge-easy">Aquecimento</span>
            </div>
            <div class="exercise-desc">
                Crie um servidor HTTP escutando na porta 8085 que receba uma string no caminho <code>/encode</code>, codifique-a em Base64 usando a biblioteca <code>crypto_base64_encode</code> e retorne um payload JSON <code>{"original": "...", "base64": "..."}</code>.
            </div>
            <div class="exercise-hint">Dica: Use <code>import "libs/crypto.g"</code> e <code>app.route()</code>.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 7.2: Histórico de Telemetria Quântica com SQLite</span>
                <span class="exercise-badge badge-medium">Aplicação</span>
            </div>
            <div class="exercise-desc">
                Escreva um script que execute 10 circuitos quânticos com ângulos de rotação aleatórios, registre a fidelidade de cada circuito em um banco de dados SQLite local <code>experimentos.db</code> e, em seguida, calcule e exiba a média e o desvio padrão das fidelidades gravadas através de uma consulta SQL agregada.
            </div>
            <div class="exercise-hint">Dica: Utilize <code>SELECT AVG(fidelidade) FROM experimentos;</code>.</div>
        </div>

        <div class="exercise-item">
            <div class="exercise-header">
                <span class="exercise-name">Exercício 7.3: Pipeline de CI Automatizado com ghost test</span>
                <span class="exercise-badge badge-hard">Desafio</span>
            </div>
            <div class="exercise-desc">
                Crie uma pasta de testes contendo 3 arquivos: um teste matemático com <code>assert_eq</code>, um teste quântico verificando a normalização <span class="math">∑ |α|² = 1.0</span> e um teste de rede neural verificando a convergência. Execute todos os testes em lote com <code>ghost test</code> e exiba o sumário de aprovação.
            </div>
            <div class="exercise-hint">Dica: Use a biblioteca <code>libs/test.g</code> com <code>assert_true()</code>.</div>
        </div>
    </div>
</div>
"""
