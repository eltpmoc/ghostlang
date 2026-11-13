# V3.2 — streaming e cobertura local

Registro da versão anterior. A [V3.3](INTERNET_CONTEXTO.md) mantém o streaming e acrescenta contexto e buscas explícitas; as mensagens de recusa e identidade abaixo são históricas.

O chat tem exibição progressiva habilitada. No terminal, o padrão é uma pausa de **12 ms por caractere UTF-8**. Em saída redirecionada/pipe, o padrão é zero para não atrasar scripts e testes. `GHOST_STREAM_MS=20` configura explicitamente 20 ms, inclusive em pipes.

Dentro de `ia_propria`, encerre o processo antigo com `/sair` e reabra:

```bash
bash scripts/ia_v3.sh chat
# Opcional: iniciar com texto mais lento
GHOST_STREAM_MS=20 bash scripts/ia_v3.sh chat
```

No chat:

```text
/velocidade 20
/stream off
/stream on
```

`/velocidade N` aceita inteiros de 0 a 100 **milissegundos de pausa por caractere**. Maior valor deixa a exibição mais lenta; zero retira a pausa artificial. `/stream off` espera a resposta completa e a imprime de uma vez; `/stream on` restaura a emissão progressiva com a velocidade configurada. `Ctrl+C` encerra o processo se precisar interromper uma resposta longa; os comandos de chat são lidos após a resposta atual.

## O que é transmitido

- **Neural:** `modelo.stream()` emite cada trecho imediatamente após prever seu token, antes de calcular o próximo. A pausa opcional regula a exibição. Não espera gerar a resposta inteira para iniciar a saída.
- **Fontes:** o texto revisado já está disponível; `text_stream()` apenas o apresenta gradualmente. É um efeito de exibição, não geração neural.
- Acentos e símbolos UTF-8 são enviados com seus bytes completos. Espaços, código e quebras de linha são preservados.
- Validações e avisos de vocabulário aparecem antes do rótulo `[Geracao neural]`. Uma falha durante a geração termina a linha parcial antes de informar o erro.

## Perguntas fora da cobertura

O modo `/neural` agora verifica se `localizar()` encontra correspondência suficiente e não ambígua na base local **antes** de executar a rede. Tanto `Qual é o presidente da França?` quanto `Qual é o presidente da Frnaça?` são recusadas com:

> [Sem fonte suficiente] Não identifiquei cobertura para essa pergunta na base local. Preciso de uma fonte confiável e atualizada para responder. Não consulto a internet.

Esse controle é uma busca lexical conservadora em GhostLang, não uma medida de confiança da rede nem um detector universal de erros. Pode recusar uma pergunta válida reformulada e não verifica a veracidade da resposta gerada para uma pergunta aceita. A rede ainda pode confundir assuntos. O modo fontes permanece indicado para explicações e exemplos revisados.

O número de tokens fora do vocabulário não mede conhecimento nem precisão. Esses avisos técnicos continuam disponíveis, separados da resposta. Não foram acrescentados fatos políticos atuais ou consulta automática à internet.

## API nativa e avaliação

```ghost
textmodel ia;
ia.load("checkpoints/v3/ghost_g_v31.bin");
ia.stream("Usuário: Olá!\nAssistente:", 100, 12, "IA> ");
text_stream("Texto pronto, exibido gradualmente.", 12);
```

`stream(prompt, max_tokens=100, delay_ms=-1, prefix="")` retorna o mesmo texto de `generate`, além de escrevê-lo no terminal com uma quebra de linha final. Para ausência de tokens, mostra `[fim de resposta]` e retorna string vazia. O valor `-1` resolve o atraso pela variável/terminal; `text_stream_delay()` informa esse padrão.

A proteção de cobertura pertence ao chat V3.2. `generate()`, `stream()`, o comando CLI `gerar` e a interface antiga `ia.chat()` do runtime não aplicam esse filtro automaticamente. A interface `ia.chat()` também conserva sua saída em bloco. A avaliação neural usa `gerar` diretamente para que uma recusa do filtro não seja contada como aprendizado. Nenhum peso foi treinado ou alterado nesta atualização.

## Verificação

Foram verificados: emissão antes do fim da resposta, identidade entre streaming e saída completa, UTF-8, posição dos avisos, velocidades inválidas, as duas perguntas acima e os exemplos documentais. Os testes foram executados em CPU e CUDA; o runtime também foi construído sem CUDA.

```bash
# Na pasta ia_propria:
python3 v3/testar_stream.py
GHOST_TEST_DEVICE=cuda python3 v3/testar_stream.py
python3 v3/testar_respostas.py
```

Resultados: `TESTES_STREAM_cpu.json` e `TESTES_STREAM_cuda.json`. A comparação das 95 respostas neurais brutas com o relatório anterior também foi preservada byte a byte; o streaming não aumenta o conhecimento do modelo.
