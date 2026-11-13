function main() {
    // ---- 1. Operador de potencia ** ----
    print("--- potencia ---");
    var p1 = 2 ** 3;
    print(p1); // 8

    var p2 = 3 ** 4;
    print(p2); // 81

    // Associatividade a direita: 2 ** (3 ** 2) = 2 ** 9 = 512
    var p3 = 2 ** 3 ** 2;
    print(p3); // 512

    // Com expressoes entre parenteses
    var base = 2;
    var exp = 5;
    var p4 = base ** exp;
    print(p4); // 32

    var p5 = (1 + 1) ** (2 + 1);
    print(p5); // 8

    // Precedencia: ** tem maior precedencia que * e +
    var p6 = 3 * 2 ** 3 + 1; // 3 * 8 + 1 = 25
    print(p6); // 25

    // ---- 2. Conversao explicita bool(x) ----
    print("--- bool ---");
    print(bool(0));      // false
    print(bool(1));      // true
    print(bool(42));     // true
    print(bool(-5));     // true
    print(bool(0.0));    // false
    print(bool(3.14));   // true
    print(bool(""));     // false
    print(bool("ghost"));// true

    list<int> l_vazia = [];
    list<int> l_cheia = [10, 20];
    print(bool(l_vazia));// false
    print(bool(l_cheia));// true

    // ---- 3. foreach em registrador quantico qreg ----
    print("--- qreg foreach ---");
    qreg qr(4);
    foreach (q in qr) {
        print(q);
    }

    foreach (int qubit in qr) {
        print(qubit * 10);
    }
}
