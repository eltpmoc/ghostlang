# Ghost Conversa V2

Modelo próprio de linguagem, treinado do zero, com interface e controle do treinamento em GhostLang e extensão numérica local C++/CUDA. O GhostLang original e a V1 permanecem intactos. A V2 usa palavras e pontuação para produzir frases em português, com acentos.

O modelo demonstrativo tem **371.269 parâmetros**, **64 tokens de contexto**, embeddings de 32 dimensões, camada oculta de 128 unidades e vocabulário de 677 tokens. É uma rede de contexto fixo com embeddings e tanh, não um Transformer. Continua experimental e não é um assistente geral confiável.

## Conversar agora

Da raiz do GhostLang:

```bash
cd ia_propria
bash scripts/ia_v2.sh chat checkpoints/v2/dialogo.bin
```

Experimente `Olá!`, `Como organizar meu dia?` ou `Como uma rede neural aprende?`. Digite `/sair` para encerrar; Ctrl+D também encerra. `/limpar` limpa o histórico opcional.

O dispositivo é escolhido automaticamente: CUDA quando compilado e funcional, CPU caso contrário. Para forçar CPU:

```bash
bash scripts/ia_v2.sh --device cpu chat checkpoints/v2/dialogo.bin
```

Por padrão cada pergunta é independente, pois o corpus demonstrativo contém diálogos de um turno. A janela de 64 tokens é usada dentro da pergunta e durante a geração. O modo `V2_HISTORY=1` mantém falas anteriores, mas só deve ser avaliado depois de treinar com conversas de múltiplos turnos; o checkpoint fornecido não foi validado para isso.

## O que mudou

| Recurso | V1 | V2 |
|---|---|---|
| Unidade de texto | Caractere ASCII normalizado | Palavra/pontuação UTF-8 |
| Contexto padrão | 4 caracteres | 64 tokens |
| Parâmetros da demonstração | 6.794 | 371.269 |
| Representação | One-hot | Embeddings treináveis |
| Objetivo | Erro quadrático | Softmax e entropia cruzada |
| Diálogos | Sem supervisão por papel | Pergunta como contexto, loss nos tokens do assistente |
| Otimizador | Adam por exemplo | AdamW em lotes, clipping de gradiente |
| Geração | Argmax | Temperatura, top-k, top-p, penalidade e bloqueio de n-gramas |
| Persistência | Pesos e biases | Pesos, momentos Adam, passo, épocas, RNG, dimensões e vocabulário |
| Escolha de pesos | Última época | Melhor validação + último estado para retomar |
| Avaliação | MSE e acurácia | CE, perplexidade, acurácia e respostas completas |
| Interface | Geração por comando | Geração e chat de terminal |

A V2 não usa pesos do Llama nem serviços externos. A extensão numérica é necessária porque o runtime nativo atual não fornece esse conjunto de embeddings, loss e treinamento em lotes. O fluxo em `src/main.g` chama essa extensão para executar cada época, avaliar, selecionar checkpoints e oferecer os comandos. A implementação numérica e a tokenização ficam em `backend/`.

## Compilar

```bash
bash scripts/compilar_v2.sh
bash scripts/ia_v2.sh hardware
```

Requer g++ C++17 e Bash. CUDA é opcional; o script procura nvcc no PATH e em `/usr/local/cuda/bin/nvcc`. `IA_BUILD=cpu` evita a compilação CUDA; `IA_BUILD=gpu` exige que ela funcione. Também são aceitos `NVCC`, `CUDA_HOME`, `GHOST_CXX` e `IA_CUDA_ARCH` (padrão `native`). Recompile ao mudar de arquitetura de GPU.

Os binários ficam em `build/v2/`. O código GhostLang é transpilado pelo compilador original, construído sem modificações nessa mesma pasta. O adaptador de compilação inclui a API da V2 e liga o backend da aplicação. Nenhum pacote ou driver é instalado.

## Treinar seus dados

Use dois arquivos UTF-8 distintos, separando conversas/documentos por uma linha vazia:

```text
Usuário: Como organizar meu dia?
Assistente: Liste suas tarefas e escolha o que é mais importante.

Usuário: Como estudar melhor?
Assistente: Divida o conteúdo em partes e pratique com exercícios.
```

Os marcadores `Usuário:` e `Assistente:` diferenciam os papéis. Quando existe `Assistente:`, só os tokens da resposta e o fim de documento geram loss. O texto da pergunta permanece no contexto. Arquivos sem esse marcador são tratados como texto comum e todos os tokens são alvos.

```bash
bash scripts/ia_v2.sh treinar meus_dados/treino.txt meus_dados/validacao.txt 60 0.001 checkpoints/v2/meu_modelo.bin
```

São aceitos até 32 MiB por arquivo e 4 milhões de posições supervisionadas por corpus. O limite de épocas por comando é 200. O vocabulário é criado apenas a partir do treino; tokens ausentes viram `<UNK>`. Documentos de validação idênticos aos do treino, após tokenização, são rejeitados. Isso não substitui uma divisão cuidadosa por origem e assunto para evitar vazamento semântico.

O corpus demonstrativo é gerado a partir de `v2/dados/conversas.json`:

```bash
python3 v2/preparar_dados.py
```

Esse comando recria `v2/dados/treino.txt` e `validacao.txt`. São 60 respostas sintéticas, cada uma com três formulações de pergunta: duas no treino e uma na validação. A validação mede novas formulações para respostas conhecidas; **não mede aquisição de novos conhecimentos ou competência de conversação geral**. Os arquivos são pequenos e não substituem um corpus real, diverso e revisado.

## Checkpoints e retomada

Para um destino `modelo.bin`, o treinamento produz:

- `modelo.bin`: estado com menor entropia cruzada de validação, incluindo o estado inicial se nenhum treino melhorar.
- `modelo.bin.ultimo`: estado completo ao final da última época concluída.
- `modelo.bin.metricas.csv`: métricas por época da execução atual.

```bash
bash scripts/ia_v2.sh continuar meus_dados/treino.txt meus_dados/validacao.txt 20 0.0005 checkpoints/v2/meu_modelo.bin.ultimo checkpoints/v2/continuado.bin
```

Use um destino novo; arquivos existentes são preservados. Para reproduzir um treino interrompido na fronteira de época, mantenha os mesmos dados, taxa e ambiente e retome `.ultimo`. O checkpoint contém os momentos de Adam e o estado do embaralhamento. As estatísticas de seleção do melhor modelo reiniciam em cada comando, usando a validação do checkpoint carregado como referência. Alterar taxa ou dados é um novo ajuste, não uma reprodução da execução anterior.

O arquivo tem versão, dimensões limitadas, vocabulário, arrays float32 explícitos e checksum FNV-1a para detectar corrupção acidental. Não serializa ponteiros ou objetos C++. O formato verifica endianness e foi testado em Linux x86-64. A gravação usa temporário e renomeação atômica. Checkpoints V1 não são compatíveis com V2.

O vocabulário e a arquitetura ficam fixos ao continuar. Se muitos termos novos ficarem desconhecidos, prepare um corpus mais amplo e treine um modelo novo para reconstruir o vocabulário.

## Configuração

As variáveis de arquitetura são usadas ao iniciar um modelo do zero. Ao carregar, valem as dimensões gravadas no checkpoint.

| Variável | Padrão | Intervalo |
|---|---:|---:|
| `V2_CONTEXT` | 64 | 4–128 tokens |
| `V2_EMBED` | 32 | 4–64 |
| `V2_HIDDEN` | 128 | 8–256 |
| `V2_VOCAB` | 2048 | 32–4096 tokens máximos |
| `V2_BATCH` | 32 | 1–128 |
| `V2_SEED` | 42 | 0–2147483647 |
| `V2_PATIENCE` | 8 | 1–200 épocas sem melhora |

O tamanho real do vocabulário depende dos dados e pode ser menor que `V2_VOCAB`. O clipping limita a norma global do gradiente a 1. AdamW usa beta1=0,9, beta2=0,999, epsilon=1e-8 e weight decay=0,0001; a taxa permanece constante dentro de cada comando.

Um perfil menor para experimentar em CPU:

```bash
V2_CONTEXT=32 V2_EMBED=16 V2_HIDDEN=64 bash scripts/ia_v2.sh --device cpu treinar v2/dados/treino.txt v2/dados/validacao.txt 20 0.001 checkpoints/v2/compacto.bin
```

Uma arquitetura maior pode ser configurada com contexto 128, embedding 64 e camada oculta 256. Isso aumenta custo e capacidade de memorização; não garante qualidade melhor sem mais dados. A V2 usa float32 e mantém parâmetros e momentos na GPU durante o treinamento.

## Geração

```bash
bash scripts/ia_v2.sh gerar checkpoints/v2/dialogo.bin $'Usuário: Como organizar meu dia?\nAssistente:' 100
```

O último argumento é o máximo de tokens novos, não de caracteres. A geração termina antes ao prever o fim do documento. O resultado impresso contém somente a continuação, e o tokenizer preserva acentos e caixa. O vocabulário é de palavras, sem BPE/subpalavras; palavras desconhecidas limitam a compreensão de perguntas novas.

| Variável | Padrão | Função |
|---|---:|---|
| `V2_TEMP` | 0.65 | Temperatura 0–2; 0 usa argmax |
| `V2_TOPK` | 20 | Mantém os k candidatos de maior probabilidade |
| `V2_TOPP` | 0.9 | Corta a distribuição por massa acumulada após top-k |
| `V2_REPETITION` | 1.15 | Divide a probabilidade dos tokens recentes da resposta por esse fator |
| `V2_NO_REPEAT` | 3 | Bloqueia n-gramas repetidos na resposta; 0 desliga |
| `V2_GENERATION_SEED` | 42 | Semente reproduzível da amostragem |
| `V2_HISTORY` | 0 | Histórico entre perguntas no chat; experimental |

Os filtros de repetição examinam a resposta gerada, permitindo repetir termos da pergunta. Não corrigem falta de conhecimento e podem modificar frases quando houver repetições legítimas. Para comparar respostas de forma determinística, use `V2_TEMP=0`.

## Avaliar

```bash
bash scripts/ia_v2.sh avaliar v2/dados/validacao.txt checkpoints/v2/dialogo.bin
python3 v2/avaliar_conversas.py checkpoints/v2/dialogo.bin
```

A avaliação numérica fornece a resposta correta anterior ao prever cada próximo token (teacher forcing). Ela não equivale à capacidade de gerar uma resposta inteira. O segundo comando avalia respostas completas das perguntas do corpus, sem fornecer os tokens da resposta, e salva um relatório JSON em `logs/v2/`. A correspondência exata é rigorosa e pode contar uma resposta equivalente como diferente; também não comprova competência fora do corpus.

Resultados e exemplos efetivamente observados estão em [VALIDACAO.md](VALIDACAO.md).

## Testes

```bash
bash scripts/ia_v2.sh --device cpu autoteste
bash scripts/ia_v2.sh --device gpu autoteste
IA_TEST_DEVICE=cpu python3 v2/tests/verificar.py
IA_TEST_DEVICE=gpu python3 v2/tests/verificar.py
```

Os testes verificam gradientes por diferenças finitas, equivalência CPU/CUDA de loss/gradientes/AdamW, queda de erro, melhor checkpoint, retomada completa, UTF-8, comandos do chat e EOF, preservação de arquivos e rejeição de arquivos corrompidos. CUDA também verifica carregamento em CPU e fallback quando não há dispositivo visível. Operações atômicas de redução em CUDA podem causar pequenas diferenças numéricas; não prometemos reprodução bit a bit entre dispositivos.

## Limites e próximos desenvolvimentos

Esta versão oferece uma base maior e um processo de desenvolvimento verificável. O corpus tem apenas 60 respostas de demonstração. O modelo pode memorizar essas respostas, misturar assuntos e responder incorretamente a perguntas novas. Não tem busca na internet, ferramentas externas nem conhecimento garantido de fatos gerais.

As próximas etapas de pesquisa são um corpus conversacional real com diversidade de fontes, avaliação independente por assunto, tokenizer de subpalavras e arquitetura com atenção. Melhorar essas partes é mais importante para conversação geral do que simplesmente aumentar épocas ou ocupar toda a VRAM.
