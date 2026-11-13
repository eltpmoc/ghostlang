// x = conv(imagem, kernel[, passo]); e x = transpose(a); : formas de comando do Parser antigo,
// restauradas no verificador e no gerador da AST em 12/11/2026 (tinham deixado de compilar).
function main() {
    gate img(4, 4);
    img(0, 0) = 1.0; img(0, 1) = 2.0; img(0, 2) = 3.0; img(0, 3) = 0.0;
    img(1, 0) = 4.0; img(1, 1) = 5.0; img(1, 2) = 6.0; img(1, 3) = 1.0;
    img(2, 0) = 7.0; img(2, 1) = 8.0; img(2, 2) = 9.0; img(2, 3) = 2.0;
    img(3, 0) = 0.0; img(3, 1) = 1.0; img(3, 2) = 2.0; img(3, 3) = 3.0;

    gate k(2, 2);
    k(0, 0) = 1.0; k(0, 1) = 0.0;
    k(1, 0) = 0.0; k(1, 1) = 1.0;

    gate saida(3, 3);
    saida = conv(img, k);      // passo 1: saida(i, j) = img(i, j) + img(i+1, j+1)
    show(saida);

    gate saida2(2, 2);
    saida2 = conv(img, k, 2);  // passo 2
    show(saida2);

    gate t(4, 4);
    t = transpose(img);
    show(t);

    // dentro de expressões, os métodos fazem o mesmo
    var m = img.conv2d(k, 1);
    var u = img.transpose();
    print(m(0, 0), " ", u(0, 1));

    // matrizes chamadas r, c e val: até 12/11/2026 os laços gerados para gate usavam esses nomes e o g++ recusava
    gate r(2, 3);
    gate c(3, 3);
    gate val(2, 2);
    c = conv(img, k);
    print(r.getRows(), "x", r.getCols(), " ", c(0, 0), " ", val.getCols());
}
