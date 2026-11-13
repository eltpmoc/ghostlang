# Diagnóstico de capacidades

Execute na raiz do projeto:

    python3 ia_propria/v3/testar_capacidades.py

Cada execução escolhe e registra uma semente nova. Para repetir exatamente os
enunciados de uma execução:

    python3 ia_propria/v3/testar_capacidades.py --seed 20260926

Para executar a inferência na GPU:

    python3 ia_propria/v3/testar_capacidades.py --seed 20260926 --device cuda

Os relatórios JSON e Markdown ficam em v3/avaliacoes, com nomes únicos. Eles
contêm todas as perguntas, gabaritos, respostas, avisos de vocabulário, tempos,
configuração e hash do checkpoint. Os pesos e a memória de uso real não são
alterados. O avaliador é uma ferramenta Python; a IA avaliada continua sendo o
programa compilado de GhostLang, usando seu modelo nativo.

## O que mede

São 24 tarefas novas, com quatro casos em cada categoria:

- Cálculo com valores gerados na execução.
- Dedução a partir de regras fornecidas no próprio enunciado.
- Cumprimento de instruções simples de saída.
- Extração e comparação de informações fornecidas.
- Uso de informação nova dada em um turno anterior.
- Aplicação da regra do produto G a valores novos; a fórmula é fornecida.

Há também quatro perguntas familiares do corpus, em uma categoria separada.
Esses controles verificam recuperação de conteúdo já conhecido; não contam como
demonstração de generalização.

Cada conjunto é apresentado a dois caminhos:

1. Assistente offline: usa o chat normal, suas regras e fichas documentais.
2. Neural direto: usa o comando gerar, com o checkpoint próprio e sem o filtro,
   as respostas prontas ou a busca documental do chat.

A internet fica desligada e a memória persistente de cada caso é isolada em
diretório temporário. Em tarefas de dois turnos, o assistente recebe uma sessão
com esses turnos; a rede recebe um prompt com o histórico real que produziu.
São usados temperatura zero e limite de 128 tokens por resposta.

## Como interpretar

Não há uma nota única de “inteligência” nem nota de QI. O relatório mostra
acertos por habilidade e registra falhas de execução separadamente.

As tarefas novas pedem uma resposta curta verificável. A correção aceita
diferenças simples de espaços, acentos e pontuação final; tuplas aceitam
parênteses e espaços. Nos controles conhecidos, ignora também o espaçamento de
tokens como C + + e . g. Respostas semanticamente equivalentes fora dessas regras
precisam de revisão manual no relatório.

O script verifica que os enunciados novos não aparecem literalmente nos corpora
conhecidos de treino da V3. Isso não prova ausência de problemas semelhantes no
treino. É uma amostra pequena de tarefas geradas por modelos de enunciado fixos;
não cobre criatividade aberta, conhecimento geral, programação arbitrária ou
confiabilidade em produção. Não adicione os gabaritos ao treino e depois trate
uma repetição da mesma avaliação como evidência de generalização.

Busca e memória são avaliadas separadamente:

    python3 ia_propria/v3/testar_memoria.py --online

Esse teste verifica funcionamento da pesquisa, armazenamento e reutilização,
não a capacidade da rede de resolver problemas novos.

## Resultado observado em 26 de setembro de 2026

Com semente 20260926 e o checkpoint ghost_g_v31.bin em CPU:

| Conjunto | Assistente offline | Neural direto |
|---|---:|---:|
| 24 tarefas novas | 0/24 | 0/24 |
| 4 perguntas conhecidas | 4/4 | 4/4 |
| Falhas técnicas | 0 | 0 |

O assistente recusou as 24 tarefas sem cobertura local. A geração neural
frequentemente devolveu frases sem relação com o problema e registrou palavras
fora do vocabulário. Por exemplo, para 26 peças mais 12 menos 2, o gabarito era
36; a rede respondeu uma frase sobre GhostLang em vez de resolver a conta.

Esses resultados mostram recuperação de conteúdo conhecido e uma limitação
clara nas habilidades novas amostradas. A pesquisa e a memória ampliam as fontes
disponíveis, mas esta avaliação não demonstra que os pesos aprenderam raciocínio.
Os 4/4 controles não são comparáveis aos 13/89 da avaliação anterior: são quatro
perguntas familiares, enquanto a avaliação anterior usa outras formulações.

Relatório desta execução:
[resultados e respostas completas](avaliacoes/capacidades_20260926T123231911671Z_20260926_cpu.md).

## Comparar a V4 experimental

    python3 ia_propria/v3/testar_capacidades.py --versao v4 --device cuda --seed 1000701692

Sem --versao, o diagnóstico continua usando a V3. O relatório registra arquitetura selecionada, checkpoint e limite de saída. Consulte [os resultados da V4](../v4/README.md).
