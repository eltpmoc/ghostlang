# IA própria em GhostLang

Está disponível a [V4 experimental, com Transformer nativo por bytes](v4/README.md), com suporte a CPU/CUDA. Ela ainda falha no diagnóstico original e não substitui a V3 estável. Para experimentar: `bash scripts/ia_v4.sh chat`, dentro desta pasta.

A versão atual é a **V3.4**, com explicações ampliadas, exemplos GhostLang compilados e continuidade de assunto no modo fontes. O tipo `textmodel` e CUDA continuam integrados ao runtime nativo. O chat pesquisa automaticamente temas públicos sem cobertura local e guarda referências para próximas sessões, inclusive sem internet. Veja [pesquisa automática e memória](v3/MEMORIA_AUTONOMA.md).

```bash
cd ia_propria
bash scripts/ia_v3.sh chat
```

Experimente: **Descreva o que é GhostLang e crie um exemplo.** A resposta inclui código completo, saída esperada e instruções para executar. Depois pode pedir "agora mostre um exemplo" sobre o último assunto reconhecido. `/limpar` limpa esse assunto; `/neural` ativa geração pelos pesos e `/fontes` retorna à consulta.

O novo modelo possui 650.808 parâmetros e foi treinado na RTX 4050. A geração livre ainda é limitada: 13/89 respostas completas corretas na avaliação, contra 11/89 antes. Exemplos documentais revisados e geração neural são identificados separadamente.

Pergunte normalmente: “Quem é o presidente da França?” ou “O que é computação quântica?”. O assistente seleciona o tema, pesquisa se necessário e salva a referência. `/memoria` lista o conhecimento adquirido; `/internet off` mantém o uso local. Os pesos não mudaram nesta atualização.

Streaming: `/stream`, `/velocidade 20`, `/stream off` e `/stream on`. Veja [controles e limites da V3.2](v3/STREAMING.md).

Consulte o [guia V3.1](v3/ATUALIZACAO_V31.md), a [referência da aplicação](v3/README.md) e a [API nativa CUDA/textmodel](../docs/CUDA_TEXTMODEL.md). Para recompilar: `bash scripts/compilar_v3.sh`. Checkpoints anteriores foram preservados; a nova versão usa `checkpoints/v3/ghost_g_v31.bin`. O estado anterior da aplicação está em `backups/`.

Para medir capacidades, execute `python3 v3/testar_capacidades.py` dentro de `ia_propria`. Veja o [diagnóstico e seus limites](v3/TESTE_CAPACIDADES.md).

---

# Referência histórica da V1

Esta seção registra a arquitetura anterior, com backend restrito à aplicação. Afirmações abaixo sobre o runtime original, o parser e o compilador descrevem a situação anterior à V3. A API e as correções atuais estão no guia nativo acima.

Modelo experimental de texto treinado do zero, com **6.794 parâmetros**, escrito em GhostLang. Aprende a prever o próximo caractere e gera texto recursivamente. O fluxo de treinamento, inferência, normalização, avaliação e persistência está nos arquivos `.g`. As operações neurais usam o runtime nativo em CPU ou o backend CUDA próprio em `backend/`, conforme a detecção automática. Não baixa modelos nem usa o Llama/GGUF existente.

É uma base funcional para desenvolver uma IA própria. O contexto de quatro caracteres e o corpus demonstrativo são pequenos: a saída pode repetir palavras ou não fazer sentido. Esta versão ainda não é um assistente de conversação nem um Transformer.

## Começar

Na raiz do GhostLang:

```bash
cd ia_propria
bash scripts/compilar.sh
bash scripts/ia.sh treinar dados/treino.txt dados/validacao.txt 12 0.003 checkpoints/meu_modelo.txt
bash scripts/ia.sh avaliar dados/validacao.txt checkpoints/meu_modelo.txt
bash scripts/ia.sh gerar checkpoints/meu_modelo.txt "ola " 120
```

Dependências de execução/compilação: Bash, compilador C++17 com OpenMP (g++) e os fontes do GhostLang presentes no diretório pai. Não requer Python para usar a IA. O CUDA é opcional: quando o toolkit está instalado, o script também compila `build/ia_cuda`; caso contrário, mantém a CPU. Todos os caminhos relativos dos comandos são resolvidos dentro de `ia_propria/`; caminhos absolutos também funcionam para corpus e checkpoints. Use nomes novos para cada treinamento: um checkpoint existente é preservado.

Para continuar aprendendo a partir dos pesos salvos:

```bash
bash scripts/ia.sh continuar dados/treino.txt dados/validacao.txt 6 0.001 checkpoints/meu_modelo.txt checkpoints/meu_modelo_v2.txt
```

A continuação carrega pesos e biases, mas reinicia os momentos do Adam e a sequência aleatória. Não equivale à retomada exata de um processo interrompido.

### Windows (PowerShell): V5

A V5 tem lançador e compilação próprios para Windows, sem Bash:

```powershell
cd ia_propria
powershell -ExecutionPolicy Bypass -File scripts\compilar_v5.ps1         # compila com Visual Studio (MSVC) + CUDA em ..\build_win_v5
powershell -ExecutionPolicy Bypass -File scripts\ia_v5.ps1               # abre o assistente (GHOST_DEVICE=auto)
powershell -ExecutionPolicy Bypass -File scripts\ia_v5.ps1 --device cpu  # força CPU (cuda/gpu se houver runtime CUDA)
```

O lançador procura `build\v5\ia.exe`, define `OMP_NUM_THREADS=4` se não houver valor e aceita caminhos relativos à pasta em que você estava. Se o runtime nativo foi compilado sem CUDA (`..\build_native\ghost-runtime.conf`), `--device cuda` volta para CPU com um aviso.

## Organização

| Caminho | Conteúdo |
|---|---|
| `src/modelo.g` | Rede, tokenização, treinamento, geração, métricas e checkpoint |
| `src/main.g` | Interface de comandos e validação dos argumentos |
| `dados/` | Textos distintos de treino e validação, escritos para esta demonstração |
| `scripts/` | Compilação Linux e seleção automática CPU/GPU |
| `backend/` | Ponte C++ e kernels CUDA específicos desta IA |
| `tests/verificar.py` | Testes externos com a biblioteca padrão do Python |
| `docs/recursos_nativos.md` | Capacidades verificadas e limites desta versão |
| `build/` | Executáveis locais e C++ gerado automaticamente |
| `checkpoints/` | Pesos próprios e métricas CSV por treinamento |
| `logs/` | Local para guardar saídas e medições |

## Arquitetura e aprendizagem

Quatro caracteres são codificados como quatro vetores one-hot de 42 posições. O vetor de entrada tem 168 elementos. Uma camada de 32 neurônios com ReLU alimenta uma saída linear de 42 valores. A rede declara `gate`, `state`, `network` e `>>` em GhostLang. Em CPU, utiliza multiplicação matricial e `optimize(..., opt="adam")` nativos. Em GPU, a ponte chama kernels CUDA equivalentes de forward, backpropagation e Adam. Os pesos e momentos permanecem na GPU durante o treino; os pesos são sincronizados antes de salvar.

O otimizador nativo minimiza erro quadrático contra o próximo caractere codificado em one-hot. Reportamos MSE médio por componente e acurácia de próximo caractere; não reportamos perplexidade ou probabilidades calibradas. A geração escolhe o maior valor da saída (argmax), acrescenta o caractere e repete. A janela não mantém memória de uma conversa.

O vocabulário contém espaço, `a-z`, `. , ! ?`, `0-9` e quebra de linha. A normalização remove acentos portugueses comuns, converte maiúsculas e troca bytes não suportados por espaços. A saída é minúscula e sem acentos. A função `encode` nativa só cria vetores inicialmente zerados; o código `.g` preenche o one-hot explicitamente. Os componentes imaginários e fantasmas permanecem zerados neste modelo.

Os exemplos são embaralhados a cada época com Fisher-Yates. A semente é 42, reproduzível no mesmo ambiente e implementação de `rand()`. O checkpoint inclui pesos e biases com 17 casas decimais, cabeçalho de versão e dimensões; o carregamento verifica contagem e valores. A gravação usa arquivo temporário e renomeação. O CSV `<checkpoint>.metricas.csv` registra uma linha por época. A versão atual salva ao concluir todas as épocas.

## Usar seus textos

Crie arquivos UTF-8 separados, por exemplo `dados/meu_treino.txt` e `dados/minha_validacao.txt`. Reserve textos distintos para avaliar generalização. Os exemplos fornecidos verificam funcionamento; não são um conjunto representativo de linguagem.

Nesta primeira versão cada corpus aceita de 5 a 20.000 caracteres após normalização e até 1 MB antes dela. Comece com algumas centenas de caracteres, 12 épocas e taxa `0.003`. É permitido usar 1–200 épocas e taxa entre `0.000001` e `0.05`. Mais dados/épocas aumentam o trabalho em CPU. Avalie tanto o MSE de validação quanto a qualidade da saída; uma redução do erro de treino isoladamente pode indicar memorização.

## Detecção automática de CPU/GPU

```bash
bash scripts/ia.sh --hardware
bash scripts/ia.sh gerar checkpoints/demonstracao_cuda.txt "ola " 120
bash scripts/ia.sh --device cpu gerar checkpoints/demonstracao_cuda.txt "ola " 120
bash scripts/ia.sh --device gpu gerar checkpoints/demonstracao_cuda.txt "ola " 120
```

O padrão `auto` tenta CUDA quando o backend está compilado, verifica o dispositivo visível, aloca memória e executa um kernel de autoteste. Se a inicialização falhar, usa CPU e informa o motivo. `--device gpu` exige GPU e retorna erro quando não pode usá-la; `--device cpu` não inicializa CUDA. Também é possível definir `IA_DEVICE=auto|cpu|gpu`. Falhas de CUDA durante o cálculo interrompem a execução, para não continuar com pesos desatualizados.

A RTX 4050 Laptop foi reconhecida e o modelo já foi treinado nela. `nvidia-smi` informa 6.141 MiB físicos e driver 595.91.07; o runtime CUDA informa 5.772 MiB disponíveis ao dispositivo. O problema anterior de NVML não ocorreu nesta nova verificação. O processador identificado é o i7-13620H com 16 processadores lógicos. A GPU Intel não é suportada pelo backend CUDA; sem NVIDIA utilizável, a IA usa CPU.

O backend é um acréscimo **somente em `ia_propria/`**. O runtime original do GhostLang permanece intacto e seu treinamento nativo continua em CPU. A diretiva `pragma gpu` da linguagem não é responsável por esta integração.

O lançador mantém `OMP_NUM_THREADS=1` em CPU, adequado às matrizes pequenas. Para comparar outros valores, use `OMP_NUM_THREADS=4 bash scripts/ia.sh --device cpu avaliar ...`. Seleção automática significa preferência por GPU funcional, não escolha por benchmark; não há garantia de que CUDA será mais rápido em todos os tamanhos de entrada.

A compilação padrão é `IA_BUILD=auto`. `IA_BUILD=cpu bash scripts/compilar.sh` gera somente o caminho CPU ativo; `IA_BUILD=gpu bash scripts/compilar.sh` exige que o backend CUDA compile. Nenhum pacote é instalado. O toolkit é procurado no PATH e em `/usr/local/cuda/bin/nvcc`. São aceitos `NVCC`, `CUDA_HOME` e `IA_CUDA_ARCH` para personalização. Por padrão, CUDA compila para a arquitetura da GPU local (`native`); recompile se mudar de computador. `CUDA_VISIBLE_DEVICES` controla os dispositivos visíveis, e o backend usa o primeiro deles.

Detalhes e resultados: [backend CUDA](docs/cuda.md).

## Verificar

Depois de compilar:

```bash
python3 tests/verificar.py
IA_TEST_DEVICE=gpu python3 tests/verificar.py
python3 tests/backends.py
```

Os dois últimos comandos exigem o backend CUDA compilado e GPU funcional. Os testes de backends verificam seleção, fallback e equivalência numérica entre CPU e GPU. Os testes de aprendizado treinam um corpus controlado, exigem queda de erro, comparam métricas antes/depois de carregar os pesos, verificam geração, normalização, reprodução da semente, continuação e rejeição de entradas/checkpoints inválidos. Usam uma pasta temporária dentro de `build/` e não substituem seus checkpoints.

## Isolamento e compilação

Nenhum fonte, header, script ou configuração original do GhostLang precisa ser editado. O script compila uma cópia do compilador em `ia_propria/build/ghost` usando os fontes originais. Depois ele transpila `src/main.g` normalmente.

O adaptador `scripts/toolchain/g++` acrescenta o arquivo original `src/GhostNumberG.cpp` à ligação, os caminhos de inclusão e a ponte local `backend/ponte.hpp`. Na variante CUDA, também liga o objeto dos kernels e o runtime estático CUDA. Isso contorna uma omissão no comando de compilação atual sem mudar o compilador ou o código gerado. Verificamos o executável novo porque o compilador pode retornar sucesso mesmo quando g++ falha. O compilador original emite avisos de C++ já existentes, que podem aparecer durante a compilação.

Os checkpoints de texto contornam a serialização binária nativa de objetos `GhostNumberG`, que contêm um `std::vector` interno. Os operadores lógicos são calculados em variáveis antes de `if`, pois o parser atual separa `&&` e `||` nas condições. Esses ajustes ficam exclusivamente no projeto da IA.

Para evoluir, os próximos trabalhos são ampliar contexto/vocabulário, implementar uma loss de classificação e amostragem adequadas, adicionar estado completo do otimizador e estudar atenção/embeddings. O backend CUDA atual atende somente à arquitetura 168 → 32 → 42 e aos parâmetros reais; mudanças de arquitetura exigem adaptar também os kernels e a ponte. Consulte o inventário em `docs/recursos_nativos.md` antes de planejar essa etapa.
