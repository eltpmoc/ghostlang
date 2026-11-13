# V3.4: pesquisa automática e conhecimento persistente

Agora basta perguntar. Quando a base revisada não cobre a pergunta, o assistente
seleciona um tema público, procura um registro local desse tema e, se necessário,
consulta a fonte externa. Uma consulta bem-sucedida é gravada para uso posterior,
inclusive depois de fechar o programa e sem conexão.

O fluxo é coordenado em GhostLang, em src/main.g e src/autonomia.g. O transporte
HTTPS e a persistência pertencem ao runtime nativo C++; o modelo próprio continua
executando em CPU/CUDA. Não foi acrescentado um serviço externo de LLM.

## Experimentar

Na raiz do projeto, execute:

    bash ia_propria/scripts/ia_v3.sh chat

Pergunte normalmente, sem comandos especiais:

    O que é computação quântica?
    Quem é o presidente da França?
    /memoria
    /sair

Em outra sessão:

    /internet off
    Explique a computação quântica.
    /memoria

A primeira consulta pode mostrar “Pesquisa automatica” e “Conhecimento salvo”.
A segunda mostra “Memoria local”, preservando o endereço e a data da fonte.
“O que é”, “explique” e “como funciona” são normalizados para permitir essas
reformulações simples do mesmo tema.

## Decisão e controles

1. Saudações e esclarecimentos são tratados localmente.
2. Fichas revisadas de GhostLang e Números G têm prioridade.
3. Para um tema público sem ficha, consulta a memória adquirida.
4. Sem registro válido, pesquisa automaticamente e guarda a referência encontrada.
5. Se a fonte falhar, avisa; não salva a mensagem de erro como conhecimento.

Comandos disponíveis:

- /internet off: bloqueia novas consultas; preserva a consulta local e a memória.
- /internet on: reativa a pesquisa automática; é o padrão ao iniciar o chat.
- /buscar PALAVRAS-CHAVE: solicita atualização imediata desse tema público.
- /memoria: lista quantidade, temas, fontes, datas e necessidade de atualização.
- /limpar: limpa o contexto da sessão; o conhecimento persistente é preservado.
- /assistente ou /fontes: prioriza respostas documentais.
- /neural: mantém geração experimental para assuntos locais cobertos; perguntas
  públicas sem cobertura também podem usar a pesquisa e a memória, com rótulos
  próprios, sem atribuir o resultado aos pesos.

A decisão usa regras e correspondência de temas implementadas em GhostLang,
não uma rede treinada para planejar ferramentas. O seletor envia apenas o tema
extraído da pergunta atual. Histórico, arquivos, checkpoints e código não são
enviados. Recusa formatos longos, URLs, números, código, referências pessoais e
alguns termos sensíveis. Essas regras são conservadoras, podem recusar perguntas
legítimas e não detectam todo conteúdo privado. Para assuntos confidenciais, use
/internet off. Os limites atuais são 240 bytes na pergunta selecionável e 16
palavras/160 bytes no tema enviado.

## O que fica armazenado

O diretório padrão é ia_propria/v3/memoria, quando usado pelo script de execução.
Cada arquivo .gkm contém versão do formato, instante UTC da obtenção, tema
normalizado, URL da fonte e resposta documental atribuída. O formato textual
GHOST_KNOWLEDGE_1 usa campos delimitados por aspas com escapes de aspas e barras.
Não é JSON nem um checkpoint neural. GHOST_KNOWLEDGE_DIR permite escolher outro
diretório, usado também para isolar os testes.

A escrita usa arquivo temporário com permissão 0600, sincronização e substituição
atômica sob bloqueio. Uma falha de atualização preserva o registro anterior.
Registros corrompidos ou com origem/formato inválido são ignorados. Há limite de
500 arquivos, com até 24.000 bytes por arquivo; ao atingir o limite, a resposta
ainda é exibida e o programa informa que não conseguiu adicioná-la. Registros
antigos não são apagados automaticamente. O mesmo tema atualizado substitui seu
próprio registro.

As fontes continuam limitadas à Wikipédia em português e ao adaptador da lista
oficial da Presidência francesa. Textos da enciclopédia são introduções de até
1.200 caracteres, mostradas como trechos relacionados; uma referência armazenada
não é garantia de que respondeu à pergunta inteira. Não existe navegação livre
por qualquer site, ingestão contínua em segundo plano ou execução de conteúdo web.

## Atualidade e qualidade

O registro sobre o presidente francês vence após uma hora. Temas com alguns
termos temporais, como “atual”, “presidente” e “preço”, também usam uma hora.
Demais temas usam sete dias. Esses prazos são uma política de atualização,
não uma garantia de que a fonte permaneceu correta durante o intervalo.

Quando um registro vence, uma nova pergunta tenta atualizá-lo. Sem conexão, a
enciclopédia pode ser mostrada como arquivo desatualizado. Um ocupante de cargo
obtido de registro oficial vencido não é apresentado como ocupante atual. A data
de consulta original nunca é trocada pela data de leitura do cache.

A memória guarda referências; não transforma instruções de páginas em comandos,
não incorpora saídas neurais à base revisada e não usa falhas de rede como dados.
Ainda não há verificação cruzada de várias fontes ou avaliação semântica da
relevância do primeiro artigo encontrado. A base adquirida permanece identificada
como externa e separada das fichas revisadas.

## Base consultável e pesos próprios

Esta atualização enriquece automaticamente a base que a IA consegue consultar.
Não faz treinamento online dos pesos: o checkpoint de 650.808 parâmetros e seu
otimizador permanecem intactos. Incorporar conteúdo da internet aos pesos exige
seleção de dados, treinamento e avaliação separados; consultar e recordar uma
fonte não demonstra melhora de generalização da rede.

## Validação

    cmake --build build_native --target ghost_knowledge_test ghost_web_test ghost_runtime_test -j 4
    ctest --test-dir build_native --output-on-failure
    python3 ia_propria/v3/testar_memoria.py --online
    GHOST_TEST_DEVICE=cuda python3 ia_propria/v3/testar_memoria.py --online
    python3 ia_propria/v3/testar_stream.py
    python3 ia_propria/v3/testar_respostas.py

Os testes de memória verificam uso entre processos, reformulações, ausência de
novas chamadas para registros válidos, atualização por prazo, funcionamento
offline, fonte inválida, corrupção, falhas de escrita, limite de registros e
preservação dos dados quando a rede falha. Os testes online usam diretórios
temporários e apenas temas públicos; não alteram a memória de uso real.
