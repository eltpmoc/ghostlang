# Validação da V3 — 26/09/2026

Checkpoint: `checkpoints/v3/ghost_g.bin`. Treino executado localmente na NVIDIA GeForce RTX 4050 Laptop GPU, usando o runtime nativo GhostLang/CUDA. Não foram baixados modelos.

## Modelo e corpus

636.633 parâmetros reais float32, contexto 64 palavras/tokens, embedding 32, camada oculta 192, lote 32 e vocabulário efetivo de 1.081 tokens. A arquitetura é declarada em GhostLang em `src/main.g`.

89 fichas: 32 específicas do projeto e 57 de conversação geral. Foram usadas 178 conversas de treino (5.719 alvos supervisionados) e 89 de validação (2.860 alvos). Aproximadamente 2,02% dos tokens da validação ficaram fora do vocabulário. As respostas se repetem entre as partições; somente as perguntas são reformuladas. Não é uma avaliação de aquisição de fatos novos.

Treino: 30 épocas, taxa 0,001, AdamW. Melhor checkpoint na época 30:

| Métrica | Resultado |
|---|---:|
| CE de validação antes do treino | 6,98077 |
| CE de validação após o treino | 0,336913553 |
| Perplexidade na validação | 1,400617979 |
| Acurácia de próximo token com contexto correto | 93,4266% |
| CE de treino online na última época | 0,002462432 |

A grande diferença entre treino e validação e os resultados abaixo mostram que a rede memoriza os exemplos com facilidade e generaliza pouco.

## Respostas completas, sem fornecer a resposta como contexto

A avaliação em `avaliar.py` usa CPU, temperatura zero, sem penalidades de repetição e limite de 160 tokens. A comparação ignora maiúsculas e espaços, inclusive os espaços introduzidos ao renderizar pontuação; exige o restante da resposta completa. Não é uma avaliação semântica humana.

- **Geração neural: 11/89 respostas correspondentes (12,36%).** Nas 32 perguntas sobre o projeto, foram 4/32. Há respostas de assunto errado e frases incoerentes; a rede ainda não é um assistente geral confiável.
- **Consulta documental: 89/89 perguntas cadastradas.** São formulações incluídas na base, portanto esse número não mede generalização.
- **Consulta adicional: 16/20 resultados esperados.** Entre dez reformulações que não estavam na base, respondeu seis corretamente e recusou quatro. Nas dez perguntas sem cobertura, recusou todas. Este conjunto pequeno não prova que toda pergunta desconhecida será recusada.

A consulta usa busca lexical em fichas revisadas, implementada em GhostLang, e mostra a fonte. Não usa os pesos para produzir a resposta. O chat começa nesse modo; `/neural` ativa explicitamente a geração experimental. O resultado detalhado, incluindo todas as saídas neurais e perguntas adicionais, está em [AVALIACAO.json](AVALIACAO.json).

## Testes de implementação

Passaram no ambiente local:

- CMake com CUDA ON e com CUDA OFF; testes do runtime em ambos os builds.
- Produtos matriciais CPU/CUDA com x, y, p e theta2, dimensões variadas e caudas de blocos.
- Gradientes por diferenças finitas e comparação CPU/CUDA de perda, gradientes e atualização AdamW.
- Duas instâncias independentes de `textmodel`, persistência e autoteste sem alterar pesos existentes.
- Compilação fora da raiz e em caminhos com espaços/apóstrofo; retorno de erro quando g++ falha.
- `pragma auto/cpu/gpu`, dispositivo inválido, fallback sem GPU visível e erro de CUDA obrigatório.
- Carregamento do checkpoint V2 `dialogo.bin` pelo tipo nativo.
- Exemplos existentes `syntax.g`, `test_algebra.g` e `test_4d.g`.
- Treino controlado da V3 com queda de CE de 2,704590 para 1,304367 em CPU e CUDA.
- Retomada CPU bit a bit; retomada CUDA e portabilidade CPU/CUDA dentro das tolerâncias.
- Melhor checkpoint, proteção contra sobrescrita, checksum, UTF-8 inválido, argumentos inválidos, troca de modo do chat e EOF.

Comandos a partir da raiz:

```bash
cmake -S . -B build_native -DGHOST_CUDA=ON -DGHOST_TEST_CUDA=ON
cmake --build build_native --target ghost ghost_runtime_test -j 4
ctest --test-dir build_native --output-on-failure
python3 tests/native_integration.py --cuda
python3 ia_propria/v3/verificar.py
GHOST_TEST_DEVICE=cuda python3 ia_propria/v3/verificar.py
python3 ia_propria/v3/avaliar.py
```

Os exemplos antigos `test_algebra_decl.g` e `test_print.g`, que usam literais `2.0i`/`3.0g`, não são incluídos como regressões aprovadas. O primeiro também usa `display()`, ausente na classe atual. A falha do primeiro foi reproduzida compilando os fontes do backup anterior à integração. Não foi corrigida como parte do CUDA. A matemática existente também foi preservada; veja [o registro das regras G](../../docs/NUMEROS_G_IMPLEMENTACAO.md).
