import "src/modelo.g"

function void ajuda() {
    print("Ghost Texto v1 - modelo proprio, 6794 parametros, CPU/OpenMP ou CUDA");
    print("treinar TREINO VALIDACAO EPOCAS TAXA CHECKPOINT");
    print("continuar TREINO VALIDACAO EPOCAS TAXA ORIGEM DESTINO");
    print("avaliar CORPUS CHECKPOINT");
    print("gerar CHECKPOINT TEXTO QUANTIDADE");
    print("normalizar TEXTO");
}

function void main(string args[]) {
    if (args.size() == 0) {
        ajuda();
        return;
    }
    string comando = args[0];
    if (comando == "hardware") {
        print("Arquitetura: 168 -> 32 -> 42; dispositivo informado pelo backend.");
        return;
    }
    int condicao_1 = comando == "normalizar" && args.size() == 2;
    if (condicao_1) {
        print(normalizar(args[1]));
        return;
    }
    int condicao_2 = comando == "treinar" || comando == "continuar";
    if (condicao_2) {
        int esperado = 6;
        if (comando == "continuar") {
            esperado = 7;
        }
        if (args.size() != esperado) {
            ajuda();
            falhar("Argumentos de treinamento invalidos.");
        }
        int condicao_3 = !regex_match(args[3], "[0-9]+") || args[3].size() > 3;
        if (condicao_3) {
            falhar("Epocas deve ser um inteiro entre 1 e 200.");
        }
        int condicao_4 = !regex_match(args[4], "0[.][0-9]+") || args[4].size() > 12;
        if (condicao_4) {
            falhar("Taxa deve ser decimal, por exemplo 0.003.");
        }
        int epocas = to_int(args[3]);
        double taxa = parse_number(args[4]);
        int condicao_5 = epocas < 1 || epocas > 200 || taxa < 0.000001 || taxa > 0.05;
        if (condicao_5) {
            falhar("Use 1..200 epocas e taxa entre 0.000001 e 0.05.");
        }
        string destino = args[esperado - 1];
        if (file_exists(destino)) {
            falhar("O destino ja existe. Escolha um novo nome para preservar o treino anterior.");
        }
        string treino = ler_corpus(args[1]);
        string validacao = ler_corpus(args[2]);
        if (treino == validacao) {
            falhar("Use textos distintos para treino e validacao.");
        }
        if (comando == "continuar") {
            carregar_pesos(args[5]);
            print("Continuando dos pesos; os momentos do Adam reiniciam.");
        }
        treinar(treino, validacao, epocas, taxa, destino);
        return;
    }
    int condicao_6 = comando == "avaliar" && args.size() == 3;
    if (condicao_6) {
        string corpus = ler_corpus(args[1]);
        carregar_pesos(args[2]);
        double erro = avaliar(corpus);
        print("MSE=" + format_number(erro, 12) + " acuracia=" + format_number(ultima_acuracia, 12));
        return;
    }
    int condicao_7 = comando == "gerar" && args.size() == 4;
    if (condicao_7) {
        int condicao_8 = !regex_match(args[3], "[0-9]+") || args[3].size() > 4 || args[2].size() > 2000;
        if (condicao_8) {
            falhar("Use um inicio de ate 2000 bytes e uma quantidade inteira de caracteres.");
        }
        int quantidade = to_int(args[3]);
        int condicao_9 = quantidade < 1 || quantidade > 2000;
        if (condicao_9) {
            falhar("Gere entre 1 e 2000 caracteres por chamada.");
        }
        carregar_pesos(args[1]);
        print(gerar(args[2], quantidade));
        return;
    }
    ajuda();
    falhar("Comando ou argumentos invalidos.");
}
