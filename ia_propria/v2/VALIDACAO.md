# Validação da V2 — 25/09/2026

Esta versão executou treino e inferência na NVIDIA GeForce RTX 4050 Laptop, preservando os fontes originais do GhostLang. A implementação da IA está em `ia_propria/v2/` e em seus scripts locais de compilação/execução.

## Configuração entregue

- 371.269 parâmetros float32; contexto de 64 tokens.
- Vocabulário de 677 tokens de palavras/pontuação; embeddings de 32 dimensões; camada oculta tanh de 128 unidades.
- Lotes de 32, taxa 0,001, AdamW e clipping de norma global em 1.
- Perguntas usadas como contexto; loss apenas nas respostas do assistente e no fim de documento.
- 120 conversas de treino, 60 de validação: 2.789 e 1.395 alvos supervisionados, respectivamente.
- Corpus sintético com 60 respostas distintas. As perguntas mudam entre os conjuntos, mas as respostas de validação já existem no treino.

O treinamento executou 100 épocas. O melhor checkpoint, `checkpoints/v2/dialogo.bin`, foi selecionado pela CE de validação na época 96. O estado da época 100, com momentos Adam e RNG, está em `dialogo.bin.ultimo`. O CSV completo está em `dialogo.bin.metricas.csv`.

## Métricas de próximo token

| Medida de validação | Valor aproximado |
|---|---:|
| CE inicial | 6,52186 |
| Melhor CE | 0,406351 |
| Perplexidade no melhor checkpoint | 1,50133 |
| Acurácia de próximo token no melhor checkpoint | 91,33% |

Essas métricas fornecem ao modelo os tokens anteriores da resposta correta. Com respostas compartilhadas entre treino e validação, valores bons podem esconder dificuldade para escolher uma resposta inteira a partir da pergunta.

## Respostas completas, sem fornecer a resposta correta

Comando: `python3 v2/avaliar_conversas.py checkpoints/v2/dialogo.bin`. Temperatura zero, histórico entre perguntas desligado e filtros padrão de geração.

| Perguntas | Respostas exatamente iguais ao exemplo |
|---|---:|
| Conhecidas do treino | 114/120 — 95,0% |
| Reformulações separadas para validação | 5/60 — 8,3% |

O relatório completo com pergunta, resposta esperada e resposta gerada está em `logs/v2/conversas_avaliadas.json`. Correspondência exata pode penalizar diferenças legítimas, mas a inspeção também encontrou respostas que misturam temas ou não respondem à pergunta. Portanto, os números de próximo token **não demonstram capacidade de conversação geral**. A generalização continua fraca.

Perguntas do treino como `Como organizar meu dia?` e `Como uma rede neural aprende?` passaram a produzir respostas completas e pertinentes. Perguntas fora do corpus, como uma pergunta factual sobre um país, ainda podem receber respostas sem relação com o assunto. Não foi acrescentada busca de respostas prontas para ocultar esse comportamento; a saída vem dos parâmetros do modelo.

As amostras finais com a temperatura padrão estão em `logs/v2/amostras_finais.json`, e um chat executado está em `logs/v2/chat_final.txt`. Arquivos de amostras anteriores nessa pasta documentam estágios intermediários e não devem ser confundidos com o resultado final.

## Verificações técnicas aprovadas

- Gradientes CPU por diferenças finitas em embeddings, pesos e biases.
- Comparação CPU/CUDA de loss, gradientes e atualização AdamW.
- Treinamento controlado em ambos os dispositivos: CE de validação 2,628600 → 1,540700.
- Melhor checkpoint corresponde à menor CE observada, incluindo recarga dos parâmetros.
- Retomada preserva pesos, momentos Adam, passo, épocas e RNG: igualdade binária em CPU no mesmo ambiente e equivalência numérica em GPU.
- Carregamento de checkpoint CUDA em CPU.
- Auto seleciona GPU funcional e faz fallback CPU quando CUDA fica invisível; GPU forçada retorna erro se indisponível.
- UTF-8, comandos de chat, encerramento por EOF, geração reprodutível e tokens reservados.
- Rejeição de checkpoint corrompido/truncado, dados de validação duplicados, números inválidos e arquivos inexistentes; checkpoints existentes preservados.
- Sintaxe dos scripts Bash.

O filtro de repetição foi corrigido durante a validação para olhar apenas os tokens da resposta gerada. Considerar também os tokens da pergunta impedia saudações legítimas. O chat usa perguntas independentes por padrão porque esse é o formato do corpus fornecido.

## Conclusão técnica

A V2 oferece uma arquitetura maior, treinamento conversacional real, uso de GPU, checkpoints completos e ferramentas para medir qualidade. O corpus ainda é uma demonstração pequena e sintética. Para avançar rumo a conversação geral, é necessário ampliar a diversidade dos dados e avaliar novos assuntos e formulações, além de evoluir tokenização e arquitetura. Aumentar apenas as épocas desse mesmo corpus não resolve a memorização observada.
