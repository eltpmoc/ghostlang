// libs/stats.g - Biblioteca de Estatistica e Matematica Aplicada da GhostLang
// Fornece medidas de tendencia central, dispersao, posicao, momentos estatisticos,
// correlacao bivariada (Pearson e Spearman), regressao linear e distribuicoes.

import "libs/math.g";

// ============================================================================
// 1. Tendencia Central
// ============================================================================

function double media(list<double> xs) {
    if (xs.length() == 0) { return 0.0; }
    double soma = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        soma = soma + xs[i];
    }
    return soma / double(xs.length());
}

function double mediana(list<double> xs) {
    if (xs.length() == 0) { return 0.0; }
    list<double> ord = xs.sorted();
    int n = ord.length();
    if (n % 2 != 0) {
        return ord[n / 2];
    }
    return (ord[(n / 2) - 1] + ord[n / 2]) / 2.0;
}

function double moda(list<double> xs) {
    if (xs.length() == 0) { return 0.0; }
    list<double> ord = xs.sorted();
    double melhor_val = ord[0];
    int max_freq = 1;
    double val_atual = ord[0];
    int freq_atual = 1;
    for (int i = 1; i < ord.length(); ++i) {
        if (ord[i] == val_atual) {
            freq_atual = freq_atual + 1;
        } else {
            if (freq_atual > max_freq) {
                max_freq = freq_atual;
                melhor_val = val_atual;
            }
            val_atual = ord[i];
            freq_atual = 1;
        }
    }
    if (freq_atual > max_freq) {
        melhor_val = val_atual;
    }
    return melhor_val;
}

function double media_ponderada(list<double> xs, list<double> pesos) {
    if (xs.length() == 0 || xs.length() != pesos.length()) { return 0.0; }
    double soma_prod = 0.0;
    double soma_pesos = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        soma_prod = soma_prod + xs[i] * pesos[i];
        soma_pesos = soma_pesos + pesos[i];
    }
    if (soma_pesos == 0.0) { return 0.0; }
    return soma_prod / soma_pesos;
}

function double media_geometrica(list<double> xs) {
    if (xs.length() == 0) { return 0.0; }
    double soma_log = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        if (xs[i] <= 0.0) {
            throw "media geometrica requer todos os valores estritamente positivos";
        }
        soma_log = soma_log + log(xs[i]);
    }
    return exp(soma_log / double(xs.length()));
}

function double media_harmonica(list<double> xs) {
    if (xs.length() == 0) { return 0.0; }
    double soma_inv = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        if (xs[i] == 0.0) {
            throw "media harmonica indefinida para elementos iguais a zero";
        }
        soma_inv = soma_inv + (1.0 / xs[i]);
    }
    return double(xs.length()) / soma_inv;
}

function double media_aparada(list<double> xs, double proporcao) {
    if (xs.length() == 0) { return 0.0; }
    if (proporcao <= 0.0) { return media(xs); }
    if (proporcao >= 0.5) { return mediana(xs); }
    list<double> ord = xs.sorted();
    int k = int(double(ord.length()) * proporcao);
    double soma = 0.0;
    int cont = 0;
    for (int i = k; i < ord.length() - k; ++i) {
        soma = soma + ord[i];
        cont = cont + 1;
    }
    if (cont == 0) { return mediana(xs); }
    return soma / double(cont);
}

// ============================================================================
// 2. Medidas de Dispersao e Extremos
// ============================================================================

function double minimo(list<double> xs) {
    if (xs.length() == 0) { return 0.0; }
    double m = xs[0];
    for (int i = 1; i < xs.length(); ++i) {
        if (xs[i] < m) { m = xs[i]; }
    }
    return m;
}

function double maximo(list<double> xs) {
    if (xs.length() == 0) { return 0.0; }
    double m = xs[0];
    for (int i = 1; i < xs.length(); ++i) {
        if (xs[i] > m) { m = xs[i]; }
    }
    return m;
}

function double amplitude(list<double> xs) {
    if (xs.length() == 0) { return 0.0; }
    return maximo(xs) - minimo(xs);
}

function double variancia(list<double> xs) {
    if (xs.length() <= 1) { return 0.0; }
    double m = media(xs);
    double soma_sq = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        double d = xs[i] - m;
        soma_sq = soma_sq + d * d;
    }
    return soma_sq / double(xs.length() - 1);
}

function double variancia_populacional(list<double> xs) {
    if (xs.length() == 0) { return 0.0; }
    double m = media(xs);
    double soma_sq = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        double d = xs[i] - m;
        soma_sq = soma_sq + d * d;
    }
    return soma_sq / double(xs.length());
}

function double desvio_padrao(list<double> xs) {
    return sqrt(variancia(xs));
}

function double desvio_padrao_populacional(list<double> xs) {
    return sqrt(variancia_populacional(xs));
}

function double erro_padrao(list<double> xs) {
    if (xs.length() == 0) { return 0.0; }
    return desvio_padrao(xs) / sqrt(double(xs.length()));
}

function double coeficiente_variacao(list<double> xs) {
    double m = media(xs);
    if (m == 0.0) { return 0.0; }
    return desvio_padrao(xs) / m;
}

function double desvio_medio_absoluto(list<double> xs) {
    if (xs.length() == 0) { return 0.0; }
    double m = media(xs);
    double soma = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        soma = soma + abs(xs[i] - m);
    }
    return soma / double(xs.length());
}

// ============================================================================
// 3. Medidas de Posicao e Forma (Quartis, Assimetria, Curtose)
// ============================================================================

function double quantil(list<double> xs, double q) {
    if (xs.length() == 0) { return 0.0; }
    if (q <= 0.0) { return minimo(xs); }
    if (q >= 1.0) { return maximo(xs); }
    list<double> ord = xs.sorted();
    double pos = q * double(ord.length() - 1);
    int idx = int(pos);
    double frac = pos - double(idx);
    if (idx + 1 < ord.length()) {
        return ord[idx] + frac * (ord[idx + 1] - ord[idx]);
    }
    return ord[idx];
}

function double percentil(list<double> xs, double p) {
    return quantil(xs, p / 100.0);
}

function list<double> quartis(list<double> xs) {
    list<double> q = [];
    q.push(quantil(xs, 0.25));
    q.push(quantil(xs, 0.50));
    q.push(quantil(xs, 0.75));
    return q;
}

function double intervalo_interquartil(list<double> xs) {
    return quantil(xs, 0.75) - quantil(xs, 0.25);
}

function double assimetria(list<double> xs) {
    if (xs.length() < 3) { return 0.0; }
    double m = media(xs);
    double s = desvio_padrao(xs);
    if (s == 0.0) { return 0.0; }
    double soma3 = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        double d = (xs[i] - m) / s;
        soma3 = soma3 + d * d * d;
    }
    int n = xs.length();
    return (double(n) / (double(n - 1) * double(n - 2))) * soma3;
}

function double curtose(list<double> xs) {
    if (xs.length() < 4) { return 0.0; }
    double m = media(xs);
    double s = desvio_padrao(xs);
    if (s == 0.0) { return 0.0; }
    double soma4 = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        double d = (xs[i] - m) / s;
        soma4 = soma4 + d * d * d * d;
    }
    int n = xs.length();
    double termo1 = (double(n) * double(n + 1)) / (double(n - 1) * double(n - 2) * double(n - 3));
    double termo2 = (3.0 * double(n - 1) * double(n - 1)) / (double(n - 2) * double(n - 3));
    return (termo1 * soma4) - termo2;
}

function list<double> z_scores(list<double> xs) {
    list<double> res = [];
    double m = media(xs);
    double s = desvio_padrao(xs);
    for (int i = 0; i < xs.length(); ++i) {
        if (s == 0.0) {
            res.push(0.0);
        } else {
            res.push((xs[i] - m) / s);
        }
    }
    return res;
}

function list<double> escalonar_min_max(list<double> xs) {
    list<double> res = [];
    double mn = minimo(xs);
    double mx = maximo(xs);
    double amp = mx - mn;
    for (int i = 0; i < xs.length(); ++i) {
        if (amp == 0.0) {
            res.push(0.0);
        } else {
            res.push((xs[i] - mn) / amp);
        }
    }
    return res;
}

// ============================================================================
// 4. Relacao Bivariada e Regressao Linear
// ============================================================================

function double covariancia(list<double> xs, list<double> ys) {
    if (xs.length() <= 1 || xs.length() != ys.length()) { return 0.0; }
    double mx = media(xs);
    double my = media(ys);
    double soma = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        soma = soma + (xs[i] - mx) * (ys[i] - my);
    }
    return soma / double(xs.length() - 1);
}

function double covariancia_populacional(list<double> xs, list<double> ys) {
    if (xs.length() == 0 || xs.length() != ys.length()) { return 0.0; }
    double mx = media(xs);
    double my = media(ys);
    double soma = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        soma = soma + (xs[i] - mx) * (ys[i] - my);
    }
    return soma / double(xs.length());
}

function double correlacao_pearson(list<double> xs, list<double> ys) {
    if (xs.length() <= 1 || xs.length() != ys.length()) { return 0.0; }
    double sx = desvio_padrao(xs);
    double sy = desvio_padrao(ys);
    if (sx == 0.0 || sy == 0.0) { return 0.0; }
    return covariancia(xs, ys) / (sx * sy);
}

// Retorna [inclinacao, intercepto, r2]
function list<double> regressao_linear(list<double> xs, list<double> ys) {
    list<double> res = [];
    if (xs.length() <= 1 || xs.length() != ys.length()) {
        res.push(0.0);
        res.push(0.0);
        res.push(0.0);
        return res;
    }
    double mx = media(xs);
    double my = media(ys);
    double var_x = 0.0;
    double cov_xy = 0.0;
    for (int i = 0; i < xs.length(); ++i) {
        double dx = xs[i] - mx;
        double dy = ys[i] - my;
        var_x = var_x + dx * dx;
        cov_xy = cov_xy + dx * dy;
    }
    if (var_x == 0.0) {
        res.push(0.0);
        res.push(my);
        res.push(0.0);
        return res;
    }
    double m = cov_xy / var_x;
    double b = my - m * mx;
    double r = correlacao_pearson(xs, ys);
    res.push(m);
    res.push(b);
    res.push(r * r);
    return res;
}

// ============================================================================
// 5. Distribuicoes de Probabilidade e Amostragem
// ============================================================================

function double normal_pdf(double x, double media_val, double desvio_val) {
    if (desvio_val <= 0.0) { throw "desvio padrao deve ser estritamente positivo"; }
    double inv_sqrt_2pi = 0.3989422804014327;
    double z = (x - media_val) / desvio_val;
    return (inv_sqrt_2pi / desvio_val) * exp(-0.5 * z * z);
}

function double normal_cdf(double x, double media_val, double desvio_val) {
    if (desvio_val <= 0.0) { throw "desvio padrao deve ser estritamente positivo"; }
    double z = (x - media_val) / (desvio_val * 1.4142135623730951);
    return 0.5 * (1.0 + erf(z));
}

function double poisson_pmf(int k, double lambda_val) {
    if (k < 0 || lambda_val <= 0.0) { return 0.0; }
    return (exp(-lambda_val) * (lambda_val ** double(k))) / fatorial(k);
}

function double binomial_pmf(int k, int n, double p) {
    if (k < 0 || k > n || p < 0.0 || p > 1.0) { return 0.0; }
    return combinacao(n, k) * (p ** double(k)) * ((1.0 - p) ** double(n - k));
}

function int lcg_next(int seed) {
    int a = 1664525;
    int c = 1013904223;
    return (a * seed + c) % 2147483647;
}

function double lcg_uniform(int seed) {
    int next_val = lcg_next(seed);
    if (next_val < 0) { next_val = -next_val; }
    return double(next_val) / 2147483647.0;
}
