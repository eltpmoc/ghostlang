# V3.1 — explicações e exemplos completos

A pergunta `Descreva o que é GhostLang e crie um exemplo.` agora é atendida no modo fontes com: definição, funcionamento, código completo, explicação das instruções, resultado esperado, comando de compilação e referências. A busca distingue termos de assunto de pedidos como "descreva", "crie um exemplo" e "resuma".

Foram acrescentados seis programas revisados: introdução à linguagem, multiplicação matriz/vetor, operações com Números G, nilpotência, seleção CPU/CUDA e treinamento com `textmodel`. Todos foram compilados e executados. O código exibido é lido desses mesmos arquivos durante a preparação da base; não é produzido livremente pela rede.

O chat documental lembra apenas a última ficha reconhecida para responder a "agora mostre um exemplo" ou "mais detalhes". `/limpar`, a troca de modo e uma consulta não reconhecida removem esse assunto. Uma consulta a dois assuntos não define um assunto único para continuação. Não há memória de conversa completa nem armazenamento das mensagens. Pedidos com dois assuntos reconhecidos ligados por "e" podem receber duas explicações; a base continua limitada e pode pedir reformulação.

## Usar

Encerre o processo antigo com `/sair` e abra novamente:

```bash
cd ia_propria
bash scripts/ia_v3.sh chat
```

Experimente:

```text
Descreva o que é GhostLang e crie um exemplo.
Explique os Números Fantasmas G com um exemplo.
O que é textmodel?
Agora mostre um exemplo.
Resuma o que é GhostLang.
```

O modo fontes permanece padrão. O modo `/neural` usa agora `checkpoints/v3/ghost_g_v31.bin`. O checkpoint original `ghost_g.bin` foi preservado e pode ser escolhido explicitamente: `bash scripts/ia_v3.sh chat checkpoints/v3/ghost_g.bin`.

## Treino e comparação neural

O novo modelo tem 650.808 parâmetros, vocabulário de 1.144 tokens e a mesma arquitetura de janela fixa/tanh. Foi treinado do zero na RTX 4050 com 802 instruções e 95 conversas de validação. A ampliação inclui paráfrases autorais e variações superficiais de prefixo/sufixo; não equivale a um corpus amplo de conhecimento novo.

O treino solicitado era de até 40 épocas, mas parou na época 16 após oito épocas sem melhora. O melhor checkpoint foi o da época 8, CE de validação 0,161621. A seleção foi feita por CE, não pela avaliação livre de respostas. O estado final fica em `.ultimo`.

Na comparação com os mesmos prompts, usando temperatura zero, sem penalidades e limite de 320 tokens:

| Avaliação | V3 anterior | V3.1 |
|---|---:|---:|
| Respostas completas nas 89 perguntas originais | 11/89 | 13/89 |
| Subconjunto de 32 perguntas sobre o projeto | 4/32 | 3/32 |
| Seis novas perguntas pedindo exemplos | 0/6 | 1/6 |

As perguntas reservadas não entram no treino, mas suas respostas são conhecidas. A correspondência ignora espaços e maiúsculas; não mede equivalência semântica nem execução de código neural. A melhora geral é pequena e não ocorre em todos os assuntos. As respostas geradas ainda podem mudar de tema, inventar fatos ou conter código incorreto.

A pergunta exata enviada pelo usuário **foi incluída no treino** como regressão. O novo modelo produz explicação e exemplo para ela; isso demonstra aprendizado daquele exemplo, não generalização ampla. A renderização neural insere espaços na pontuação e dentro de strings; o modo fontes preserva o código original validado.

A geração no lançador passou a ser determinística por padrão (`GHOST_TEXT_TEMP=0`), sem bloqueio de n-gramas e sem penalidade de repetição, pois repetições de nomes e símbolos são necessárias em código. O limite do chat é 320 tokens; respostas não possuem comprimento arbitrário. As variáveis de ambiente continuam permitindo outros parâmetros.

## Consulta documental e testes

- 89/89 perguntas cadastradas mantiveram o resumo correto, podendo acrescentar detalhes.
- Nas 20 consultas adicionais anteriores: 18/20 resultados esperados, contra 16/20. Oito das dez reformulações foram atendidas e duas recusadas; as dez perguntas sem cobertura foram recusadas.
- 22 regressões específicas: 12 pedidos de explicação/exemplo e dez recusas fora da cobertura, além de continuidade/limpeza de assunto.
- Seis exemplos compilados e executados, incluindo treino de demonstração e autoteste.

Essas baterias são pequenas e não garantem acerto ou recusa para qualquer pergunta. Resultados completos: `COMPARACAO_V31.json`, `AVALIACAO_ATUAL.json` e `TESTES_RESPOSTAS.json`. Os relatórios históricos da V3 foram preservados.

## Reproduzir

Na pasta `ia_propria`:

```bash
bash scripts/compilar_v3.sh
python3 v3/testar_respostas.py
python3 v3/comparar_modelos.py
python3 v3/avaliar.py
python3 v3/preparar_instrucoes.py
bash scripts/ia_v3.sh --device cuda treinar v3/dados/instrucoes_treino.txt v3/dados/instrucoes_validacao.txt 40 0.001 checkpoints/v3/outro_treino.bin
```

Os dados de consulta ficam em `dados/conhecimento.json`; a geração de código de consulta, em `gerar_consulta.py`; o código executado no chat continua em GhostLang. `preparar_instrucoes.py` gera o corpus ampliado em arquivos separados do corpus original. Nenhuma regra matemática G nem kernel CUDA foi alterado nesta atualização.
