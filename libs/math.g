// libs/math.g - Biblioteca Matematica Completa da GhostLang
// Fornece constantes, aritmetica avancada, teoria dos numeros, exponenciais,
// logaritmos, trigonometria direta e inversa, funcoes hiperbolicas,
// potencias, raizes de ordem arbitraria (nth_root) e funcoes especiais.

// ============================================================================
// 1. Constantes Matematicas Fundamentais
// ============================================================================

function double const_pi() { return 3.14159265358979323846; }
function double const_tau() { return 6.28318530717958647692; }
function double const_e() { return 2.71828182845904523536; }
function double const_phi() { return 1.61803398874989484820; }
function double const_sqrt2() { return 1.41421356237309504880; }
function double const_sqrt3() { return 1.73205080756887729352; }
function double const_ln2() { return 0.69314718055994530941; }
function double const_ln10() { return 2.30258509299404568402; }
function double const_log2e() { return 1.44269504088896340736; }
function double const_log10e() { return 0.43429448190325182765; }

// ============================================================================
// 2. Operacoes Aritmeticas Basicas e Teoria dos Numeros
// ============================================================================

function double soma(double a, double b) { return a + b; }
function double subtracao(double a, double b) { return a - b; }
function double multiplicacao(double a, double b) { return a * b; }
function double divisao(double a, double b) {
    if (b == 0.0) { throw "divisao por zero"; }
    return a / b;
}

function int soma_inteira(int a, int b) { return a + b; }
function int subtracao_inteira(int a, int b) { return a - b; }
function int multiplicacao_inteira(int a, int b) { return a * b; }
function int divisao_inteira(int a, int b) {
    if (b == 0) { throw "divisao inteira por zero"; }
    return a / b;
}
function int resto_inteiro(int a, int b) {
    if (b == 0) { throw "modulo por zero"; }
    return a % b;
}

function double resto_divisao(double a, double b) {
    if (b == 0.0) { throw "modulo flutuante por zero"; }
    return fmod(a, b);
}

function double absoluto(double x) { return abs(x); }
function double sinal(double x) {
    if (x > 0.0) { return 1.0; }
    if (x < 0.0) { return -1.0; }
    return 0.0;
}

function bool eh_par(int n) { return (n % 2) == 0; }
function bool eh_impar(int n) { return (n % 2) != 0; }

function bool eh_primo(int n) {
    if (n <= 1) { return false; }
    if (n <= 3) { return true; }
    if (n % 2 == 0 || n % 3 == 0) { return false; }
    int i = 5;
    while (i * i <= n) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
        i = i + 6;
    }
    return true;
}

function int proximo_primo(int n) {
    if (n <= 1) { return 2; }
    int primo = n + 1;
    while (!eh_primo(primo)) {
        primo = primo + 1;
    }
    return primo;
}

function int mdc(int a, int b) {
    if (a < 0) { a = -a; }
    if (b < 0) { b = -b; }
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

function int mmc(int a, int b) {
    if (a == 0 || b == 0) { return 0; }
    int m = mdc(a, b);
    return (a / m) * b;
}

function double fatorial(int n) {
    if (n < 0) { throw "fatorial nao definido para numeros negativos"; }
    double res = 1.0;
    for (int i = 2; i <= n; ++i) {
        res = res * double(i);
    }
    return res;
}

function double permutacao(int n, int k) {
    if (k < 0 || k > n) { return 0.0; }
    double res = 1.0;
    for (int i = n - k + 1; i <= n; ++i) {
        res = res * double(i);
    }
    return res;
}

function double combinacao(int n, int k) {
    if (k < 0 || k > n) { return 0.0; }
    if (k == 0 || k == n) { return 1.0; }
    if (k > n - k) { k = n - k; }
    double res = 1.0;
    for (int i = 1; i <= k; ++i) {
        res = res * double(n - k + i);
        res = res / double(i);
    }
    return res;
}

// ============================================================================
// 3. Potenciacao e Raizes (incluindo Raiz n-esima Universal)
// ============================================================================

function double potencia(double base, double expoente) {
    return base ** expoente;
}

function int potencia_inteira(int base, int expoente) {
    if (expoente < 0) { return 0; }
    int resultado = 1;
    int b = base;
    int exp = expoente;
    while (exp > 0) {
        if (exp % 2 == 1) {
            resultado = resultado * b;
        }
        b = b * b;
        exp = exp / 2;
    }
    return resultado;
}

// Raiz quadrada no dominio real
function double raiz_quadrada(double numero) {
    if (numero < 0.0) { throw "raiz quadrada de numero negativo no dominio real nao existe; utilize csqrt(numero), raiz_quadrada(complex) ou raiz_quadrada_complexa(numero)"; }
    return sqrt(numero);
}

// Raiz quadrada no dominio complexo (suporta radicando negativo ou complexo)
function complex raiz_quadrada(complex z) {
    return sqrt(z);
}

function complex raiz_quadrada_complexa(complex z) {
    return sqrt(z);
}

function complex csqrt(complex z) {
    return sqrt(z);
}

// Raiz cubica no dominio real
function double raiz_cubica(double numero) {
    return cbrt(numero);
}

// Raiz cubica no dominio complexo
function complex raiz_cubica(complex z) {
    return ghost_nth_root_complex(z, 3);
}

function complex raiz_cubica_complexa(complex z) {
    return ghost_nth_root_complex(z, 3);
}

function complex ccbrt(complex z) {
    return ghost_nth_root_complex(z, 3);
}

// Raiz n-esima com radicando x e indice n (n natural >= 1) no dominio real
function double raiz_n(double x, int n) {
    return ghost_nth_root(x, n);
}

// Raiz n-esima universal no dominio complexo (suporta radicando negativo ou complexo)
function complex raiz_n(complex z, int n) {
    return ghost_nth_root_complex(z, n);
}

function complex raiz_n_complexa(complex z, int n) {
    return ghost_nth_root_complex(z, n);
}

function double nth_root(double x, int n) {
    return ghost_nth_root(x, n);
}

function complex nth_root(complex z, int n) {
    return ghost_nth_root_complex(z, n);
}

function complex nth_root_c(complex z, int n) {
    return ghost_nth_root_complex(z, n);
}

function double root(double x, int n) {
    return ghost_nth_root(x, n);
}

function complex root(complex z, int n) {
    return ghost_nth_root_complex(z, n);
}

function complex croot(complex z, int n) {
    return ghost_nth_root_complex(z, n);
}

function double hipotenusa(double a, double b) {
    return hypot(a, b);
}

// ============================================================================
// 4. Exponenciais e Logaritmos
// ============================================================================

function double exponencial(double x) { return exp(x); }
function double exponencial2(double x) { return exp2(x); }
function double expm1_fn(double x) { return expm1(x); }

function double logaritmo_natural(double numero) {
    if (numero <= 0.0) { throw "logaritmo de numero menor ou igual a zero nao e real"; }
    return log(numero);
}

function double logaritmo_base10(double numero) {
    if (numero <= 0.0) { throw "logaritmo de numero menor ou igual a zero nao e real"; }
    return log10(numero);
}

function double logaritmo_base2(double numero) {
    if (numero <= 0.0) { throw "logaritmo de numero menor ou igual a zero nao e real"; }
    return log2(numero);
}

function double logaritmo_base(double numero, double base) {
    if (numero <= 0.0 || base <= 0.0 || base == 1.0) {
        throw "base ou argumento de logaritmo invalido";
    }
    return log(numero) / log(base);
}

function double log1p_fn(double x) { return log1p(x); }

// ============================================================================
// 5. Trigonometria Direta e Inversa
// ============================================================================

function double graus_para_radianos(double graus) {
    return graus * (3.14159265358979323846 / 180.0);
}

function double radianos_para_graus(double radianos) {
    return radianos * (180.0 / 3.14159265358979323846);
}

function double seno(double radianos) { return sin(radianos); }
function double cosseno(double radianos) { return cos(radianos); }
function double tangente(double radianos) { return tan(radianos); }

function double cotangente(double radianos) {
    double t = tan(radianos);
    if (t == 0.0) { throw "cotangente indefinida (divisao por zero)"; }
    return 1.0 / t;
}

function double secante(double radianos) {
    double c = cos(radianos);
    if (c == 0.0) { throw "secante indefinida (divisao por zero)"; }
    return 1.0 / c;
}

function double cossecante(double radianos) {
    double s = sin(radianos);
    if (s == 0.0) { throw "cossecante indefinida (divisao por zero)"; }
    return 1.0 / s;
}

function double arco_seno(double x) {
    if (x < -1.0 || x > 1.0) { throw "dominio de arco seno deve estar entre -1 e 1"; }
    return asin(x);
}

function double arco_cosseno(double x) {
    if (x < -1.0 || x > 1.0) { throw "dominio de arco cosseno deve estar entre -1 e 1"; }
    return acos(x);
}

function double arco_tangente(double x) { return atan(x); }
function double arco_tangente2(double y, double x) { return atan2(y, x); }

// ============================================================================
// 6. Funcoes Hiperbolicas e Inversas
// ============================================================================

function double seno_hiperbolico(double x) { return sinh(x); }
function double cosseno_hiperbolico(double x) { return cosh(x); }
function double tangente_hiperbolica(double x) { return tanh(x); }
function double arco_seno_hiperbolico(double x) { return asinh(x); }
function double arco_cosseno_hiperbolico(double x) {
    if (x < 1.0) { throw "dominio de arco cosseno hiperbolico deve ser x >= 1"; }
    return acosh(x);
}
function double arco_tangente_hiperbolica(double x) {
    if (x <= -1.0 || x >= 1.0) { throw "dominio de arco tangente hiperbolica deve ser |x| < 1"; }
    return atanh(x);
}

// ============================================================================
// 7. Arredondamento e Utilitarios
// ============================================================================

function double piso(double x) { return floor(x); }
function double teto(double x) { return ceil(x); }
function double arredondar(double x) { return round(x); }
function double truncar(double x) { return trunc(x); }

function double clamp(double val, double min_val, double max_val) {
    if (val < min_val) { return min_val; }
    if (val > max_val) { return max_val; }
    return val;
}

function double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

// ============================================================================
// 8. Funcoes Especiais e Ativacoes Neurais
// ============================================================================

function double funcao_gama(double x) { return tgamma(x); }
function double log_gama(double x) { return lgamma(x); }

function double funcao_beta(double a, double b) {
    if (a <= 0.0 || b <= 0.0) { throw "parametros da funcao beta devem ser positivos"; }
    return (tgamma(a) * tgamma(b)) / tgamma(a + b);
}

function double funcao_erro(double x) { return erf(x); }
function double funcao_erro_complementar(double x) { return erfc(x); }

function double ativacao_sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

function double ativacao_relu(double x) {
    if (x > 0.0) { return x; }
    return 0.0;
}

function double gelu(double x) {
    double sqrt_2_pi = 0.7978845608028654;
    return 0.5 * x * (1.0 + tanh(sqrt_2_pi * (x + 0.044715 * (x ** 3))));
}

function void exibir_cabecalho_matematico() {
    println("=================================================");
    println(" GhostMath Lib v2.0 - Motor Matematico Especialista");
    println("=================================================");
}
