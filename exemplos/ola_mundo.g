import "libs/quantum.g";

function void main() {
    string nome = "Pioneiro Quântico";
    int ano = 2026;
    
    println(f"Bem-vindo ao GhostLang, {nome}! Ano de consolidação da linguagem: {ano}.");
    
    // Criando um qubit em superposição instantânea
    qreg q(1);
    q.h(0); // Porta Hadamard
    
    println("Estado do qubit |ψ⟩:");
    q.show();
}
