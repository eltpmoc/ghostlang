function int main() {
    int escolhido = 0;
    hypothesis (var x in [10, 50, 30, 20]) {
        escolhido = x;
        collapse_max(x);
    }
    print("Escolhido final: " + escolhido);
    
    int menor = 0;
    hypothesis (var y in [100, 25, 75, 50]) {
        menor = y;
        collapse_min(y);
    }
    print("Menor final: " + menor);
    print("hypothesis_compilado: OK");
    return 0;
}
