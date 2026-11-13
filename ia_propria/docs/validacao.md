# Validação inicial em CPU — 25/09/2026

Registro histórico da primeira entrega. A GPU foi corrigida e a validação posterior com CUDA está em [cuda.md](cuda.md).

Compilação concluída no Ubuntu 26.04.1 com g++ 15.2.0 e OpenMP. O compilador foi construído a partir dos fontes originais, sem alterações, dentro de `ia_propria/build/`.

## Modelo demonstrativo pronto

Já existe um checkpoint treinado em `checkpoints/demonstracao.txt`. A partir de `ia_propria/`:

```bash
bash scripts/ia.sh gerar checkpoints/demonstracao.txt "ola " 120
bash scripts/ia.sh avaliar dados/validacao.txt checkpoints/demonstracao.txt
```

Treinamento: 12 épocas, taxa 0.003, Adam nativo, semente 42, uma thread de CPU, corpus fornecido em `dados/`.

| Medida | Resultado |
|---|---:|
| MSE inicial de treino (saída arredondada) | 0,037351 |
| MSE final de treino (saída arredondada) | 0,00999601 |
| MSE de validação após recarregar checkpoint | 0,017398799243 |
| Acurácia de próximo caractere na validação | 50,877% |

O CSV completo está em `checkpoints/demonstracao.txt.metricas.csv`. O menor MSE de validação ocorreu antes da última época; o checkpoint desta versão armazena o estado final. Mais épocas não garantem melhor generalização.

A geração testada com `ola ` começa com `ola mundo.` e depois apresenta erros e repetições. Esses resultados demonstram aprendizado e persistência funcionando, não qualidade de conversação. O conjunto de validação é minúsculo e do mesmo domínio dos exemplos, portanto a acurácia não é uma medida de capacidade geral.

## Testes automatizados

Comando executado: `python3 tests/verificar.py`.

Resultado: aprovado. No corpus controlado dos testes, o MSE caiu de 0,03800840 para 0,00038140. Foram verificados pesos e biases após recarga, comprimento/prefixo de geração, normalização de português, reprodução com a mesma semente, continuação, preservação de checkpoint existente e erros para dados/argumentos/checkpoints inválidos. Os arquivos temporários desses testes foram removidos pelo próprio teste.

Os scripts também passaram na verificação de sintaxe `bash -n`.

## Limites de hardware observados

O treinamento foi executado em CPU. `nvidia-smi` falhou com `Driver/library version mismatch` e informou biblioteca NVML 595.91. A VRAM não foi aferida; não houve uso comprovado da GPU, instalação de pacotes ou mudanças nos drivers.

Toda criação e adaptação desta entrega ficou em `ia_propria/`. Os arquivos originais da linguagem e o GGUF existente em `modelo_ia/` não foram editados.
