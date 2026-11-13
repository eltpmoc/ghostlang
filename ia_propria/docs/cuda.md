# Execução automática em CPU ou GPU

O GhostLang original não foi alterado. A aplicação agora contém uma extensão C++/CUDA em `backend/`, integrada durante a compilação dos programas GhostLang. Os módulos `.g` continuam controlando os dados, treinamento, avaliação, geração e checkpoints.

## Uso

Na pasta `ia_propria`:

```bash
bash scripts/compilar.sh
bash scripts/ia.sh --hardware
bash scripts/ia.sh gerar checkpoints/demonstracao_cuda.txt "ola " 120
```

O padrão é `auto`: GPU CUDA quando compilada e funcional, CPU caso contrário. O diagnóstico de seleção é escrito em stderr. Para controlar explicitamente:

```bash
bash scripts/ia.sh --device cpu avaliar dados/validacao.txt checkpoints/demonstracao_cuda.txt
bash scripts/ia.sh --device gpu avaliar dados/validacao.txt checkpoints/demonstracao_cuda.txt
```

O reconhecimento usa a API CUDA, não apenas a saída do `nvidia-smi`. O autoteste executa um kernel e confere o resultado transferido para a CPU. `--hardware` também mostra o processador, a quantidade de processadores disponíveis e a consulta ao driver NVIDIA. A presença de uma GPU não garante que ela esteja habilitada: `CUDA_VISIBLE_DEVICES` pode ocultá-la.

Um erro de inicialização em modo automático permite fallback. Uma falha durante treinamento/inferência é reportada e interrompe o processo, preservando a consistência dos parâmetros. O modo GPU explícito nunca troca silenciosamente para CPU.

## Implementação

- `backend/cuda.cu`: forward, derivada ReLU, backpropagation, Adam, transferências e autoteste.
- `backend/ponte.hpp`: seleção do dispositivo e intercâmbio dos tensores GhostLang com vetores reais da GPU.
- `scripts/toolchain/g++`: inclui a ponte e liga os objetos apropriados sem editar o C++ gerado ou os fontes da linguagem.
- `build/ia`: execução nativa CPU; `build/ia_cuda`: execução CUDA com fallback CPU.
- `build/backends.txt`: variantes válidas da última compilação; evita escolher um binário GPU antigo depois de uma compilação somente CPU.

CUDA usa precisão double, ReLU na camada oculta, saída linear e o mesmo Adam da CPU (beta1=0,9, beta2=0,999, epsilon=1e-8). Os deltas são calculados antes de atualizar qualquer peso. Desativamos fusão de multiplicação/soma na compilação CUDA para facilitar a comparação numérica. Os kernels atendem exclusivamente a rede 168 → 32 → 42 com 6.794 parâmetros reais. Não oferecem uma implementação geral de todas as operações GhostLang.

Pesos e momentos de Adam permanecem no dispositivo. A cada amostra, são enviados a entrada e o alvo; a avaliação copia de volta a saída. Antes de salvar, os pesos e biases voltam às matrizes GhostLang. O checkpoint `GHOST_TEXT_V1` permanece igual e funciona nos dois backends. Continuar de um checkpoint reinicia os momentos do Adam, como na versão CPU.

O runtime CUDA é ligado estaticamente; a execução ainda requer driver NVIDIA compatível para usar GPU. Não instalamos pacotes nem alteramos drivers. A compilação usa a arquitetura local detectada pelo nvcc. Para distribuir para outra GPU, recompile ou configure `IA_CUDA_ARCH` com uma arquitetura suportada pelo toolkit e pelo destino.

## Validação executada em 25/09/2026

Hardware: NVIDIA GeForce RTX 4050 Laptop GPU, driver 595.91.07, toolkit CUDA 13.3.33, g++ 15.2.0, Ubuntu 26.04.1. O driver consultado por `nvidia-smi` anuncia compatibilidade CUDA 13.2; a compilação local e a execução dos kernels CUDA 13.3 foram efetivamente testadas com sucesso.

O driver reporta 6.141 MiB físicos, e a API CUDA reporta 5.772 MiB em `totalGlobalMem`; esses números vêm de consultas distintas. O processador é o Intel i7-13620H com 16 processadores lógicos disponíveis.

Testes concluídos:

- Suíte de aprendizado completa em CPU e em GPU: ambas aprovadas, com MSE controlado 0,03800840 → 0,00038140.
- Modo automático escolheu CUDA; CPU explícita escolheu o runtime nativo.
- Com `CUDA_VISIBLE_DEVICES` vazio, automático voltou à CPU e GPU explícita retornou erro.
- Treinamento idêntico de três épocas nos dois backends: diferença máxima observada entre os pesos salvos igual a zero neste teste.
- Checkpoints treinados em ambos os backends foram avaliados e usados para gerar texto nos dois dispositivos, com resultados equivalentes.

O arquivo `checkpoints/demonstracao_cuda.txt` é o modelo demonstrativo treinado na GPU por 12 épocas. O CSV adjacente contém as métricas. A qualidade de texto continua limitada pelo tamanho do modelo e pelo corpus; trocar o dispositivo não amplia a capacidade da rede.

Para repetir os testes:

```bash
python3 tests/verificar.py
IA_TEST_DEVICE=gpu python3 tests/verificar.py
python3 tests/backends.py
```

Os testes de CUDA exigem uma GPU acessível. A reprodução bit a bit observada aqui não é uma garantia para outras GPUs, compiladores ou versões do runtime.
