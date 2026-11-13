import "libs/test.g";

function void test_quantum_advanced() {
    qreg q(4);
    q.x(0);
    q.x(1);
    q.x(2);
    q.mcx(0, 1, 2, 3);

    assert_eq_double(q.prob(3), 1.0, 0.0001, "qubit 3 em |1>");
    assert_eq_double(q.expect_z(3), -1.0, 0.0001, "<Z> do qubit 3 deve ser -1");

    var c = q.counts(100);
    assert_eq_int(c["1111"], 100, "100 shots em |1111>");

    qreg r(2);
    r.h(0);
    r.s(0);
    r.sdg(0);
    r.t(0);
    r.tdg(0);
    r.sx(0);
    r.p(0, 0.5);
    r.rx(0, 0.5);
    r.ry(0, 0.5);
    r.rz(0, 0.5);
    r.rzz(0, 1, 0.5);
    r.reset();

    assert_eq_double(r.prob(0), 0.0, 0.0001, "apos reset estado em |0>");

    print("test_quantum_advanced: OK");
}

function void main() {
    test_quantum_advanced();
    print("todos os testes quanticos avancados passaram com sucesso!");
}
