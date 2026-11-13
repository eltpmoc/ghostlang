# CUDA e modelos de texto nativos — Linux

O compilador construído pelo CMake liga `ghost_runtime` automaticamente. A aplicação não precisa injetar cabeçalhos, compilar seu próprio backend ou montar comandos de linkagem CUDA.

## Compilar o GhostLang

```bash
cmake -S . -B build_native -DGHOST_CUDA=AUTO -DCMAKE_BUILD_TYPE=Release
cmake --build build_native --target ghost -j 4
build_native/ghost tests/ghost/native_textmodel.g -o build_native/exemplo
GHOST_DEVICE=auto build_native/exemplo ok
```

`GHOST_CUDA=AUTO` inclui CUDA quando encontra o toolkit; `ON` exige o toolkit; `OFF` constrói uma biblioteca sem dependência de CUDA. O toolkit é necessário na compilação CUDA. Para executar esse binário é necessário um driver compatível se for usada a GPU; sem dispositivo funcional, `auto` usa CPU.

As arquiteturas padrão do build CUDA são 75, 86 e 89. Para a RTX 4050, pode especificar `-DCMAKE_CUDA_ARCHITECTURES=89`; para outro equipamento, configure sua arquitetura suportada pelo toolkit. O runtime utiliza o dispositivo visível de índice zero, selecionável externamente por `CUDA_VISIBLE_DEVICES`.

`ghost-runtime.conf`, ao lado do compilador, identifica a raiz, a biblioteca estática e cudart. Reconfigure o CMake se mover o projeto ou trocar o diretório do build. Binários produzidos não leem esse manifesto; já contêm o runtime. O fluxo novo foi validado em Linux; o REPL antigo e a saída WebAssembly não oferecem esta integração nativa.

## Escolha de dispositivo

```ghost
pragma auto;
print(device_info());
textmodel ia = text_model(64, 32, 192, 32, 4096, 42);
ia.self_test();
```

Também pode usar `use_device("auto")`, `use_device("cpu")` ou `use_device("cuda")`. `pragma gpu` equivale a CUDA obrigatório; `pragma cpu` força CPU. Sem chamada explícita, o padrão é a variável `GHOST_DEVICE`, ou `auto` quando ausente. `gpu` é aceito como alias de `cuda`.

`auto` realiza uma alocação e executa um kernel de verificação antes de selecionar CUDA. Quando isso falha, informa o motivo em `device_info()` e seleciona CPU. `cuda` obrigatório retorna erro. Falhas durante operações CUDA também geram erro: não há reinício silencioso de um treino parcialmente atualizado.

`device_backend()` retorna `cpu` ou `cuda`; `gpu_available()` informa se a política atual selecionou CUDA, não um inventário físico das placas. `gpu_matrix_calls()` conta produtos matriciais efetivamente executados na GPU.

Escolha o dispositivo **antes de `prepare` ou `load`**. Cada modelo mantém o backend escolhido e seus buffers até uma nova preparação/carga. Uma mudança global afeta futuras matrizes e modelos preparados depois, não migra automaticamente modelos vivos. `ia.backend()` informa o backend da instância após preparação/carga. Instâncias distintas possuem pesos, RNG e memória CUDA próprios; operações concorrentes na mesma instância não são suportadas.

## Modelo de texto

```ghost
function void main() {
    print(device_info());
    textmodel ia = text_model(64, 32, 192, 32, 4096, 42);
    ia.prepare("treino.txt", "validacao.txt");
    double erro = ia.epoch(0.001);
    print("CE treino: " + erro);
    print("CE validacao: " + ia.validate());
    ia.save("modelo.bin");
    print(ia.generate("Usuário: Olá!\nAssistente:", 80));
}
```

| Método | Função |
|---|---|
| `prepare(treino, validacao, checkpoint="")` | Cria pesos/vocabulário ou retoma um checkpoint e carrega os corpus |
| `epoch(taxa)` | Treina uma época; retorna CE média dos lotes durante o treino |
| `validate()` | CE no conjunto de validação carregado |
| `accuracy()` | Acurácia de próximo token da última avaliação |
| `evaluate(arquivo)` | Avalia um corpus sem treinar |
| `epochs()` | Total de épocas concluídas |
| `save(arquivo)` / `load(arquivo)` | Checkpoint com parâmetros, AdamW, RNG e arquitetura |
| `generate(prompt, limite=100)` | Gera uma continuação pelos pesos |
| `chat()` | Interface neural simples do runtime |
| `info()` / `backend()` | Arquitetura / dispositivo da instância |
| `self_test()` | Diferenças finitas e, em CUDA, comparação CPU/GPU; não modifica o modelo |

O construtor recebe `(contexto, embedding, oculta, lote, vocabulario_maximo, seed)`; os padrões são `(64,32,128,32,2048,42)`. Os pesos são **float32 reais**, não `GhostNumberG`. A rede usa embeddings concatenados, tanh, softmax, entropia cruzada, AdamW e clipping global de gradiente em 1.0. O tokenizer é de palavras e pontuação UTF-8, sensível a maiúsculas. Não é BPE e palavras novas tornam-se `<UNK>`.

Pesos e momentos permanecem na GPU durante o treino. O backend CUDA reúne kernels para embeddings, camadas, softmax, retropropagação e AdamW. Entradas, probabilidades e métricas ainda atravessam CPU/GPU; é uma implementação pequena, sem cuBLAS/Tensor Cores. Não há promessa de aceleração para qualquer tamanho de problema.

Geração: `GHOST_TEXT_TEMP` (0–2; zero = escolha determinística), `GHOST_TEXT_TOPK`, `GHOST_TEXT_TOPP`, `GHOST_TEXT_REPETITION`, `GHOST_TEXT_NO_REPEAT`, `GHOST_TEXT_GENERATION_SEED`. A semente reinicia por chamada. `GHOST_TEXT_HISTORY=1` ativa contexto entre turnos somente em `ia.chat()`; a interface própria V3 é sem histórico. Esses parâmetros não garantem veracidade.

O formato binário mantém o identificador `GHOSTV2` por compatibilidade. V1 não é compatível. Carregar V2 no runtime nativo não adiciona conhecimento nem muda automaticamente sua arquitetura/vocabulário.

## Matrizes G

O operador de produto de `GhostMatrix`, usado por `gate`, `state` e produtos de `network`, pode executar um kernel em blocos 16×16 que preserva as regras atuais de x/y/p. Os buffers CUDA são reutilizados por thread. Em `auto`, só produtos com pelo menos 262144 combinações linha×coluna×dimensão interna são enviados à GPU; em `cuda`, qualquer produto não vazio usa GPU.

Empacotamento, transferência e resultados continuam passando pela CPU. Demais operações de matrizes e o otimizador das redes antigas não foram integralmente portados. Veja [as regras G verificadas](NUMEROS_G_IMPLEMENTACAO.md).


## Streaming de texto (V3.2)

`ia.stream(prompt, max_tokens=100, delay_ms=-1, prefix="")` emite os trechos à medida que os tokens são previstos, retorna o texto completo e termina a saída com nova linha. `text_stream(texto, delay_ms=-1)` exibe um texto já pronto gradualmente. Os bytes de cada caractere UTF-8 são mantidos juntos.

O atraso aceita 0–100 ms por caractere. `-1` usa `GHOST_STREAM_MS`, ou 12 ms quando stdout é terminal e zero quando redirecionado. `text_stream_delay()` informa o padrão. A pausa controla somente a exibição; não muda pesos nem seleção de tokens. Avisos de vocabulário precedem o prefixo da resposta. `ia.chat()` conserva a interface antiga em bloco.

O filtro lexical de cobertura do chat V3.2 é implementado na aplicação `.g`; não é aplicado por esses métodos. Veja [o guia do chat](../ia_propria/v3/STREAMING.md).
