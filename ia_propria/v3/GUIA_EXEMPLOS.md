# Explicações e exemplos locais da V3.1

## GhostLang

Um programa GhostLang é escrito em arquivo `.g`. O compilador lê a sintaxe, produz C++ e chama o compilador C++ para gerar um executável. O ponto de entrada pode ser `function void main()`. `string` guarda texto, `int` guarda inteiros, e `print` mostra valores no terminal. Matrizes, estados, redes e Números G têm tipos específicos; `textmodel` oferece um modelo de linguagem treinável.

`exemplos/ola.g` declara um nome, calcula o dobro de cinco e imprime duas linhas. Não exige modelo treinado nem placa NVIDIA. Esse é um exemplo completo de entrada, variáveis, cálculo e saída; aqui "entrada" significa ponto de entrada do programa, não leitura de dados do usuário.

## Matrizes

`state` representa um vetor coluna; `gate` representa uma matriz. Em `exemplos/matrizes.g`, os pesos são preenchidos explicitamente: a diagonal é 1 e 2, os demais valores são zero. Multiplicar essa matriz por [1,2] gera [1,4]. `pragma auto` seleciona CPU ou CUDA conforme disponibilidade e tamanho da operação; essa matriz pequena pode usar CPU.

## Números G

Em `exemplos/numeros_g.g`, a=(1,2,3) e b=(4,5,6) usam componentes x, y e p. A soma produz (5,7,9). O produto produz (-6,13,18), calculado por x=1*4-2*5, y=1*5+2*4 e p=1*6+4*3. Ambos os resultados têm theta2=0 na implementação atual. `exemplos/nilpotente.g` mostra que o fantasma puro (0,0,1) multiplicado por si mesmo resulta em três componentes nulas.

Esses exemplos executam as regras existentes. O parâmetro theta2 não é preservado por todos os operadores, e o produto não é associativo em geral. Consulte `docs/NUMEROS_G_IMPLEMENTACAO.md` antes de atribuir outras propriedades à teoria. Nada nesta atualização modifica a álgebra.

## Modelo de texto

`exemplos/textmodel.g` define uma rede pequena com contexto 8, embedding 8, camada oculta 24, lote 16, vocabulário máximo 2048 e semente 42. Carrega os corpus existentes, executa uma época e avalia o resultado. O exemplo demonstra a API e não grava arquivos nem fornece um conversador pronto. Um treinamento útil requer dados variados, avaliação de respostas completas e mais trabalho que uma chamada isolada de `epoch`.

## CUDA

`exemplos/cuda.g` usa seleção automática, mostra o dispositivo e chama o autoteste do modelo. Quando CUDA é selecionada, o teste compara perda, gradientes e atualização AdamW com CPU. A seleção é feita antes de preparar/carregar o modelo. Auto não promete aceleração para todo tamanho de entrada, e o texto da interface permanece na CPU.

## Limite dos exemplos

São exemplos revisados, compilados e executados pelo teste `testar_respostas.py`. A consulta documental apresenta esses arquivos; ela não sintetiza programas arbitrários. Os trechos produzidos por `/neural` não recebem essa garantia. Pedidos sem cobertura, como Vulkan, servidores HTTP ou integração com outra linguagem, devem ser recusados ou ter a parte não atendida identificada.
