// Alvo do teste do depurador (tests/cli/test_dap.py): breakpoint na linha 3 (dentro de soma).
function int soma(int a, int b) {
    int c = a + b;
    return c;
}

function main() {
    int x = 2;
    int y = soma(x, 5);
    print("y = ", y);
}
