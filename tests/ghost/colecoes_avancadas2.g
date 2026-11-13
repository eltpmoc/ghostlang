// Teste de fatiamento com passo [start:end:step] e operações vetoriais/matriciais em lote

function main() {
    print("== fatiamento com passo em listas ==");
    list<int> xs = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
    print(xs[::2]);
    print(xs[1::2]);
    print(xs[::-1]);
    print(xs[2:8:2]);
    print(xs[:5]);
    print(xs[6:]);
    print(xs[-4:]);
    print(xs[:-3]);
    print(xs[8:2:-2]);

    print("== fatiamento com passo em textos ==");
    string s = "GhostLang";
    print(s[::2]);
    print(s[::-1]);
    print(s[0:5]);
    print(s[5:]);
    print(s[1:7:2]);

    print("== operacoes vetoriais em lote ==");
    list<int> v1 = [1, 2, 3, 4];
    list<int> v2 = [10, 20, 30, 40];
    print("soma: ", vec_add(v1, v2));
    print("sub: ", vec_sub(v2, v1));
    print("mul: ", vec_mul(v1, v2));
    print("scale: ", vec_scale(v1, 2.5));
    print("dot: ", dot(v1, v2));
    print("norm: ", v1.norm());

    print("== metodos fluentes de vetor em listas ==");
    print("v1.vec_add: ", v1.vec_add(v2));
    print("v2.vec_sub: ", v2.vec_sub(v1));
    print("v1.vec_mul: ", v1.vec_mul(v2));
    print("v1.vec_scale: ", v1.vec_scale(0.5));
    print("v1.dot: ", v1.dot(v2));
    print("v1.slice com step: ", v1.slice(0, 4, 2));

    print("== operacoes matriciais em lote ==");
    // Multiplicação de matriz linear 2x2 por 2x2:
    // A = [[1, 2], [3, 4]] -> [1, 2, 3, 4]
    // B = [[5, 6], [7, 8]] -> [5, 6, 7, 8]
    // C = A * B = [[19, 22], [43, 50]] -> [19, 22, 43, 50]
    list<double> mA = [1.0, 2.0, 3.0, 4.0];
    list<double> mB = [5.0, 6.0, 7.0, 8.0];
    print("mat_mul: ", mat_mul(mA, mB, 2, 2, 2));
    print("transpose: ", mat_transpose(mA, 2, 2));

    // Matrizes 2D aninhadas
    list<list<double>> A2D = [[1.0, 2.0], [3.0, 4.0]];
    list<list<double>> B2D = [[2.0, 0.0], [1.0, 2.0]];
    print("mat2d_mul: ", mat2d_mul(A2D, B2D));
    print("fim colecoes avancadas 2");
}
