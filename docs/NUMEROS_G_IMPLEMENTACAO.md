# Números Fantasmas G: comportamento verificado no código

Este registro descreve `src/GhostNumberG.cpp` e distingue implementação e documentação. Não redefine a teoria nem modifica suas operações. É uma fonte de orientação para revisar o conhecimento da IA V3.

O manual, na seção de álgebra 4D, descreve `a + bi + cg + dθ`. Já o produto no código comenta `g = (x + i y) + pθ`, e os operadores principais retornam `theta2=0`. O campo `theta2` aparece em outros métodos, mas não se comporta nesses operadores como uma quarta coordenada aditiva independente.

| Operação implementada | Resultado |
|---|---|
| Soma / subtração | Opera x, y e p componente a componente; theta2=0 |
| Produto | x=x1x2−y1y2; y=x1y2+y1x2; p=x1p2+x2p1; theta2=0 |
| Conjugação | (x,−y,p,theta2) |
| `norm_squared()` | x²+y²+p²·theta2 |
| `real_norm_squared()` | x²+y² |
| `rotate()` | Muda (x,y,p) para (y,p,x) no próprio objeto |

Para o fantasma puro θ=(0,0,1), o produto dá θ²=0. Também dá iθ=0, para i=(0,1,0). Isso tem uma consequência verificável: **o produto atual não é associativo em geral**, pois `(i*i)*θ = -θ` e `i*(i*θ) = 0`. Portanto, a V3 não deve apresentá-lo como uma álgebra complexa dual associativa sem essa ressalva.

Na divisão, se as partes x e y do divisor têm módulo menor que 1e-15, o código retorna (0,0,p_resultado,0). A componente p_resultado recebe a magnitude complexa do numerador quando maior que 1e-15, ou o p original caso contrário. Por essa convenção, `GhostNumberG(1.0) / GhostNumberG(0.0)` retorna (0,0,1,0), e (3,4,2) dividido por zero retorna (0,0,5,0). Isso descreve uma regra do projeto, não uma identidade da divisão comum. No caminho regular, usa o método `inverse()`; não se deve inferir automaticamente todas as propriedades de um inverso de corpo.

Há também diferenças entre construtores: `GhostNumberG(int)` inicializa `value` mas deixa x=0; `GhostNumberG(double)` inicializa x com o escalar. Os exemplos e testes desta integração usam literais explícitos de ponto flutuante. Essa diferença preexistente foi preservada.

`tests/cpp/native_runtime.cpp` verifica exemplos das regras acima e compara produtos matriciais CPU/CUDA para x, y, p e theta2, incluindo dimensões que não são múltiplas do bloco CUDA. O backend novo reproduz o comportamento atual. Corrigir ou reformular a álgebra exige uma decisão separada sobre as definições desejadas.
