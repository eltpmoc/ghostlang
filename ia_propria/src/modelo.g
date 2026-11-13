// Modelo autoregressivo pequeno: 4 caracteres -> 168 entradas -> 32 ReLU -> 42 saidas.
// CPU: network/optimize nativos. GPU: backend CUDA isolado desta IA.
string alfabeto = " abcdefghijklmnopqrstuvwxyz.,!?0123456789\n";
srand(42);
gate oculta(32, 168);
gate saida(42, 32);
network modelo = oculta >> saida;
int usar_gpu = ia_iniciar();
state entrada = encode("", 168);
state alvo = encode("", 42);
double ultima_acuracia = 0.0;

function void falhar(string mensagem) {
    print("Erro: " + mensagem);
    exit(1);
}

function string normalizar(string texto) {
    // UTF-8: substituicoes explicitas antes de percorrer os bytes.
    texto = regex_replace(texto, "á|à|â|ã|ä|Á|À|Â|Ã|Ä", "a");
    texto = regex_replace(texto, "é|è|ê|ë|É|È|Ê|Ë", "e");
    texto = regex_replace(texto, "í|ì|î|ï|Í|Ì|Î|Ï", "i");
    texto = regex_replace(texto, "ó|ò|ô|õ|ö|Ó|Ò|Ô|Õ|Ö", "o");
    texto = regex_replace(texto, "ú|ù|û|ü|Ú|Ù|Û|Ü", "u");
    texto = regex_replace(texto, "ç|Ç", "c");
    texto = regex_replace(texto, "\r\n", "\n");
    string maiusculas = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    string minusculas = "abcdefghijklmnopqrstuvwxyz";
    string resultado = "";
    for (int i = 0; i < texto.size(); i = i + 1) {
        string letra = texto.substr(i, 1);
        int indice = maiusculas.find(letra);
        int condicao_1 = indice >= 0 && indice < 26;
        if (condicao_1) {
            letra = minusculas.substr(indice, 1);
        }
        indice = alfabeto.find(letra);
        int condicao_2 = indice < 0 || indice >= 42;
        if (condicao_2) {
            letra = " ";
        }
        resultado = resultado + letra;
    }
    return resultado;
}

function string ler_corpus(string caminho) {
    if (!file_exists(caminho)) {
        falhar("Corpus nao encontrado: " + caminho);
    }
    string texto = readfile(caminho);
    if (texto.size() > 1000000) {
        falhar("Use um arquivo de ate 1 MB nesta primeira versao.");
    }
    texto = normalizar(texto);
    int condicao_3 = texto.size() < 5 || texto.size() > 20000;
    if (condicao_3) {
        falhar("O corpus normalizado precisa ter entre 5 e 20000 caracteres.");
    }
    if (!regex_search(texto, "[a-z0-9]")) {
        falhar("O corpus precisa conter letras ou numeros suportados.");
    }
    return texto;
}

function void preparar(string contexto) {
    for (int i = 0; i < 168; i = i + 1) {
        entrada(i, 0).x = 0.0;
    }
    for (int i = 0; i < 4; i = i + 1) {
        int token = alfabeto.find(contexto.substr(i, 1));
        entrada(i * 42 + token, 0).x = 1.0;
    }
}

function double avaliar(string texto) {
    double erro_total = 0.0;
    int acertos = 0;
    int amostras = texto.size() - 4;
    for (int pos = 4; pos < texto.size(); pos = pos + 1) {
        preparar(texto.substr(pos - 4, 4));
        state predicao = ia_prever(modelo, entrada);
        int esperado = alfabeto.find(texto.substr(pos, 1));
        int melhor = 0;
        for (int j = 0; j < 42; j = j + 1) {
            double desejado = 0.0;
            if (j == esperado) {
                desejado = 1.0;
            }
            double diferenca = predicao(j, 0).x - desejado;
            erro_total = erro_total + diferenca * diferenca;
            if (predicao(j, 0).x > predicao(melhor, 0).x) {
                melhor = j;
            }
        }
        if (melhor == esperado) {
            acertos = acertos + 1;
        }
    }
    ultima_acuracia = 1.0 * acertos / amostras;
    return erro_total / (amostras * 42.0);
}

function void salvar_pesos(string caminho) {
    ia_sincronizar(modelo);
    // Texto com pesos reais e biases. Nao serializa objetos C++ em memoria.
    string dados = "GHOST_TEXT_V1\n168\n32\n42\n";
    for (int r = 0; r < 32; r = r + 1) {
        for (int c = 0; c < 168; c = c + 1) {
            dados = dados + format_number(oculta(r, c).x, 17) + "\n";
        }
        dados = dados + format_number(oculta.bias[r].x, 17) + "\n";
    }
    for (int r = 0; r < 42; r = r + 1) {
        for (int c = 0; c < 32; c = c + 1) {
            dados = dados + format_number(saida(r, c).x, 17) + "\n";
        }
        dados = dados + format_number(saida.bias[r].x, 17) + "\n";
    }
    writefile(caminho + ".tmp", dados);
    if (readfile(caminho + ".tmp") != dados) {
        falhar("Falha ao gravar checkpoint temporario: " + caminho);
    }
    if (rename((caminho + ".tmp").c_str(), caminho.c_str()) != 0) {
        falhar("Falha ao concluir checkpoint: " + caminho);
    }
}

function void carregar_pesos(string caminho) {
    ia_invalidar();
    if (!file_exists(caminho)) {
        falhar("Checkpoint nao encontrado: " + caminho);
    }
    string dados = readfile(caminho);
    if (dados.size() > 1000000) {
        falhar("Checkpoint excede o tamanho permitido.");
    }
    var linhas = string_split(dados, 10);
    if (linhas.size() != 6798) {
        falhar("Checkpoint incompleto ou com arquitetura incompativel.");
    }
    int condicao_4 = linhas[0] != "GHOST_TEXT_V1" || linhas[1] != "168" || linhas[2] != "32" || linhas[3] != "42";
    if (condicao_4) {
        falhar("Formato de checkpoint incompativel.");
    }
    for (int i = 4; i < linhas.size(); i = i + 1) {
        if (!regex_match(linhas[i], "-?[0-9]+[.][0-9]+")) {
            falhar("Checkpoint contem um valor numerico invalido.");
        }
        double valor = parse_number(linhas[i]);
        int condicao_5 = linhas[i].size() > 30 || valor < -1000.0 || valor > 1000.0;
        if (condicao_5) {
            falhar("Checkpoint contem um peso fora dos limites.");
        }
    }
    int indice = 4;
    for (int r = 0; r < 32; r = r + 1) {
        for (int c = 0; c < 168; c = c + 1) {
            oculta(r, c).x = parse_number(linhas[indice]);
            indice = indice + 1;
        }
        oculta.bias[r].x = parse_number(linhas[indice]);
        indice = indice + 1;
    }
    for (int r = 0; r < 42; r = r + 1) {
        for (int c = 0; c < 32; c = c + 1) {
            saida(r, c).x = parse_number(linhas[indice]);
            indice = indice + 1;
        }
        saida.bias[r].x = parse_number(linhas[indice]);
        indice = indice + 1;
    }
}

function void treinar(string treino, string validacao, int epocas, double taxa, string destino) {
    double inicial = avaliar(treino);
    print("MSE inicial treino: " + inicial);
    string historico = "epoca,mse_treino,mse_validacao,acuracia_validacao\n";
    int amostras = treino.size() - 4;
    state ordem = encode("", amostras);
    for (int i = 0; i < amostras; i = i + 1) {
        ordem(i, 0).x = i;
    }
    for (int epoca = 1; epoca <= epocas; epoca = epoca + 1) {
        // Fisher-Yates: visita cada janela uma vez por epoca.
        for (int i = amostras - 1; i > 0; i = i - 1) {
            int j = rand() % (i + 1);
            double temporario = ordem(i, 0).x;
            ordem(i, 0).x = ordem(j, 0).x;
            ordem(j, 0).x = temporario;
        }
        for (int i = 0; i < amostras; i = i + 1) {
            int pos = ordem(i, 0).x + 4;
            preparar(treino.substr(pos - 4, 4));
            for (int j = 0; j < 42; j = j + 1) {
                alvo(j, 0).x = 0.0;
            }
            int esperado = alfabeto.find(treino.substr(pos, 1));
            alvo(esperado, 0).x = 1.0;
            if (usar_gpu) {
                ia_treinar_gpu(modelo, entrada, alvo, taxa);
            } else {
                modelo => optimize(entrada, alvo, epochs=1, lr=taxa, opt="adam");
            }
        }
        double erro_treino = avaliar(treino);
        double erro_validacao = avaliar(validacao);
        int condicao_6 = !(erro_treino >= 0.0 && erro_treino < 1000.0 && erro_validacao >= 0.0 && erro_validacao < 1000.0);
        if (condicao_6) {
            falhar("Treino divergiu. Reduza a taxa de aprendizado.");
        }
        print("Epoca " + epoca + " | MSE treino " + erro_treino + " | MSE validacao " + erro_validacao + " | acuracia " + ultima_acuracia);
        historico = historico + format_number(epoca, 0) + "," + format_number(erro_treino, 12) + "," + format_number(erro_validacao, 12) + "," + format_number(ultima_acuracia, 12) + "\n";
    }
    salvar_pesos(destino);
    writefile(destino + ".metricas.csv", historico);
    print("Checkpoint salvo: " + destino);
}

function string gerar(string inicio, int quantidade) {
    string resultado = normalizar(inicio);
    string contexto = "    " + resultado;
    for (int i = 0; i < quantidade; i = i + 1) {
        preparar(contexto.substr(contexto.size() - 4, 4));
        state predicao = ia_prever(modelo, entrada);
        int melhor = 0;
        for (int j = 1; j < 42; j = j + 1) {
            if (predicao(j, 0).x > predicao(melhor, 0).x) {
                melhor = j;
            }
        }
        string letra = alfabeto.substr(melhor, 1);
        resultado = resultado + letra;
        contexto = contexto + letra;
    }
    return resultado;
}
