# IA própria V3.4

Atualização atual: [pesquisa automática e memória persistente](MEMORIA_AUTONOMA.md). O [guia da V3.3](INTERNET_CONTEXTO.md) registra a versão anterior. O [streaming](STREAMING.md) continua disponível. As [explicações, exemplos e resultados da V3.1](ATUALIZACAO_V31.md) continuam disponíveis.

A V3 define o modelo e seu treinamento em **GhostLang**, pelo tipo nativo `textmodel`. O runtime da linguagem implementa os cálculos em C++/CUDA. Não há dependência do backend da aplicação V2, Python no chat, modelos baixados, serviços externos de LLM ou PyTorch. O assistente consulta fontes públicas por HTTPS quando necessário e armazena referências com fonte e data.

O modelo neural continua experimental: embeddings, contexto fixo, camada tanh, softmax e AdamW, com pesos reais float32. Não é Transformer, não usa pesos G e não possui conhecimento geral amplo. Aprende sobre GhostLang e Números G pelos exemplos do corpus; conhecer a teoria como conteúdo é diferente de usar sua álgebra nos pesos.

## Executar

Dentro de `ia_propria`:

```bash
bash scripts/compilar_v3.sh
bash scripts/ia_v3.sh hardware
bash scripts/ia_v3.sh autoteste
bash scripts/ia_v3.sh chat
bash scripts/ia_v3.sh consultar "O que é a Teoria dos Números Fantasmas G?"
bash scripts/ia_v3.sh --device cpu chat
bash scripts/ia_v3.sh --device cuda chat
```

O chat inicia no modo **assistente** (`/fontes` é um sinônimo): busca lexical conservadora, normalização de maiúsculas/acentos, limiar de correspondência e recusa em casos ambíguos. Respostas são revisadas e mostram o arquivo de origem. Essa busca é implementada em GhostLang, independentemente dos pesos neurais. Ela atende pedidos de explicação com exemplo e duas consultas reconhecidas ligadas por "e", mas não interpreta arbitrariamente qualquer instrução, pode pedir reformulação e consulta automaticamente a memória adquirida e fontes públicas para temas sem cobertura local. Exemplos revisados incluem código, resultado esperado e comando de execução.

O chat exibe texto progressivamente: `/velocidade 20` define 20 ms de pausa por caractere; `/stream off` mostra a resposta inteira; `/stream on` reativa. O padrão no terminal é 12 ms.

Use `/neural` para gerar, após passar pelo filtro de cobertura local, pelos pesos de `checkpoints/v3/ghost_g_v31.bin`, `/fontes` para retornar à consulta e `/sair` para encerrar. Perguntas sem correspondência suficiente na base passam pelo seletor de pesquisa pública e memória antes de uma eventual recusa. Esse filtro pode recusar reformulações válidas e não garante veracidade: a geração neural ainda pode repetir ou inventar informações. Nenhuma mensagem do chat modifica os pesos. O histórico não é salvo automaticamente; referências públicas consultadas são gravadas em `v3/memoria`, com tema, fonte e data. `/memoria` lista os registros e `/internet off` bloqueia novas conexões. No modo fontes, pedidos como "agora mostre um exemplo" retomam a última ficha reconhecida. `/limpar` apaga o contexto; trocar de modo preserva o assunto. Uma nova pergunta de conteúdo substitui o assunto anterior; não é memória completa de conversa. Saudações, identidade e esclarecimentos são tratados pelo controle local também no modo neural, com rótulo próprio. O prompt da rede continua contendo apenas a pergunta atual.

## Treinar ou continuar

```bash
bash scripts/ia_v3.sh treinar v3/dados/treino.txt v3/dados/validacao.txt 30 0.001 checkpoints/v3/meu_modelo.bin
bash scripts/ia_v3.sh continuar v3/dados/treino.txt v3/dados/validacao.txt 10 0.0005 checkpoints/v3/meu_modelo.bin.ultimo checkpoints/v3/continuado.bin
bash scripts/ia_v3.sh chat checkpoints/v3/meu_modelo.bin
bash scripts/ia_v3.sh avaliar v3/dados/validacao.txt checkpoints/v3/meu_modelo.bin
bash scripts/ia_v3.sh gerar checkpoints/v3/meu_modelo.bin "Usuário: O que é GhostLang?
Assistente:" 100
```

O programa recusa destinos existentes. Salva o melhor checkpoint por entropia cruzada de validação, o último estado em `.ultimo`, métricas em `.metricas.csv` e identificação do treino em `.treino.txt`. Interrompe após oito épocas consecutivas sem melhora. O checkpoint guarda pesos, otimizador e RNG; o formato é compatível com o backend V2, mas um vocabulário carregado não aumenta ao continuar o treino. Use treino novo para ampliar o vocabulário.

A arquitetura está declarada em `v3/src/main.g`. O construtor aceita contexto 4–128, embedding 4–64, oculta 8–256, lote 1–128 e vocabulário 8–4096. O corpus usa `Usuário:` e `Assistente:`, com linha vazia entre exemplos; o treinamento supervisiona a resposta. Também aceita documentos de texto livre. Limite de 32 MiB por corpus.

## Conhecimento e avaliação

`dados/conhecimento.json` contém fichas locais sobre linguagem, CUDA e Números G. `preparar_dados.py` confere as âncoras das fontes, gera treino/validação e o código da consulta, registrando caminhos, linhas e hashes em `dados/fontes.json`. Após editar fichas ou fontes, recompile para atualizar a consulta. Para atualizar o conhecimento nos pesos, treine novamente.

A base inicial inclui exemplos gerais em português da V2. Duas formulações de cada ficha vão ao treino e uma à validação; **as respostas são compartilhadas**. Portanto, a validação mede novas formulações de fatos conhecidos, não aquisição de fatos inéditos. Acerto documental e qualidade neural devem ser avaliados separadamente. As inconsistências matemáticas estão registradas em `../../docs/NUMEROS_G_IMPLEMENTACAO.md`.

Resultados do checkpoint inicial e testes: [VALIDACAO.md](VALIDACAO.md). Para repetir: `python3 v3/avaliar.py` e `python3 v3/verificar.py`, dentro de `ia_propria`.

O corpus ampliado da V3.1 é gerado com `python3 v3/preparar_instrucoes.py` em `dados/instrucoes_treino.txt` e `dados/instrucoes_validacao.txt`. O corpus original permanece disponível. `avaliar.py` grava `AVALIACAO_ATUAL.json`, preservando o relatório histórico. Os exemplos executáveis são verificados por `python3 v3/testar_respostas.py`.

## Testar capacidades e generalização

Execute `python3 ia_propria/v3/testar_capacidades.py` na raiz do projeto. O [guia do diagnóstico](TESTE_CAPACIDADES.md) separa tarefas novas, controles conhecidos, geração neural e uso de ferramentas, com respostas completas para revisão.
