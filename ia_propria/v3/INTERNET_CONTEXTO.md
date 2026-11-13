# IA V3.3: conversa e fontes públicas

Registro histórico. A [V3.4](MEMORIA_AUTONOMA.md) amplia esta versão com decisão automática de pesquisa e memória persistente; a restrição a buscas explícitas descrita abaixo foi substituída.

A aplicação continua coordenada em GhostLang (`src/main.g`, `src/dialogo.g` e
`src/conhecimento.g`). O tipo nativo `textmodel` executa os pesos próprios em CPU
ou CUDA. O novo transporte HTTPS pertence ao runtime C++ da linguagem e usa o
`/usr/bin/curl` instalado no Ubuntu. Não há API de LLM, modelo externo ou Python
durante o chat. Python continua sendo usado para preparar dados e executar testes.

## Usar

Na raiz do projeto:

```bash
bash ia_propria/scripts/compilar_v3.sh
bash ia_propria/scripts/ia_v3.sh chat
```

Exemplo de sessão:

```text
Olá
Quem é você?
Descreva GhostLang e crie um exemplo.
Não entendi
/buscar presidente da França
/buscar computação quântica
/internet off
/stream
/velocidade 20
/limpar
/sair
```

O modo inicial `/assistente` responde com as fichas locais revisadas; `/fontes`
é um sinônimo. Saudações, identidade e esclarecimentos sobre a possibilidade de
errar têm tratamento explícito em GhostLang, inclusive em `/neural`. Esses textos
recebem o rótulo **Conversa local**, não geração neural.

O chat conserva a última ficha e a última pergunta/resposta de conteúdo somente
na memória da sessão. “Não entendi”, “explique melhor” e pedidos de exemplo retomam
essa ficha com detalhes e exemplo revisado, quando disponível. Sem ficha, recupera
a resposta anterior e informa que não possui explicação adicional validada.
Isso é continuidade limitada de assunto, não compreensão arbitrária do histórico.
Saudações e trocas de modo preservam o assunto; `/limpar` apaga o contexto.

`/stream` mostra o estado. `/stream on`, `/stream off` e `/velocidade 0..100`
continuam disponíveis. Texto documental e respostas de busca já prontas recebem
exibição progressiva; a geração neural transmite os trechos à medida que os gera.
As consultas de rede são síncronas e exibem uma mensagem de espera antes do resultado.

## Internet explícita e verificável

Somente `/buscar PALAVRAS-CHAVE` acessa a rede. Perguntas comuns, histórico,
checkpoints e arquivos não são enviados. A abertura do chat e a ajuda do comando
informam o destino e pedem termos públicos, sem dados pessoais ou segredos.
São aceitas até 16 palavras e 160 bytes, sem números, e-mails, URLs ou código.
Essa validação de formato **não detecta todo conteúdo sensível**: use termos públicos.
`/internet off` bloqueia as buscas; `/internet on` habilita o comando, sem buscas
automáticas. Não há salvamento automático de conversas ou aprendizagem com a rede.

- Temas gerais usam a API pública da Wikipédia em português: localizam o primeiro
  artigo relacionado e exibem até 1.200 caracteres de introdução, título, URL e
  horário UTC de consulta. É um trecho atribuído, não uma síntese neural nem uma
  resposta necessariamente completa. A posição na busca não garante relevância.
- Para “presidente da França”, há um adaptador da lista oficial da Presidência
  francesa. Ele identifica dinamicamente o nome marcado **In office**; o nome do
  presidente não está codificado. “Frnaça” é uma correção explícita desse comando.
  Se o HTML mudar ou houver ambiguidade, informa a falha, sem usar presidentes
  anteriores como resposta atual. Outros fatos atuais não têm esse adaptador.
- Falhas de rede, ausência de resultados e respostas inesperadas são informadas;
  o chat local continua funcionando. Não existe navegador geral, busca em toda a
  web, execução de páginas, downloads de programas ou leitura de URLs arbitrárias.

O runtime usa HTTPS validado, destinos fixos, argumentos sem shell, sem seguir
redirecionamentos, sem ler `.curlrc`, no máximo 2 MiB por resposta e dez segundos
por requisição (duas para a enciclopédia). Textos remotos são dados exibidos, nunca
comandos, instruções de ferramentas ou exemplos executados. Controles de terminal
são removidos. Dependência de execução: `/usr/bin/curl`; nenhuma chave de API.

API nativa disponível em programas compilados pelo GhostLang:

```ghost
function void main() {
    // Termos publicos escolhidos explicitamente pelo usuario do programa.
    print(web_search_public("computacao quantica"));
}
```

Referências dos serviços:
[MediaWiki Search](https://www.mediawiki.org/wiki/API:Search),
[TextExtracts](https://www.mediawiki.org/wiki/Extension:TextExtracts#API),
[lista oficial francesa](https://www.elysee.fr/en/french-presidency/the-presidents-of-the-republic).

## O que esta atualização melhora e o que falta

Corrige os casos de saudação e esclarecimento relatados, amplia as fontes
consultáveis e mantém continuidade local. **Não altera a arquitetura nem treina
novos pesos**. O checkpoint anterior permanece intacto; a geração experimental
continua limitada pela rede de contexto fixo. Ter ferramentas e acesso a textos
não demonstra raciocínio avançado, aprendizagem autônoma ou melhora em um teste de
generalização neural. `/assistente` é o modo recomendado para fontes revisadas;
`/neural` continua disponível para experimentar os pesos próprios.

## Verificar

```bash
cmake --build build_native --target ghost_web_test ghost_runtime_test -j 4
ctest --test-dir build_native --output-on-failure
python3 ia_propria/v3/testar_dialogo.py
python3 ia_propria/v3/testar_dialogo.py --online
python3 ia_propria/v3/testar_stream.py
python3 ia_propria/v3/testar_respostas.py
```

`--online` consulta apenas os temas públicos de teste; não exige um nome de
presidente fixo. Sem a opção, testa também indisponibilidade usando um proxy local
fechado. Resultados ficam em `TESTES_DIALOGO_cpu.json` e, quando executado com
`GHOST_TEST_DEVICE=cuda`, `TESTES_DIALOGO_cuda.json`.
