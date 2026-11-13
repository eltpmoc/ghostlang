// Vários erros de sintaxe: a AST continua a leitura depois de cada um e mostra todos de uma vez
function main() {
    int x = 5
    print("x = ", x);
    int y = (3 + 4;
    if (x > 2 {
        print("maior");
    }
    double z = * 2;
    print("fim");
}
