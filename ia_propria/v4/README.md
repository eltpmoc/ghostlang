# IA V4 experimental — Transformer próprio em GhostLang

A V4 implementa as mudanças de representação, contexto e treinamento indicadas
pelo diagnóstico da V3. O modelo foi treinado do zero neste computador, sem pesos
baixados, PyTorch, API de LLM ou servidor externo de inferência.

**Ainda não está pronta para substituir a V3 estável.** No diagnóstico original
com a semente 1000701692, a geração neural continuou em 0/24 nas tarefas novas.
Em outro conjunto sintético, separado do treino e da validação, acertou 9/60.
Os resultados completos estão abaixo; mudança de arquitetura não é prova de
inteligência geral.

## Executar

Na raiz do projeto:

    bash ia_propria/scripts/ia_v4.sh chat

No chat:

    /internet off
    /neural
    Diga somente a palavra casa.
    /limpar
    /assistente
    O que é GhostLang?
    /sair

O modo inicial combina fontes revisadas, memória adquirida e geração
experimental. Convites para construir um trabalho ou projeto recebem um roteiro
revisado, identificado como `[Planejamento local]`, com opções e uma pergunta
sobre o objetivo. Essa resposta pertence à coordenação em GhostLang, não aos
pesos. No modo assistente, perguntas como “Que horas são?” e “Qual é a
data de hoje?” usam o contexto temporal nativo do GhostLang, sem internet.
A resposta mostra data, hora e fuso configurado no computador; depende da
correção do relógio do sistema. `/agora` consulta esse relógio em qualquer modo.

/neural usa os pesos para gerar texto, inclusive saudações, com uma verificação
estrutural antes da exibição. Ao reconhecer uma
pergunta sobre a hora atual, informa que é necessário usar `/agora` ou voltar
a `/assistente`; não tenta adivinhar a hora pelos pesos. `/tempo` mostra o contexto
temporal em qualquer modo. /fontes desativa
a geração livre. /internet off bloqueia consultas externas. A memória pública
da V3 é compartilhada; os checkpoints neurais são separados.

Continuam disponíveis /stream, /stream on|off, /velocidade N, /memoria,
/buscar TERMOS, /agora, /tempo e /limpar. A geração transmite caracteres UTF-8 completos.
Trocar de modo limpa o contexto da conversa, preservando a memória documental.

A busca e a memória verificam uma correspondência lexical conservadora entre
o tema solicitado e o título do artigo (acentos e palavras de ligação são
normalizados). Resultados sem correspondência suficiente não são apresentados
como conhecimento nem salvos. Essa verificação não prova que o conteúdo responde
à pergunta; sinônimos também podem ser recusados. Registros antigos incompatíveis,
inclusive filmes ou artigos sobre convenções horárias associados a “que horas
são”, são ignorados sem apagar arquivos. Perguntas sobre o filme “Que Horas Ela
Volta?” continuam sendo temas de pesquisa legítimos.

Para compilar e verificar:

    bash ia_propria/scripts/compilar_v4.sh
    bash ia_propria/scripts/ia_v4.sh --device cpu autoteste
    bash ia_propria/scripts/ia_v4.sh --device cuda autoteste
    bash ia_propria/scripts/ia_v4.sh hardware

O script V3 continua disponível e seus checkpoints foram preservados.

## O que mudou no modelo

| Propriedade | V3 neural | V4 neural |
|---|---|---|
| Representação | Vocabulário fixo de palavras, com token desconhecido | 256 bytes e três tokens especiais |
| Arquitetura | Embeddings, janela concatenada, tanh, softmax | Transformer decoder causal, duas camadas |
| Atenção | Ausente | Quatro cabeças por camada |
| Contexto | 64 tokens de palavra | 384 tokens de byte |
| Dimensão interna | Embeddings 32 / camada oculta 192 | Largura 96 / feed-forward 384 |
| Parâmetros | 650.808 | 307.776 |
| Histórico neural | Pergunta isolada no chat V3 | Turnos anteriores incluídos no prompt |

O modelo usa normalização antes dos blocos, conexões residuais, GELU, embeddings
posicionais aprendidos e projeção para 259 classes. A atenção causal impede o
acesso a tokens futuros. Não há dropout, KV cache, RoPE ou pesos compartilhados
entre entrada e saída nesta implementação. A configuração foi escolhida para
permitir treino e auditoria locais, não para competir com um LLM de grande porte.
Referência do mecanismo:
[Attention Is All You Need](https://arxiv.org/abs/1706.03762).

Cada texto UTF-8 aceito pode ser representado sem token de palavra desconhecida,
incluindo acentos e emojis. Isso resolve perda de representação, não garante
compreensão de uma palavra nunca ensinada. Texto UTF-8 inválido e controles de
terminal são recusados. A saída respeita fronteiras de caracteres completos.

384 bytes não equivalem a 384 palavras nem garantem contexto maior que a V3.
O chat preserva a pergunta atual inteira e só acrescenta turnos anteriores
completos que caibam no limite do checkpoint, reservando um token inicial.
Uma pergunta que sozinha ultrapasse o limite recebe um aviso para ser reduzida;
não é enviada parcialmente ao modelo. Respostas documentais grandes não entram
nesse histórico neural. /limpar reinicia esse histórico. A API bruta `generate`
e o comando `gerar` mantêm o recorte pela esquerda com aviso para prompts longos.
Registros da memória documental não são automaticamente incorporados aos pesos.

## Implementação e API GhostLang

O programa e o controle de treinamento estão em src/main.g. O compilador
GhostLang disponibiliza a fábrica nativa transformer_model; o runtime C++
implementa o grafo diferenciável e os kernels CUDA executam treino e inferência
na NVIDIA. A implementação CPU oferece a mesma API. Não foi alterado o tipo
textmodel existente nem a álgebra dos Números G.

    var modelo = transformer_model(384, 96, 4, 2, 8, 42);
    modelo.prepare("v4/dados/treino.gtd", "v4/dados/validacao.gtd");
    double erro = modelo.steps(100, 0.001);
    double validacao = modelo.validate(32);
    modelo.save("checkpoints/v4/experimento.bin");

Argumentos: contexto em bytes, largura, cabeças, camadas, lote e semente.
Há limites de dimensões e de buffers para evitar configurações incompatíveis
com o uso local. O modelo gera texto por generate ou stream e informa a
configuração por info. CPU/CUDA seguem GHOST_DEVICE ou --device.

AdamW mantém momentos e contador de atualizações. O treinamento usa entropia
cruzada nos bytes da resposta e no EOS, sem supervisionar os bytes da pergunta;
gradientes são limitados por componente a [-1,1]. O decaimento dos pesos é
separado da atualização adaptativa.
[Referência de AdamW](https://arxiv.org/abs/1711.05101).

Os checkpoints GTRBYTE1 guardam arquitetura, pesos float32, momentos AdamW,
contador, RNG e checksum de integridade. A gravação é atômica. Um carregamento
inválido não substitui o modelo atual. Checkpoints V2/V3 não são compatíveis com
este formato e continuam sendo lidos por textmodel.

## Dados, treino e separação de avaliações

Python é usado para gerar dados e avaliar resultados; não executa a rede no chat.

    python3 ia_propria/v4/preparar_dados.py
    python3 ia_propria/v4/ampliar_dados.py

A primeira etapa gerou 8.800 registros: cálculo, dedução, leitura de dados,
instruções, contexto, aplicação dos Números G e conteúdo local. A ampliação
produziu 15.308 registros, incluindo a base inicial e novas formulações de cópia
e contexto. Os arquivos JSONL permitem inspecionar as perguntas e respostas; o
runtime lê a versão binária GTD com limites de tamanho e integridade estrutural.

Há 600 registros de validação e 600 de teste, com templates distintos dos
templates iniciais de treino. As formulações adicionais de treino não copiam
esses registros. Os manifestos registram sementes e hashes. Os testes finais
foram executados depois de congelar o checkpoint; seus resultados não foram
usados para retreinar esta versão.

Foram executados 4.000 passos iniciais e mais 3.000 passos com dados ampliados,
com lote 8, na RTX 4050. O log guarda a perda de treino e de validação a cada
100 passos. A validação de perda usa 32 lotes fixos: não é avaliação completa de
600 respostas geradas. O candidato foi selecionado por 12/60 respostas corretas
na amostra de validação, contra 11/60 no candidato escolhido por menor perda.
Essa diferença pequena não demonstra significância estatística.

Para reproduzir com nomes novos:

    bash ia_propria/scripts/ia_v4.sh --device cuda treinar v4/dados/treino.gtd v4/dados/validacao.gtd 40 0.001 checkpoints/v4/experimento.bin
    bash ia_propria/scripts/ia_v4.sh --device cuda continuar v4/dados/treino_ampliado.gtd v4/dados/validacao.gtd 30 0.0003 checkpoints/v4/experimento.bin.ultimo checkpoints/v4/ampliado_novo.bin

O número informado é de blocos de 100 passos, não de épocas. Destinos existentes
são recusados. Melhor perda e último estado são preservados separadamente.
A cópia estável selecionada para uso experimental é
checkpoints/v4/transformer_selecionado.bin. A seleção está registrada em
[SELECAO.json](relatorios/SELECAO.json).

## Resultados e limites

No teste separado, com dez casos por categoria, sem internet, calculadora,
solucionador simbólico ou respostas prontas:

| Categoria | Acertos V4 |
|---|---:|
| Dedução | 8/10 |
| Leitura de dados | 1/10 |
| Cálculo | 0/10 |
| Instruções/cópia | 0/10 |
| Contexto | 0/10 |
| Aplicação dos Números G | 0/10 |

O diagnóstico original, com a semente fornecida pelo usuário, continuou em
0/24 tarefas novas. A geração neural também regrediu nos quatro controles
conhecidos (0/4), enquanto o assistente conserva as fichas documentais. Por isso,
a V4 é uma alternativa experimental, não substitui silenciosamente a V3.
As falhas são substanciais; aprender algumas associações em uma família de
dedução não demonstra raciocínio geral.

Para conferir:

    python3 ia_propria/v4/avaliar.py --device cuda --por-categoria 10
    python3 ia_propria/v3/testar_capacidades.py --versao v4 --device cuda --seed 1000701692
    python3 ia_propria/v3/testar_capacidades.py --versao v3 --seed 1000701692

A correção exige respostas curtas verificáveis; todas as saídas são mantidas
nos relatórios para inspeção. A V4 usa limite de saída em bytes; a V3 usa tokens
de palavra. Os limites ficam registrados na avaliação.

## Validação técnica

Os testes verificam gradientes por diferenças finitas, ausência de vazamento de
tokens futuros, aprendizado de um conjunto mínimo, paridade CPU/CUDA, UTF-8,
integridade de checkpoint e retomada reproduzível de pesos, momentos e RNG em
CPU. O teste de gradientes encontrou erro absoluto máximo de aproximadamente
0,000136 na configuração reduzida.

    cmake --build build_native --target ghost_transformer_test -j 4
    ctest --test-dir build_native --output-on-failure

Esses testes asseguram o funcionamento da implementação nos casos cobertos.
Eles não comprovam que o modelo adquiriu as capacidades que continuam falhando
nas avaliações de respostas completas.

## Regressão de relógio e roteamento

Depois de recompilar, encerre o processo antigo com `/sair` e abra o chat novamente.
O menu atualizado contém `/agora`.

    python3 ia_propria/v4/verificar_chat.py
    GHOST_TEST_DEVICE=cuda python3 ia_propria/v4/verificar_chat.py

Os testes consultam o relógio real em UTC e America/Sao_Paulo, verificam o intervalo
de execução e o deslocamento do fuso, reproduzem as frases relatadas e conferem
que não houve pesquisa nem gravação de memória. Também cobrem perguntas longas,
streaming e limpeza de contexto. Os testes nativos `web_sources_offline` e
`knowledge_memory_offline` usam fontes simuladas para rejeitar títulos irrelevantes.
Essas correções são de ferramentas e coordenação; não representam novo treinamento
ou melhoria comprovada de raciocínio neural.

## Noção operacional de tempo

O chat mantém um `temporal_context()` nativo durante a conversa. Ele combina o
relógio civil do computador (datas, horas e fuso) com `steady_clock` (durações).
As medições de duração não dependem de correções manuais ou sincronizações do
relógio civil. A data exibida depende da configuração correta do computador.

No modo `/assistente` ou `/fontes`, experimente:

    Você tem percepção de tempo?
    Há quanto tempo estamos conversando?
    Quanto tempo passou desde minha mensagem anterior?
    Que dia será amanhã?
    Que dia foi ontem?
    Que dia será depois de amanhã?
    Que horas serão daqui a 15 minutos?
    Daqui a 3 dias
    /tempo

`/tempo` mostra início da conversa, tempo decorrido, número de mensagens e
horário da última mensagem. O intervalo pedido em linguagem natural é medido
entre a chegada da mensagem anterior e a chegada da pergunta atual. Sem uma
mensagem anterior, o chat informa que não tem dados para esse cálculo. Comandos
(como `/stream` e `/tempo`) e linhas vazias não contam como mensagens. As
medições começam quando o chat abre, após carregar o checkpoint.

Hoje, ontem, anteontem, amanhã e depois de amanhã são datas no fuso local; dias
são calculados pelo calendário, respeitando viradas de mês, ano e horário de
verão. Expressões numéricas `daqui a N minutos/horas/dias` e `há N
minutos/horas/dias` aceitam de 0 a 3660 unidades. Minutos e horas são durações;
dias são deslocamentos no calendário. Isso calcula referências temporais;
não agenda tarefas ou lembretes. O reconhecimento é por regras em português,
não uma compreensão ilimitada de qualquer expressão temporal.

`/limpar` reinicia o histórico temporal e a contagem da conversa. Trocar de modo
preserva as medições temporais, embora limpe o histórico de texto neural. Os
horários pessoais ficam somente em RAM, não vão à internet nem à memória
enciclopédica, e não sobrevivem ao encerramento do processo.

No `/neural`, os pesos continuam isolados dessas ferramentas: a interface orienta
usar `/agora`, `/tempo` ou `/assistente` quando reconhece uma solicitação temporal.
Essa funcionalidade não altera o checkpoint, não retreina a rede e não representa
experiência subjetiva do tempo. A camada de coordenação usa dados observados.

API nativa acessível em GhostLang:

```text
var tempo = temporal_context();
tempo.observe();
print(tempo.clock());
print(tempo.info());
print(tempo.answer("que dia sera amanha"));
tempo.clear();
```

`answer` recebe português sem acentos e em minúsculas (o chat usa
`intencao_dialogo`), e devolve texto vazio quando a ferramenta não cobre a
pergunta. `observe()` registra a chegada de uma mensagem antes de respondê-la.

Teste determinístico, sem alterar o relógio do sistema:

    ctest --test-dir build_native --output-on-failure -R temporal_context_offline

Esse teste cobre ano bissexto, viradas de dia/mês/ano, horário de verão,
primeira mensagem sem histórico, reinício do contexto, intervalos excessivos,
roteamento de temas não temporais e ajuste para trás do relógio civil.

## Saídas neurais repetitivas

A frase “Vamos contruir um trabalho juntos. Qual a sua ideia?” reproduziu no
checkpoint selecionado a sequência sem sentido relatada pelo usuário. A falha
vem da geração neural limitada; o acesso ao relógio não ensina linguagem aos
pesos. O checkpoint foi preservado.

O chat agora chama `generate_checked(prompt, max_bytes)`. A geração é acumulada
antes de aparecer. A verificação rejeita texto vazio, ciclos longos de caracteres,
muitas palavras curtas repetidas e respostas que atingem o limite de bytes sem
emitir fim de resposta. Repetições reconhecidas durante a geração a interrompem
mais cedo. A saída recusada não aparece no terminal nem entra no histórico;
o chat exibe `[Geracao neural recusada]` com o motivo. Isso também vale no
modo `/neural`, sem substituir a falha por uma resposta pronta.

Com `/stream on`, a resposta aceita é exibida progressivamente **depois** dessa
verificação. Portanto, há espera antes do primeiro caractere. `/stream off`
mostra o mesmo resultado de uma vez. A API bruta `stream()` continua disponível
para instrumentação e não realiza essa verificação.

A verificação é heurística: pode recusar repetições legítimas e não identifica
toda palavra inventada, resposta incoerente ou erro factual. Uma resposta aceita
não tem correção garantida. Este ajuste não aumenta a capacidade de raciocínio
nem muda os resultados de treinamento. A API `generate()` e o comando `gerar`
continuam expondo a saída bruta para avaliações honestas.

`GhostGenerationResult` contém `accepted`, `text`, `reason` e `generated_bytes`.
Quando `accepted` é falso, `text` fica vazio. Os testes nativos cobrem a sequência
relatada, ciclos, respostas curtas legítimas e texto UTF-8. Os testes do chat
reproduzem a pergunta nos modos assistente e neural, com apresentação progressiva
ligada e desligada, e verificam que a recusa não contamina a próxima resposta.


## Jogos de texto para aprendizagem supervisionada

Há um currículo de seis jogos com três níveis, dados procedurais, treino nativo
GhostLang/CUDA e avaliação diretamente pelos pesos, sem os textos do chat.
Consulte [jogos/README.md](jogos/README.md) para preparar, jogar, treinar e avaliar.
Os checkpoints dos jogos são experimentos separados; o chat padrão é preservado.


O [painel de progresso dos jogos](jogos/painel_progresso.html) mostra resultados
por habilidade, comparação entre checkpoints e respostas reais. Atualize-o com
`python3 ia_propria/v4/jogos/painel.py` após novas avaliações. A medição completa
dos dois conjuntos passou de 7/216 e 3/216 para 33/216 e 30/216, respectivamente;
ainda é um sinal limitado nos exercícios, sem comprovação de inteligência geral.
