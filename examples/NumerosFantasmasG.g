// exemplos/NumerosFantasmasG.g
// Álgebra Hiper-Complexa dos Números Fantasmas G (4D) em GhostLang

function void main() {
    // 1. Instanciação direta com o construtor ghost(x, y, p, theta2)
    ghost g1 = ghost(3.0, 4.0, 1.7, 2.0);
    ghost g2 = ghost(1.0, 2.0, 3.0, 4.0);

    println("=== Instanciação e Saída Padrão ===");
    println(f"Número fantasma g1: {g1}");
    println(f"Número fantasma g2: {g2}");

    // 2. Operações Fundamentais da Álgebra G (Soma e Produto)
    ghost soma = g1 + g2;
    ghost produto = g1 * g2;
    println("\n=== Operações Algébricas ===");
    println(f"Soma dos números fantasmas G: {soma}");
    println(f"Produto dos números fantasmas G: {produto}");

    // 3. Exibição Personalizada nos Formatos Suportados:
    println("\n=== Formatos de Exibição (4D Vetorial vs Matemático) ===");

    // Formato 4D Vetorial: G = (x, yi, p, θ²)
    println(f"g1 em formato 4D (via f-string): {g1:4d}");
    println("g1 em formato 4D (via to_4d): " + g1.to_4d());
    println("g1 com prefixo 4D: G = " + g1.to_4d());

    // Formato Matemático Algébrico: G = x + y·i + p·p + θ²
    println(f"g1 em formato Matemático (via f-string): {g1:math}");
    println("g1 em formato Matemático (via to_math): " + g1.to_math());
    println("g1 com prefixo Matemático: G = " + g1.to_math());

    println(f"\nSoma em formato 4D: {soma:4d}");
    println(f"Soma em formato Matemático: {soma:math}");

    println(f"\nProduto em formato 4D: {produto:4d}");
    println(f"Produto em formato Matemático: {produto:math}");

    // 4. Norma Real ao Quadrado (projeção clássica)
    println(f"\nNorma real ao quadrado de g1 (x² + y²): {g1.real_norm_squared()}");
}
