// GhostLang controla as etapas do treino, selecao, persistencia e interface.
// A extensao local implementa os tensores/gradientes CPU e CUDA da V2.
function void ajuda_v2() {
    print("Ghost Conversa V2 - modelo proprio de palavras em portugues");
    print("treinar TREINO VALIDACAO EPOCAS TAXA DESTINO");
    print("continuar TREINO VALIDACAO EPOCAS TAXA ORIGEM DESTINO");
    print("avaliar CORPUS CHECKPOINT");
    print("gerar CHECKPOINT PROMPT MAX_TOKENS");
    print("chat CHECKPOINT | info CHECKPOINT | hardware | autoteste");
}

function void main(string args[]) {
    v2_iniciar();
    if (args.size() == 0) {
        ajuda_v2();
        return;
    }
    string comando = args[0];
    if (comando == "hardware") {
        v2_info();
        return;
    }
    if (comando == "autoteste") {
        v2_autoteste();
        return;
    }
    int treino_solicitado = comando == "treinar" || comando == "continuar";
    if (treino_solicitado) {
        int tamanho = 6;
        string origem = "";
        if (comando == "continuar") {
            tamanho = 7;
        }
        if (args.size() != tamanho) {
            v2_falhar("Argumentos invalidos para treinamento.");
        }
        int epocas = v2_inteiro(args[3], 1, 200);
        double taxa = v2_decimal(args[4], 0.000001, 0.02);
        string destino = args[tamanho - 1];
        int existe = file_exists(destino) || file_exists(destino + ".ultimo") || file_exists(destino + ".metricas.csv");
        if (existe) {
            v2_falhar("Destino ja existe. Use um novo nome para preservar o modelo.");
        }
        if (comando == "continuar") {
            origem = args[5];
        }
        v2_preparar(args[1], args[2], origem);
        double melhor = v2_validar();
        print("CE inicial validacao: " + melhor + " | perplexidade: " + v2_perplexidade(melhor));
        v2_salvar(destino);
        int paciencia = v2_paciencia();
        int sem_melhora = 0;
        int epoca = 0;
        int continuar_treino = 1;
        string metricas = "epoca,ce_treino_online,ce_validacao,perplexidade,acuracia\n";
        while (continuar_treino) {
            double erro_treino = v2_epoca(taxa);
            double erro_validacao = v2_validar();
            double perplexidade = v2_perplexidade(erro_validacao);
            double acerto = v2_acuracia();
            int total_epocas = v2_epocas();
            v2_salvar(destino + ".ultimo");
            if (erro_validacao < melhor) {
                melhor = erro_validacao;
                sem_melhora = 0;
                v2_salvar(destino);
            } else {
                sem_melhora = sem_melhora + 1;
            }
            print("Epoca " + total_epocas + " | CE treino " + erro_treino + " | CE validacao " + erro_validacao + " | PPL " + perplexidade + " | acuracia " + acerto);
            metricas = metricas + format_number(total_epocas, 0) + "," + format_number(erro_treino, 9) + "," + format_number(erro_validacao, 9) + "," + format_number(perplexidade, 9) + "," + format_number(acerto, 9) + "\n";
            writefile(destino + ".metricas.csv", metricas);
            epoca = epoca + 1;
            continuar_treino = epoca < epocas && sem_melhora < paciencia;
        }
        print("Melhor checkpoint: " + destino + " | CE validacao " + melhor);
        print("Retomada completa: " + destino + ".ultimo");
        return;
    }
    int avaliar_solicitado = comando == "avaliar" && args.size() == 3;
    if (avaliar_solicitado) {
        v2_carregar(args[2]);
        double erro = v2_avaliar_arquivo(args[1]);
        print("CE=" + format_number(erro, 9) + " perplexidade=" + v2_perplexidade(erro) + " acuracia=" + v2_acuracia());
        return;
    }
    int gerar_solicitado = comando == "gerar" && args.size() == 4;
    if (gerar_solicitado) {
        int quantidade = v2_inteiro(args[3], 1, 2048);
        v2_carregar(args[1]);
        print(v2_gerar(args[2], quantidade));
        return;
    }
    int chat_solicitado = comando == "chat" && args.size() == 2;
    if (chat_solicitado) {
        v2_carregar(args[1]);
        v2_chat();
        return;
    }
    int info_solicitado = comando == "info" && args.size() == 2;
    if (info_solicitado) {
        v2_carregar(args[1]);
        v2_info();
        return;
    }
    ajuda_v2();
    v2_falhar("Comando invalido.");
}
