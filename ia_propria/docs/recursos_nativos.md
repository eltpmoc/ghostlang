# Recursos locais do GhostLang

Inventário baseado nos fontes presentes em `../src`, `../include` e nos manuais locais. A implementação prevalece sobre exemplos do manual. O objetivo é aproveitar os recursos aplicáveis ao modelo de texto, sem inserir operações sem função no treinamento.

| Recurso | Uso ou situação nesta IA |
|---|---|
| `gate`, `state`, álgebra matricial | Pesos, entradas one-hot, alvos e predições |
| `network`, `>>` | Rede de duas camadas com bias e ReLU intermediária |
| `optimize`, Adam, backpropagation | Treinamento nativo em CPU; CUDA equivalente implementado separadamente em `ia_propria/backend/` |
| Gradiente descendente (`gd`) | Disponível no runtime; o modelo escolhe Adam |
| ReLU, sigmoid, sin, cos, tan | ReLU usada pela rede; outras ativações não são necessárias à arquitetura |
| `transpose`, produto elemento a elemento | Usados internamente pelo backpropagation |
| `encode`, `decode` | `encode` cria estados vazios; codificação de caracteres feita pela IA. O par nativo baseado em ASCII não é um tokenizer de LLM |
| Arquivos e strings, regex | Corpus, normalização e checkpoints textuais versionados |
| Funções, laços, tipos e `import` | Separação dos módulos e pipeline de treinamento |
| CSV | Métricas exportadas; pesos mantidos em formato textual validado |
| OpenMP | Disponível nas multiplicações; controle pelo lançador |
| `pragma gpu` | Não há backend CUDA/OpenCL no treinamento matricial examinado |
| `ghost` e `complex` | Runtime usa números Ghost; componentes não reais ficam zerados. O otimizador atual atualiza componentes reais |
| Hadamard e portas Pauli, `apply` | Simulação quântica disponível, sem finalidade nesta arquitetura textual |
| `conv` | Convoluções disponíveis para outras arquiteturas; não usadas neste modelo |
| JSON, datas, classes | Recursos auxiliares disponíveis; dispensáveis para o núcleo atual |
| `server`, `route`, `listen` | Possível etapa futura de serviço de inferência; nenhum servidor é iniciado por este projeto |
| `spawn`, mutex, sleep | Concorrência auxiliar; treino mantém atualização sequencial dos pesos |
| `swarm` | A tradução examinada invoca `start ...ghost_swarm.exe`; não é habilitada neste projeto Linux |
| `llm`, `load_gguf`, prompt | Encapsulam inferência com modelo externo e llama-cli; não treinam nosso modelo próprio |
| WebAssembly | Caminho de compilação opcional no Ghost; não validado para esta aplicação Ubuntu |
| `.save()`/`.load()` binários | Não usados: gravam representação de objetos com memória dinâmica; adotado formato textual só com parâmetros reais |

Referências locais: `include/GhostMatrix.hpp` para rede, Adam e OpenMP; `include/GhostLib.hpp` para I/O e codificação; `src/Parser.cpp` para sintaxe efetiva; `src/ghost_compiler.cpp` para o comando de compilação; `include/GhostNumberG.hpp` para o layout do número Ghost. Todos permanecem intactos.

## Extensão CUDA da aplicação

A IA agora possui backend CUDA opcional em `backend/cuda.cu` e ponte em `backend/ponte.hpp`. Esse recurso pertence à aplicação, não ao GhostLang original. Suporta a rede 168 → 32 → 42, forward, backpropagation e Adam em GPU, com checkpoints intercambiáveis. Consulte [CUDA](cuda.md).
