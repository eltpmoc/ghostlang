// Execute na raiz do projeto; requer os arquivos de corpus da V3.
function void main() {
    pragma auto;
    textmodel ia = text_model(8, 8, 24, 16, 2048, 42);
    ia.prepare("ia_propria/v3/dados/treino.txt", "ia_propria/v3/dados/validacao.txt");
    double perda = ia.epoch(0.001);
    print("Erro medio no treino: " + perda);
    print("Erro na validacao: " + ia.validate());
    print("Uma epoca demonstra a API; ainda nao cria um bom conversador.");
}
