# Jogos de texto para treinar a IA própria

Os jogos geram problemas e gabaritos para supervisionar o Transformer existente.
O modelo responde pelos pesos, usando a execução nativa GhostLang/CPU/CUDA.
Python prepara os mundos, pontua as respostas e coordena os comandos. O gabarito
não é fornecido ao processo de inferência e não substitui a saída do modelo.

Este é um currículo de aprendizagem supervisionada, não treinamento por reforço
(RL). Jogar mede o que os pesos sabem; somente `treinar` atualiza os pesos com
entropia cruzada e AdamW nativos. Um placar melhor não demonstra, sozinho,
conversação geral ou capacidade de um LLM grande.

## Jogos e níveis

| Jogo | Nível 1 | Nível 2 | Nível 3 |
|---|---|---|---|
| Cópia | Código de 2 letras | Código de 4 letras | Código de 6 letras |
| Memória | Uma caixa | Três caixas | Caixas com atualização de valor |
| Regras | Uma associação | Escolher entre três regras | Aplicar duas regras em sequência |
| Caminho | Uma porta | Duas portas | Três portas |
| Ordem | Primeiro elemento | Posição indicada | Inverter e selecionar a posição |
| Contagem | Um movimento | Dois movimentos | Três movimentos |

Os gabaritos são calculados a partir do estado de cada jogo. Por exemplo, o jogo
de caminho percorre as portas; memória aplica as atualizações antes de consultar
a caixa. As respostas finais são curtas para caber no contexto de 384 bytes do
modelo atual. O professor não gera raciocínios intermediários para o modelo.

## Executar na raiz do projeto

Preparar o conjunto reproduzível (não sobrescreve dados diferentes):

```bash
python3 ia_propria/v4/jogos/jogos.py preparar
```

Observar seis desafios inéditos, com a resposta real e o gabarito separados:

```bash
python3 ia_propria/v4/jogos/jogos.py jogar --device cuda --nivel 1 --rodadas 6
```

Cada rodada mostra o enunciado, a resposta dos pesos, o gabarito e o acerto/erro.
O placar e as respostas ficam em `relatorios/partida_*.json`. `--seed` permite
reproduzir os desafios. `--nivel 2` e `--nivel 3` aumentam a dificuldade.

Treinar em três etapas cumulativas, preservando o checkpoint original:

```bash
python3 ia_propria/v4/jogos/jogos.py treinar --device cuda \
  --blocos-por-nivel 2 \
  --destino ia_propria/checkpoints/v4/jogos_experimento
```

Cada bloco tem 100 atualizações: o exemplo acima executa 600 atualizações no
total. A primeira etapa usa nível 1, a segunda níveis 1 e 2, a terceira os três
níveis. O treinamento usa `continuar` de `v4/src/main.g`, carrega os pesos e o
estado de AdamW e segue com o último estado da etapa anterior. O checkpoint
selecionado usa a menor perda de validação do último nível; nem sempre ele é o
último passo executado. O destino precisa ser novo, para preservar experimentos.

`--checkpoint CAMINHO` escolhe a origem do treino ou o modelo avaliado. `--taxa`
controla a taxa de aprendizagem (padrão 0.0003). O exemplo usa somente jogos;
isso pode prejudicar outras capacidades. O piloto não substitui o checkpoint
padrão do chat automaticamente.

Avaliar os pesos treinados:

```bash
python3 ia_propria/v4/jogos/jogos.py avaliar --device cuda \
  --checkpoint ia_propria/checkpoints/v4/jogos_experimento/selecionado.bin \
  --particao teste --por-grupo 3
python3 ia_propria/v4/jogos/jogos.py avaliar --device cuda \
  --checkpoint ia_propria/checkpoints/v4/jogos_experimento/selecionado.bin \
  --particao teste_formulacao --por-grupo 3
```

A avaliação chama `gerar`, que expõe a saída bruta do Transformer. Não passa pelo
chat, suas respostas locais, pesquisa na internet nem pelo filtro de repetições.
O limite é 32 bytes e a correção exige igualdade exata, removendo somente espaços
nas pontas. Respostas extras contam como erro. Falhas de execução são contadas à
parte, como erros na pontuação. O relatório inclui respostas completas, hashes
do checkpoint e dos dados, acertos por jogo/nível e uma referência simples que
responde sempre com a resposta mais frequente do treino naquele grupo.

## Separação de treino e avaliação

O conjunto padrão contém 2.160 problemas de treino, 216 de validação, 216 de
teste e 216 de teste com nova formulação. IDs do estado e prompts são disjuntos
entre as quatro partições. O teste habitual usa enunciados do mesmo estilo do
treino com estados novos. O teste de formulação usa um quarto enunciado, ausente
do treino. Assim, distinguimos problemas novos de variação na forma de perguntar.

As instruções têm templates, como enunciados de exercícios. Isso não é uma
resposta pronta no chat: os dados do problema mudam, os pesos precisam gerar a
resposta e o gabarito só é usado para treinar ou pontuar. O conjunto é pequeno e
sintético, com vocabulário limitado; não mede conhecimento geral ou diálogo livre.

`--por-grupo 3` avalia 54 casos (6 jogos × 3 níveis × 3 exemplos). Para os 216
casos, use `--por-grupo 12`. Não escolha os pesos pelo resultado do teste; use a
validação. Consultar repetidamente o mesmo teste para orientar mudanças também
reduz sua independência. Para um novo experimento, gere outra pasta com outro
`--seed`, mantendo os registros anteriores.

## Verificações

```bash
python3 ia_propria/v4/jogos/testar_jogos.py
```

Testa gabaritos conhecidos, limites do contexto, estados e prompts disjuntos,
formato binário nativo, currículo cumulativo, reprodução por semente, recusa de
sobrescrita e a fronteira de inferência (o modelo recebe apenas o prompt).


## Resultado do piloto executado

Foi executado um piloto de 600 atualizações na RTX 4050, partindo do checkpoint
V4 selecionado (passo 7000) e terminando no passo 7600. O arquivo novo está em
`ia_propria/checkpoints/v4/jogos_piloto_20260926/selecionado.bin`.

| Avaliação (mesmos problemas antes/depois) | Antes | Depois | Resposta mais frequente do treino |
|---|---:|---:|---:|
| Problemas inéditos, enunciado familiar | 2/54 | 8/54 | 5/54 |
| Problemas inéditos, nova formulação | 1/54 | 7/54 | 3/54 |

Sem falhas de execução. O ganho observado é pequeno e o desempenho absoluto
continua baixo: não demonstra competência geral nem fluência de conversa.
Não houve mais treino ou escolha de pesos baseada nesses resultados de teste.
O checkpoint padrão do chat permanece intacto.

Para observar os pesos desse piloto em desafios novos:

```bash
python3 ia_propria/v4/jogos/jogos.py jogar --device cuda --nivel 1 \
  --checkpoint ia_propria/checkpoints/v4/jogos_piloto_20260926/selecionado.bin
```

Relatórios: [comparação](relatorios/PILOTO.json),
[respostas antes](relatorios/antes_teste.json),
[respostas depois](relatorios/depois_teste.json),
[transferência de formulação](relatorios/depois_formulacao.json).


## Painel visual de progresso

O painel compara os mesmos desafios antes e depois, por habilidade e nível,
e permite inspecionar acertos novos, regressões e erros persistentes. Inclui
referência da resposta mais frequente do treino e uma visão separada de testes
com outra formulação. Partidas idênticas do mesmo checkpoint são agrupadas;
repetir a mesma semente não aumenta a evidência de aprendizagem.

Abrir no Ubuntu, a partir da raiz do projeto:

```bash
xdg-open ia_propria/v4/jogos/painel_progresso.html
```

O HTML funciona offline e pode ser aberto diretamente no navegador. É uma
fotografia dos relatórios, sem treinamento nem consultas automáticas. Para
atualizar depois de produzir novas avaliações:

```bash
python3 ia_propria/v4/jogos/painel.py
```

Os seletores permitem comparar checkpoints, conjuntos e níveis. Se as amostras
tiverem tamanhos diferentes, somente os casos exatamente em comum entram na
comparação. Conjuntos, métodos ou partições incompatíveis não são comparados.
O painel confere os gabaritos e contagens nos relatórios; arquivos inconsistentes
são listados como rejeitados. Os metadados incluem hashes dos modelos e dados.
A opção validação é identificada como conjunto usado para selecionar os pesos.

A avaliação foi ampliada de 54 para todos os 216 problemas de cada conjunto,
sem novo treino nem mudança de checkpoint:

| Conjunto completo | Antes | Depois | Referência simples |
|---|---:|---:|---:|
| Enunciado familiar | 7/216 | 33/216 | 15/216 |
| Nova formulação | 3/216 | 30/216 | 11/216 |

No conjunto familiar, foram 29 acertos novos e 3 regressões. O resultado mostra
um ganho limitado nesses exercícios; não comprova inteligência geral ou diálogo
fluente. Os resultados completos ficam em `relatorios/painel_*_216.json`.

Verificações do painel:

```bash
python3 ia_propria/v4/jogos/testar_painel.py
node ia_propria/v4/jogos/testar_painel_browser.mjs
```

O segundo teste é opcional e usa o Chrome instalado com um perfil temporário,
sem acessar o perfil pessoal. Confere contagens, filtros, exemplos, paginação,
comparação com pesos idênticos e layout móvel. `GHOST_CHROME` permite indicar
outro caminho de Chrome/Chromium compatível. O painel em si não depende de Node.
