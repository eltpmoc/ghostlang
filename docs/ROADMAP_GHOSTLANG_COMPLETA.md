# Roadmap Oficial de Maturidade - GhostLang

> **Situação em 19/11/2026:** versão 0.9, em desenvolvimento. Progresso no dashboard: 76,7% (94 recursos no nível 4, 7 no 3, 1 no 2 e 3 no 1; nenhum no nível 5, que exige CI rodando e Linux/macOS comprovados).
> **Checklist oficial:** 86 de 91 itens. Faltam: Build e testes no Linux revalidados; Build no macOS; CI em matriz Windows/Linux/macOS; Instalador Windows e pacotes Linux; Backend para hardware quântico real.
> **Testes:** 211 testes do CTest passando no Windows x64.
> **Plataformas:** só Windows x64 testado. Linux (Fedora 43) e macOS 26 são as próximas etapas; o CI (`.github/workflows/ci.yml`, repositório https://github.com/eltpmoc/ghostlang) roda o Windows como alvo verificado e Linux/macOS como experimentais.
> **Ainda incompletos (nível 1 ou 2):** Backends quânticos reais (IBM, IonQ, Braket), Vulkan / Metal / ROCm, Linux x64, macOS (arm64/x64).
> *Cabeçalho gerado por `scripts/python/sync_manual_maturidade.py` a partir de `dashboard/ghostlang-status.json`.*

Este documento registra a trajetória de evolução da GhostLang e o que ainda falta para ela ser uma linguagem completa, madura, documentada, distribuível e de alta performance. A maturidade de cada recurso, com o motivo, fica no dashboard (`dashboard/ghostlang-status.json`).

A lista esta organizada por camadas: nucleo da linguagem, compilador, runtime,
bibliotecas padrao, desktop, tooling, distribuicao, testes e governanca do
projeto.

## 1. Definicao oficial da linguagem

Antes de adicionar muitos recursos novos, a GhostLang precisa ter uma
especificacao oficial. Hoje a linguagem ja possui parser, exemplos, manual e
runtime, mas algumas capacidades existem no REPL, outras no compilador, e outras
aparecem em exemplos ou no roadmap.

Falta criar:

- Especificacao formal da sintaxe.
- Especificacao formal dos tipos.
- Especificacao de escopo, lifetime e visibilidade.
- Especificacao de modulos e imports.
- Especificacao do comportamento de erros.
- Especificacao da biblioteca padrao.
- Especificacao do alvo C++ gerado.
- Definicao clara do que e estavel, experimental e legado.

Entregaveis recomendados:

- `SPEC.md`: documento oficial da linguagem.
- `GRAMMAR.md`: gramatica em EBNF ou formato parecido.
- `STD_LIB.md`: contrato da biblioteca padrao.
- `COMPILER_ARCHITECTURE.md`: arquitetura do compilador/transpilador.
- `COMPATIBILITY.md`: matriz de suporte Windows, Linux, macOS e WebAssembly.

## 2. Nucleo da linguagem

### 2.1 Sintaxe e gramatica

Falta estabilizar a sintaxe para que todos os comandos tenham uma forma unica,
previsivel e documentada.

Melhorias necessarias:

- Padronizar obrigatoriedade de `;` em todos os comandos.
- Padronizar declaracoes com e sem `main`.
- Decidir se `state nome(4, 4)` e oficialmente valido ou se `state` deve ser
  apenas vetor.
- Separar claramente `state`, `gate`, `matrix`, `tensor` e `network`.
- Definir se `var` e `auto`, `double` ou inferencia real.
- Definir sintaxe oficial para arrays, listas e mapas.
- Definir sintaxe oficial para strings multilinha.
- Definir sintaxe oficial para interpolacao, como `f"valor={x}"`.
- Definir comentarios de linha e bloco como parte formal da linguagem.
- Definir regras para nomes, Unicode em identificadores e palavras reservadas.

### 2.2 Sistema de tipos

Hoje ha tipos fortes mapeados para C++, mas falta um sistema de tipos mais
completo e consistente.

Falta implementar ou estabilizar:

- Booleano nativo: `bool`.
- Inteiros com tamanhos definidos: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`,
  `u32`, `u64`.
- Numeros de ponto flutuante: `f32`, `f64`.
- Tipo `char`.
- Tipo `array<T>`.
- Tipo `list<T>` ou `vector<T>`.
- Tipo `map<K,V>`.
- Tipo `set<T>`.
- Tipo `optional<T>` ou `maybe<T>`.
- Tipo `result<T,E>` para erros recuperaveis.
- Tipo `bytes` para dados binarios.
- Tipo `path` para caminhos de arquivo.
- Tipos de funcao, callbacks e lambdas.
- Generics/templates para funcoes e classes.
- Conversoes explicitas e implicitas bem definidas.
- Regras de promocao numerica.
- Regras de comparacao entre tipos.
- Regras para `null` ou ausencia de valor.

### 2.3 Expressoes e operadores

Falta completar e validar a semantica de expressoes.

Melhorias:

- Precedencia oficial de operadores.
- Operadores logicos: `&&`, `||`, `!`.
- Operadores bitwise: `&`, `|`, `~`, `<<`, `>>`.
- Modulo/resto: `%`.
- Atribuicoes compostas: `+=`, `-=`, `*=`, `/=`, `%=`.
- Incremento/decremento padronizado.
- Operador ternario ou expressao condicional.
- Comparacao profunda para strings, arrays, json e matrizes.
- Indexacao segura: `lista[i]`.
- Slicing: `lista[ini:fim]`.
- Chamadas encadeadas.
- Lambdas.
- Pattern matching ou `match`.

### 2.4 Controle de fluxo

Parte ja existe, mas precisa ficar uniforme entre compilador e REPL.

Falta completar:

- `switch/case/default` no compilador.
- `foreach` no compilador.
- `break` e `continue` em loops compilados.
- `try/catch/finally` ou modelo proprio de erros.
- `defer` para limpeza.
- `match` para estruturas complexas.
- Loops sobre arrays, strings, mapas e matrizes.
- Escopo correto de variaveis dentro de blocos.

### 2.5 Funcoes

Melhorias necessarias:

- Sobrecarga de funcoes.
- Argumentos opcionais.
- Argumentos nomeados.
- Valores padrao.
- Retorno multiplo ou tuplas.
- Funcoes anonimas.
- Closures.
- Callbacks tipados.
- Passagem por valor, referencia e referencia constante.
- Recursao testada e documentada.
- Funcoes genericas.
- Separacao entre funcoes publicas e privadas em modulos.

### 2.6 Orientacao a objetos

O projeto ja tem `class`, `public`, `private`, `this` e `new`, mas ainda falta
amadurecer para OOP completa.

Falta implementar:

- Construtores.
- Destrutores.
- Heranca.
- Interfaces ou traits.
- Classes abstratas.
- Metodos estaticos.
- Atributos estaticos.
- Encapsulamento real no codigo gerado.
- Modificadores `public`, `private`, `protected`.
- Sobrecarga de metodos.
- Sobrecarga de operadores em classes GhostLang.
- `super`.
- Composicao documentada.
- Objetos alocados no heap com gerenciamento claro.
- Regras de copia/move.

## 3. Compilador e transpilador

Hoje a GhostLang transpila para C++. O caminho e bom, mas precisa ser mais
robusto.

### 3.1 Arquitetura interna

Falta separar o compilador em fases claras:

- Lexer.
- Parser.
- AST.
- Analise semantica.
- Checagem de tipos.
- Resolucao de nomes.
- Expansao de imports.
- Geracao de C++.
- Otimizacoes.
- Emissao de binario.

Hoje muito codigo parece gerar C++ diretamente durante o parse. Isso acelera o
desenvolvimento inicial, mas dificulta diagnosticos, otimizacoes e manutencao.

Melhorias recomendadas:

- Criar uma AST real.
- Criar uma tabela de simbolos.
- Criar uma fase de type-checking.
- Criar uma fase de lowering para C++.
- Separar parser de gerador de codigo.
- Ter mensagens de erro com arquivo, linha, coluna e trecho do codigo.
- Recuperar de erros para mostrar varios erros por compilacao.
- Adicionar flags de debug do compilador.

### 3.2 Imports e modulos

Falta um sistema de modulos consistente.

Melhorias:

- Resolver imports relativos ao arquivo atual, nao ao diretorio de execucao.
- Evitar importar o mesmo arquivo duas vezes.
- Detectar ciclos de importacao.
- Suportar `import "lib/http.g" as http`.
- Suportar imports seletivos.
- Criar namespaces.
- Separar modulos publicos e privados.
- Cache de compilacao por modulo.
- Metadados de pacote.

### 3.3 Build e toolchain

Falta tornar o build previsivel e multiplataforma.

Melhorias:

- Comando `ghost build`.
- Comando `ghost run`.
- Comando `ghost test`.
- Comando `ghost fmt`.
- Comando `ghost doc`.
- Comando `ghost package`.
- Cache de arquivos temporarios.
- Opcao para manter ou apagar `_temp.cpp`.
- Modo verbose com comando C++ completo.
- Modo release/debug.
- Suporte a clang++ alem de g++.
- Suporte a MSVC no Windows.
- Suporte oficial a MinGW.
- Suporte oficial a Emscripten.
- Cross-compilation documentada.

### 3.4 Diagnosticos

Falta profissionalizar as mensagens do compilador.

Necessario:

- Erros com linha e coluna.
- Mostrar trecho do codigo com marcador.
- Sugestoes de correcao.
- Avisos (`warnings`) separados de erros.
- Codigos de erro, como `GHOST-E1001`.
- Modo `--strict`.
- Modo `--warnings-as-errors`.
- Relatorio de imports.
- Relatorio de tipos inferidos.
- Relatorio do C++ gerado.

## 4. Runtime e biblioteca base

### 4.1 Runtime comum

Criar um runtime central `GhostRuntime` com:

- Inicializacao multiplataforma.
- Configuracao de encoding/UTF-8.
- Gerenciamento de argumentos CLI.
- Tratamento global de erros.
- Logging.
- Sistema de paths.
- Sistema de ambiente.
- Inicializacao de rede.
- Inicializacao de threads.
- Inicializacao opcional de GPU.

### 4.2 Erros e excecoes

Falta um modelo oficial de erro.

Opcoes:

- Modelo baseado em excecoes.
- Modelo `Result<T,E>`.
- Modelo hibrido.

Necessario:

- Stack trace GhostLang.
- Erros recuperaveis.
- Erros fatais.
- `try/catch`.
- `panic`.
- `assert`.
- Logs de erro.

### 4.3 Memoria e seguranca

Como a linguagem gera C++, precisa definir bem seguranca de memoria.

Melhorias:

- Evitar `system()` em libs sensiveis.
- Sanitizar comandos de shell.
- Sanitizar paths.
- Bloquear path traversal em todas as libs de arquivo.
- Preferir APIs nativas em vez de shell.
- Usar RAII no runtime C++.
- Definir ownership de objetos.
- Evitar variaveis globais geradas sem necessidade.
- Criar modo seguro para execucao de scripts.

## 5. Biblioteca padrao essencial

Hoje ja existem libs iniciais: `http`, `http_client`, `fs`, `string`, `time`,
`crypto`, `console`, `os`, `math`. Elas precisam ser expandidas e padronizadas.

### 5.1 Lib `core`

Funcoes basicas:

- `type_of(valor)`.
- `assert(condicao, mensagem)`.
- `panic(mensagem)`.
- `clone(valor)`.
- `equals(a, b)`.
- `hash(valor)`.
- `range(inicio, fim, passo)`.
- `min`, `max`, `clamp`.
- `swap`.
- Conversoes seguras.

### 5.2 Lib `collections`

Estruturas:

- `Array<T>`.
- `List<T>`.
- `Map<K,V>`.
- `Set<T>`.
- `Queue<T>`.
- `Stack<T>`.
- `Deque<T>`.
- `PriorityQueue<T>`.

Operacoes:

- `map`.
- `filter`.
- `reduce`.
- `sort`.
- `find`.
- `contains`.
- `join`.
- `split`.
- Iteradores.

### 5.3 Lib `string`

Expandir para:

- `trim`.
- `starts_with`.
- `ends_with`.
- `contains`.
- `replace`.
- `replace_all`.
- `split`.
- `join`.
- `to_upper`.
- `to_lower`.
- `substring`.
- `index_of`.
- `last_index_of`.
- Unicode real.
- Normalizacao UTF-8.
- Regex compilada.

### 5.4 Lib `fs`

Expandir para:

- `read_text`.
- `write_text`.
- `append_text`.
- `read_bytes`.
- `write_bytes`.
- `copy`.
- `move`.
- `remove`.
- `exists`.
- `is_file`.
- `is_dir`.
- `list_dir`.
- `walk_dir`.
- `create_dir`.
- `create_dir_all`.
- `temp_dir`.
- `current_dir`.
- `absolute_path`.
- `canonical_path`.
- `watch_file`.
- `watch_dir`.

### 5.5 Lib `path`

Necessaria para multiplataforma:

- `path_join`.
- `path_dirname`.
- `path_basename`.
- `path_ext`.
- `path_without_ext`.
- `path_normalize`.
- `path_is_absolute`.
- Separadores Windows/Linux/macOS.

### 5.6 Lib `json`

Hoje existe parser simples. Para producao, precisa:

- Parser JSON completo.
- Arrays JSON.
- Objetos aninhados.
- Numeros, bool, null.
- Escapes completos.
- Serializacao.
- Desserializacao tipada.
- JSON Pointer.
- Validacao.
- Pretty print.

### 5.7 Lib `toml`, `yaml`, `csv`, `xml`

Criar libs para formatos comuns:

- `toml_parse`, `toml_get`.
- `yaml_parse`, `yaml_get`.
- `csv_read`, `csv_write`.
- `xml_parse`, `xml_query`.

### 5.8 Lib `math`

Reestruturar e remover duplicacoes.

Adicionar:

- Funcoes trigonometrica completas.
- Estatistica: media, mediana, desvio padrao.
- Algebra linear basica.
- Numeros aleatorios.
- Distribuicoes.
- Otimizacao numerica.
- Interpolacao.
- FFT.
- Funcoes para vetores.
- Funcoes para matrizes.

### 5.9 Lib `datetime`

Adicionar:

- `now`.
- `utc_now`.
- Timezones.
- Duracoes.
- Parsing ISO-8601.
- Formatacao.
- Diferenca entre datas.
- Adicao/subtracao de duracoes.
- Timers.
- Cron-like scheduler.

### 5.10 Lib `net`

Adicionar:

- TCP client/server.
- UDP.
- DNS.
- TLS.
- Sockets async.
- WebSocket client/server.
- URL parser.
- Query string.
- Multipart/form-data.

### 5.11 Lib `http`

Expandir o servidor atual:

- Middleware.
- Headers.
- Cookies.
- Sessions.
- Query params.
- Path params.
- Multipart upload.
- Static files com cache configuravel.
- CORS.
- Compression gzip/br.
- TLS/HTTPS.
- Streaming responses.
- WebSockets.
- Rate limiting.
- Router por grupos.
- Logs de acesso.
- Modo dev com reload.

### 5.12 Lib `crypto`

Base64 e bom inicio, mas faltam:

- SHA-256.
- SHA-512.
- HMAC.
- AES.
- ChaCha20.
- RSA/ECDSA wrappers.
- Random seguro.
- Password hashing: Argon2, bcrypt ou scrypt.
- JWT.
- Certificados.

### 5.13 Lib `database`

Criar:

- SQLite.
- PostgreSQL.
- MySQL/MariaDB.
- ODBC.
- Redis.
- Migrations.
- Query builder.
- Prepared statements.
- Connection pool.

### 5.14 Lib `process`

Adicionar:

- Spawn de processo seguro.
- Captura de stdout/stderr.
- Exit code.
- Variaveis de ambiente.
- Working directory.
- Timeout.
- Sinais.
- Pipes.

### 5.15 Lib `logging`

Adicionar:

- Niveis: trace, debug, info, warn, error, fatal.
- Logs coloridos.
- Logs JSON.
- Rotacao de arquivos.
- Contexto.
- Integracao com servidor HTTP.

### 5.16 Lib `testing`

Adicionar:

- `test`.
- `assert_equal`.
- `assert_true`.
- `assert_throws`.
- Mocks simples.
- Benchmarks.
- Coverage.
- Test runner.

## 6. IA, algebra e ciencia de dados

Essa e a area de identidade mais forte da GhostLang.

### 6.1 Matrix/tensor

Melhorias:

- Separar `GhostMatrix`, `Tensor`, `State`, `Gate`.
- Tensores N-dimensionais.
- Broadcasting.
- Shape checking.
- Operacoes vetorizadas.
- Serializacao de tensores.
- Load/save binario.
- Operacoes BLAS quando disponivel.
- Suporte opcional a OpenBLAS/MKL.

### 6.2 Deep learning

Adicionar:

- Camadas: dense, conv2d, pooling, dropout, embedding.
- Ativacoes: relu, leaky_relu, gelu, softmax, tanh.
- Loss: mse, mae, cross_entropy.
- Otimizadores: SGD, Adam, AdamW, RMSProp.
- Backprop consistente.
- Batches.
- Dataset API.
- Checkpoints.
- Export/import de modelos.
- Inference mode.
- Metrics.
- Treino supervisionado e nao supervisionado.

### 6.3 LLM

Expandir `GhostLLM`:

- Streaming de tokens.
- System prompt.
- Temperatura/top-p/top-k.
- Context size configuravel.
- Chat history nativo.
- Tool calling local.
- Embeddings.
- RAG local.
- Vector store.
- Model registry.
- Suporte multiplataforma robusto para llama.cpp.
- Tratamento de erros de modelo.
- Cancelamento de inferencia.

### 6.4 Data science

Criar libs:

- `dataframe`.
- `series`.
- CSV/Parquet.
- Normalizacao.
- One-hot encoding.
- Train/test split.
- Graficos.
- Estatistica.
- Pipelines.

## 7. Concorrencia e async

Hoje existem `spawn`, `mutex`, `lock`, `unlock` e `sleep`. Falta um modelo
maior.

Adicionar:

- `async` e `await`.
- Futures/promises.
- Channels.
- Thread pools.
- Tasks.
- Timers async.
- Mutex RAII.
- Read/write locks.
- Atomic types.
- Semaphores.
- Condvars.
- Cancelamento.
- Structured concurrency.
- Event loop.

## 8. Desktop multiplataforma

Este e um bloco novo e importante para tornar a GhostLang util em aplicacoes
desktop no Windows, Linux e macOS.

### 8.1 Estrategia recomendada

Criar uma biblioteca padrao `lib/desktop.g` por cima de um backend C++ nativo.
Existem tres caminhos possiveis:

1. Backend WebView:
   - Usa HTML/CSS/JS para UI.
   - Usa WebView2 no Windows.
   - Usa WebKitGTK no Linux.
   - Usa WKWebView no macOS.
   - Mais rapido para implementar e bom para apps modernos.

2. Backend Native Widgets:
   - Usa Win32/WinUI no Windows.
   - Usa GTK/Qt/wxWidgets no Linux.
   - Usa Cocoa/AppKit no macOS.
   - Visual mais nativo, mas mais trabalhoso.

3. Backend Immediate Mode:
   - Usa Dear ImGui, SDL ou GLFW.
   - Bom para ferramentas, dashboards, editores e apps tecnicos.
   - Menos nativo, mas simples e portavel.

Recomendacao pratica:

- Fase 1: `GhostDesktop` com WebView.
- Fase 2: `GhostUI` com widgets nativos ou Qt/wxWidgets.
- Fase 3: `GhostCanvas` com renderizacao 2D/3D.

### 8.2 Lib `desktop`

API desejada:

- `desktop_init()`.
- `desktop_run()`.
- `desktop_quit()`.
- `window_create(title, width, height)`.
- `window_show(window)`.
- `window_hide(window)`.
- `window_close(window)`.
- `window_set_title(window, title)`.
- `window_set_size(window, width, height)`.
- `window_center(window)`.
- `window_maximize(window)`.
- `window_minimize(window)`.
- `window_fullscreen(window, enabled)`.
- `window_on_close(window, callback)`.

### 8.3 Lib `webview`

Para apps desktop modernos:

- `webview_create(title, width, height)`.
- `webview_load_html(window, html)`.
- `webview_load_file(window, path)`.
- `webview_load_url(window, url)`.
- `webview_eval_js(window, js)`.
- `webview_bind(window, name, callback)`.
- `webview_set_devtools(window, enabled)`.
- `webview_set_icon(window, path)`.

Com isso, GhostLang poderia criar apps assim:

```ghost
import "lib/desktop.g";
import "lib/webview.g";

function string on_click(string payload) {
    return "{\"ok\": true}";
}

function main() {
    var app = webview_create("Meu App Ghost", 900, 600);
    webview_bind(app, "on_click", on_click);
    webview_load_file(app, "index.html");
    desktop_run();
}
```

### 8.4 Lib `ui`

Se houver widgets nativos:

- `button`.
- `label`.
- `textbox`.
- `textarea`.
- `checkbox`.
- `radio`.
- `select`.
- `slider`.
- `progress`.
- `table`.
- `tree`.
- `tabs`.
- `menu`.
- `toolbar`.
- `statusbar`.
- `split_view`.
- `scroll_view`.
- `dialog`.

Eventos:

- `on_click`.
- `on_change`.
- `on_input`.
- `on_submit`.
- `on_key`.
- `on_mouse`.
- `on_resize`.
- `on_focus`.

### 8.5 Lib `dialog`

Desktop precisa de dialogs multiplataforma:

- `dialog_open_file`.
- `dialog_save_file`.
- `dialog_select_folder`.
- `dialog_message`.
- `dialog_confirm`.
- `dialog_error`.
- `dialog_color`.
- `dialog_font`.

### 8.6 Lib `clipboard`

Adicionar:

- `clipboard_get_text`.
- `clipboard_set_text`.
- `clipboard_get_image`.
- `clipboard_set_image`.

### 8.7 Lib `tray`

Adicionar:

- `tray_create`.
- `tray_set_icon`.
- `tray_set_tooltip`.
- `tray_add_menu_item`.
- `tray_on_click`.
- `tray_destroy`.

### 8.8 Lib `notification`

Adicionar notificacoes nativas:

- Windows Toast Notifications.
- Linux libnotify.
- macOS UserNotifications.

API:

- `notify(title, message)`.
- `notify_with_icon(title, message, icon)`.
- `notify_action(title, message, action_label, callback)`.

### 8.9 Lib `menu`

Adicionar menus nativos:

- Menu da aplicacao.
- Menu de janela.
- Context menu.
- Atalhos.
- Separadores.
- Checkable menu items.

### 8.10 Lib `screen`

Adicionar:

- Listar monitores.
- Tamanho da tela.
- DPI.
- Escala.
- Posicionamento de janela por monitor.

### 8.11 Lib `input`

Adicionar:

- Teclado.
- Mouse.
- Atalhos globais.
- Drag and drop.
- Gamepad.

### 8.12 Lib `canvas`

Para desenho 2D:

- Criar canvas.
- Linhas.
- Retangulos.
- Circulos.
- Texto.
- Imagens.
- Gradientes.
- Transformacoes.
- Eventos de mouse.
- Export PNG.

### 8.13 Lib `graphics3d`

Para apps tecnicos, simuladores e jogos:

- Janela OpenGL/Vulkan/Metal via camada portavel.
- Mesh.
- Camera.
- Material.
- Texture.
- Shader.
- Scene.
- Model loading.

### 8.14 Lib `audio`

Adicionar:

- Play sound.
- Music stream.
- Microfone.
- Volume.
- Dispositivos.
- Formatos WAV/MP3/OGG.

### 8.15 Lib `packaging_desktop`

Para entregar apps reais:

- Gerar `.exe` com icone no Windows.
- Gerar instalador `.msi` ou `.exe`.
- Gerar `.AppImage`, `.deb`, `.rpm` no Linux.
- Gerar `.app` e `.dmg` no macOS.
- Assinatura de codigo.
- Notarization no macOS.
- Auto-update.
- Bundling de assets.
- Bundling de modelos GGUF.

## 9. WebAssembly e web

Ja existe flag `--wasm`, mas precisa amadurecer.

Falta:

- Template HTML padrao.
- Export automatico de funcoes.
- Bindings JS gerados.
- Tipos suportados documentados.
- Modo browser sem servidor TCP.
- Exemplos oficiais.
- Build web com assets.
- Integracao com canvas/webgl.
- Pacote npm opcional.

## 10. Ferramentas de desenvolvedor

### 10.1 Formatador

Criar `ghost fmt`:

- Indentacao padrao.
- Espacos em operadores.
- Quebra de linhas.
- Organizacao de imports.
- Formatacao de classes e funcoes.

### 10.2 Linter

Criar `ghost lint`:

- Variavel nao usada.
- Import nao usado.
- Funcao sem retorno.
- Codigo inalcançavel.
- Conversoes perigosas.
- Sombra de variavel.
- Uso de APIs inseguras.

### 10.3 Language Server

Criar LSP para VS Code e outras IDEs:

- Autocomplete.
- Go to definition.
- Find references.
- Rename symbol.
- Hover docs.
- Diagnostics em tempo real.
- Semantic highlighting.
- Code actions.
- Formatting.

### 10.4 Debugger

Criar modo debug:

- Breakpoints.
- Step over/into/out.
- Watch variables.
- Stack trace.
- Mapeamento GhostLang -> C++ gerado.
- Debug via GDB/LLDB por baixo.

### 10.5 Package manager: GhostPM

Criar gerenciador:

- `ghost init`.
- `ghost add pacote`.
- `ghost remove pacote`.
- `ghost update`.
- `ghost publish`.
- `ghost.lock`.
- `ghost.toml`.
- Versoes semanticas.
- Repositorio local.
- Repositorio remoto.
- Cache.
- Assinatura/verificacao.

## 11. Testes e qualidade

Falta uma estrategia forte de testes.

Criar:

- Testes unitarios do lexer.
- Testes unitarios do parser.
- Testes de AST.
- Testes de geracao C++.
- Testes de compilacao real.
- Testes de execucao.
- Testes de runtime.
- Testes das libs.
- Testes multiplataforma.
- Testes WebAssembly.
- Golden tests para mensagens de erro.
- Fuzzing do parser.
- Fuzzing de JSON/HTTP.
- Benchmarks.

Matriz minima:

- Windows x64.
- Linux x64.
- macOS arm64.
- macOS x64, se possivel.

## 12. Documentacao

Falta transformar a documentacao em material profissional.

Criar:

- Guia de instalacao.
- Tutorial "Hello Ghost".
- Tutorial CLI.
- Tutorial HTTP API.
- Tutorial desktop.
- Tutorial IA local.
- Tutorial deep learning.
- Tutorial WebAssembly.
- Referencia da linguagem.
- Referencia da biblioteca padrao.
- Guia de migracao por versao.
- Cookbook.
- FAQ.
- Pagina de erros comuns.
- Exemplos completos.

Corrigir:

- Encoding/acentuacao dos arquivos existentes.
- Duplicacoes no manual.
- Divergencias entre manual e implementacao.
- Exemplos que nao compilam.

## 13. Distribuicao e instalacao

Falta empacotar a GhostLang como produto instalavel.

Criar:

- Instalador Windows.
- Pacote Linux `.deb`.
- Pacote Linux `.rpm`.
- AppImage.
- Homebrew formula para macOS.
- Script de instalacao.
- Versao portable.
- Releases versionadas.
- Checksums.
- Assinatura de binarios.
- Canal nightly.
- Canal stable.

## 14. Seguranca

Pontos importantes:

- Evitar shell injection em `os_exec`, HTTP Linux via `curl`, `dir_create` e
  chamadas similares.
- Sanitizar inputs de servidor HTTP.
- Limitar tamanho de request.
- Melhorar parser HTTP.
- HTTPS/TLS.
- Secrets/env management.
- Permissoes para scripts.
- Sandbox opcional.
- Auditoria de dependencias.

## 15. Performance

Melhorias:

- Benchmarks oficiais.
- Perfilamento do compilador.
- Perfilamento do runtime.
- Otimizacao de `GhostMatrix`.
- Evitar copias desnecessarias.
- SIMD.
- OpenMP configuravel.
- Backend BLAS.
- Cache de compilacao.
- Incremental compilation.
- Precompiled headers.

## 16. Compatibilidade multiplataforma

Hoje ha partes especificas para Windows e Linux. Para ficar completa:

- Criar camada `GhostPlatform`.
- Remover comandos shell dependentes de Windows, como `>nul`, de libs comuns.
- Implementar paths portaveis.
- Implementar HTTP client nativo portavel ou usar lib multiplataforma.
- Padronizar encoding UTF-8.
- Testar terminal em Windows Terminal, PowerShell, CMD, bash, zsh.
- Testar filesystem com espacos, acentos e caracteres especiais.
- Testar rede em Windows/Linux/macOS.
- Testar empacotamento em todos os sistemas.

## 17. Priorizacao sugerida

### Fase 1: estabilizacao

- Corrigir documentacao e encoding.
- Criar `SPEC.md`.
- Separar recursos estaveis/experimentais.
- Corrigir divergencias entre manual, REPL e compilador.
- Melhorar erros do compilador.
- Criar testes basicos.

### Fase 2: compilador serio

- AST real.
- Tabela de simbolos.
- Type checker.
- Modulos/imports robustos.
- `ghost build/run/test/fmt`.

### Fase 3: biblioteca padrao essencial

- `core`.
- `collections`.
- `fs`.
- `path`.
- `json`.
- `string`.
- `datetime`.
- `net`.
- `testing`.

### Fase 4: HTTP e IA

- HTTP server completo.
- HTTP client seguro.
- WebSocket.
- LLM streaming.
- Dataset API.
- Deep learning mais consistente.

### Fase 5: desktop

- `desktop`.
- `webview`.
- `dialog`.
- `clipboard`.
- `tray`.
- `notification`.
- Packaging desktop.

### Fase 6: distribuicao publica

- Instaladores.
- Package manager GhostPM.
- LSP.
- Debugger.
- CI multiplataforma.
- Versao 1.0.

## 18. Definicao de "GhostLang 1.0"

A GhostLang pode ser considerada uma linguagem completa e finalizada na versao
1.0 quando tiver:

- Especificacao oficial.
- Compilador estavel.
- Type checker consistente.
- Erros claros.
- Biblioteca padrao essencial.
- Test suite automatizada.
- Documentacao coerente.
- Instaladores.
- VS Code/LSP funcional.
- Suporte Windows, Linux e macOS.
- Exemplos reais mantidos.
- Compatibilidade entre versoes.
- Roadmap de evolucao sem quebrar codigo antigo sem aviso.

## 19. Lista curta do que mais falta

Em resumo, os maiores blocos faltantes sao:

- Especificacao formal da linguagem.
- AST e analise semantica no compilador.
- Sistema de tipos mais completo.
- Modulos/imports robustos.
- Erros profissionais.
- Biblioteca padrao maior.
- Testes automatizados.
- Package manager.
- LSP e debugger.
- Desktop multiplataforma.
- Empacotamento/instaladores.
- Seguranca e sandbox.
- Documentacao limpa e consistente.

## 20. Direcao estrategica

A melhor identidade para a GhostLang nao e tentar copiar JavaScript, Python,
Rust ou C#. A forca dela esta em ser uma linguagem nativa para:

- IA local.
- Matematica/tensores.
- Backends pequenos e rapidos.
- Ferramentas desktop.
- Automacao.
- Aplicacoes full-stack compactas.
- Experimentos com LLM, algebra fantasma e redes neurais.

O caminho mais forte e transformar a GhostLang em uma linguagem "AI-native
systems scripting": simples por cima, C++ rapido por baixo, com libs prontas
para IA, HTTP, desktop e automacao multiplataforma.
