import "v3/src/conhecimento.g"
import "v3/src/dialogo.g"
import "v3/src/autonomia.g"

// Texto documental e avisos ja completos recebem exibicao progressiva.
function void exibir_resposta(string resposta, int progressivo, int atraso) {
    if (progressivo == 1) { text_stream(resposta, atraso); }
    else { print(resposta); }
}

// Intencoes, contexto, pesquisa automatica e memoria coordenados em GhostLang.
function void conversa(string checkpoint) {
    textmodel modelo = text_model(64, 32, 192, 32, 4096, 42);
    int carregado = 0;
    int neural = 0;
    int assunto = -1;
    int progressivo = 1;
    int atraso = text_stream_delay();
    int internet = 1;
    string ultima_pergunta = "";
    string ultima_resposta = "";
    print("IA V3.4 | pesquisa automatica e memoria local com fontes.");
    print("/assistente | /neural | /fontes | /limpar | /sair");
    print("/stream [on|off] | /velocidade 0..100 (ms por caractere)");
    print("/buscar PALAVRAS-CHAVE | /internet [on|off] | /memoria");
    print("Pesquisa automatica ativada: temas publicos ausentes ou vencidos sao consultados e guardados com fonte e data. Historico e arquivos nao sao enviados. /internet off mantem tudo local; /memoria lista o conhecimento adquirido.");
    string linha = "";
    while (1) {
        input("Voce> ", linha);
        if (input_closed()) { return; }
        if (linha == "/sair") { return; }
        int comando = 0;
        if (linha == "/neural") {
            comando = 1;
            if (file_exists(checkpoint)) {
                if (carregado == 0) { modelo.load(checkpoint); carregado = 1; }
                neural = 1;
                print("Modo neural: pesos experimentais; podem responder fora do assunto. Saudacoes e esclarecimentos usam o controle local. /assistente prioriza respostas com fontes.");
            } else { print("Checkpoint ausente. Execute treinar ou indique um checkpoint no comando chat."); }
        }
        if (linha == "/fontes" || linha == "/assistente") { comando = 1; neural = 0; print("Modo assistente: base revisada, memoria adquirida e pesquisa automatica quando falta conhecimento."); }
        if (linha == "/limpar") { comando = 1; assunto = -1; ultima_pergunta = ""; ultima_resposta = ""; print("Contexto da conversa limpo."); }
        if (linha == "/stream") {
            comando = 1;
            if (progressivo == 1) { print("Streaming ativado. Use /stream off para desativar."); }
            else { print("Streaming desativado. Use /stream on para ativar."); }
        }
        if (linha == "/stream on") { comando = 1; progressivo = 1; print("Streaming ativado."); }
        if (linha == "/stream off") { comando = 1; progressivo = 0; print("Streaming desativado: resposta completa de uma vez."); }
        if (linha == "/internet") {
            comando = 1;
            if (internet == 1) { print("Pesquisa automatica ativada para temas publicos sem resposta local atualizada. /internet off bloqueia a rede e preserva a memoria."); }
            else { print("Internet desativada. A base revisada e a memoria adquirida continuam disponiveis. /internet on reativa a pesquisa automatica."); }
        }
        if (linha == "/internet on") { comando = 1; internet = 1; print("Pesquisa automatica ativada. Somente o tema publico selecionado e consultado; referencias encontradas sao guardadas na memoria."); }
        if (linha == "/internet off") { comando = 1; internet = 0; print("Internet desativada. Consultas permanecem locais."); }
        if (linha == "/memoria") { comando = 1; print(knowledge_info()); }
        if (linha == "/buscar" || regex_search(linha, "^/buscar ")) {
            comando = 1;
            if (internet == 0) { print("Internet desativada. Use /internet on para habilitar /buscar."); }
            else {
                if (linha == "/buscar") { print("Use /buscar PALAVRAS-CHAVE publicas: ate 16 palavras e 160 bytes, sem numeros, URLs, e-mails ou codigo. Os termos sao enviados a Wikipedia; presidente da Franca consulta a fonte oficial. Nao inclua dados pessoais ou segredos."); }
                else {
                    string termos = linha.substr(8);
                    string resposta_web = consultar_memoria(minusculas(termos), internet, 1);
                    exibir_resposta(resposta_web, progressivo, atraso);
                    assunto = -1;
                    ultima_pergunta = termos;
                    ultima_resposta = resposta_web;
                }
            }
        }
        if (regex_search(linha, "^/velocidade")) {
            comando = 1;
            if (regex_match(linha, "^/velocidade (100|[0-9]{1,2})$")) {
                var partes = string_split(linha, 32);
                atraso = text_int(partes[1], 0, 100);
                print("Pausa por caractere: " + atraso + " ms. Use /stream on para ativar.");
            } else { print("Use /velocidade N, com N de 0 a 100 ms. Maior valor significa exibicao mais lenta."); }
        }
        if (comando == 0 && regex_search(linha, "^/")) {
            comando = 1;
            print("Comando desconhecido. Use /assistente, /neural, /fontes, /buscar TERMOS, /internet, /memoria, /limpar, /stream, /velocidade N ou /sair.");
        }
        if (comando == 0 && linha.size() > 0) {
            if (linha.size() > 4000) { print("Mensagem longa demais (4000 bytes)."); }
            else {
                string resposta = resposta_social(linha);
                int tratada = 0;
                if (resposta.size() > 0) {
                    exibir_resposta(resposta, progressivo, atraso);
                    // Saudar/agradecer nao apaga o assunto documental em andamento.
                    tratada = 1;
                }
                if (tratada == 0 && retomar_dialogo(linha)) {
                    tratada = 1;
                    if (assunto >= 0) {
                        resposta = responder_ficha(assunto, linha + " detalhes exemplo");
                    } else {
                        if (ultima_resposta.size() > 0) {
                            resposta = "[Contexto local] O assunto anterior foi: " + ultima_pergunta + ".\nPosso recuperar a resposta abaixo, mas não tenho uma explicação adicional validada. Indique o trecho específico que deseja esclarecer.\n\n" + ultima_resposta;
                        } else { resposta = "[Contexto local] Ainda não há um assunto anterior. Diga o tema ou cole o trecho que deseja entender."; }
                    }
                    exibir_resposta(resposta, progressivo, atraso);
                }
                if (tratada == 0) {
                    int ficha = localizar(linha);
                    if (ficha >= 0) {
                        assunto = ficha;
                        if (neural == 1) {
                            string prompt = "Usuário: " + linha + "\nAssistente:";
                            if (progressivo == 1) { resposta = modelo.stream(prompt, 320, atraso, "[Geracao neural] "); }
                            else { resposta = modelo.generate(prompt, 320); print("[Geracao neural] " + resposta); }
                        } else { resposta = responder_ficha(ficha, linha); exibir_resposta(resposta, progressivo, atraso); }
                    } else {
                        assunto = -1;
                        resposta = consultar(linha);
                        if (regex_search(resposta, "^\\[Sem fonte suficiente\\]")) {
                            string tema = tema_pesquisa(linha);
                            if (tema.size() > 0) { resposta = consultar_memoria(tema, internet, 0); }
                            else { resposta = "[Sem fonte suficiente] Não tenho uma resposta local e não identifiquei um tema público adequado para pesquisar automaticamente. Reformule usando apenas o assunto público, sem dados pessoais. Não envio arquivos nem o histórico da conversa."; }
                        }
                        exibir_resposta(resposta, progressivo, atraso);
                    }
                    ultima_pergunta = linha;
                    ultima_resposta = resposta;
                }
            }
        }
    }
}

function void ajuda_v3() {
    print("Ghost V3 - textmodel nativo, GhostLang e Numeros G");
    print("treinar TREINO VALIDACAO EPOCAS TAXA DESTINO");
    print("continuar TREINO VALIDACAO EPOCAS TAXA ORIGEM DESTINO");
    print("avaliar CORPUS CHECKPOINT");
    print("gerar CHECKPOINT PROMPT MAX_TOKENS");
    print("chat [CHECKPOINT] | consultar PERGUNTA | info CHECKPOINT | hardware | autoteste");
}

function void main(string args[]) {
    print(device_info());
    textmodel modelo = text_model(64, 32, 192, 32, 4096, 42);
    if (args.size() == 0) {
        conversa("checkpoints/v3/ghost_g_v31.bin");
        return;
    }
    string comando = args[0];
    if (comando == "consultar" && args.size() == 2) {
        print(consultar(args[1]));
        return;
    }
    if (comando == "hardware") {
        modelo.info();
        return;
    }
    if (comando == "autoteste") {
        modelo.self_test();
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
            text_error("Argumentos invalidos para treinamento.");
        }
        int epocas = text_int(args[3], 1, 200);
        double taxa = text_decimal(args[4], 0.000001, 0.02);
        string destino = args[tamanho - 1];
        int existe = file_exists(destino) || file_exists(destino + ".ultimo") || file_exists(destino + ".metricas.csv") || file_exists(destino + ".treino.txt");
        if (existe) {
            text_error("Destino ja existe. Use um novo nome para preservar o modelo.");
        }
        if (comando == "continuar") {
            origem = args[5];
        }
        modelo.prepare(args[1], args[2], origem);
        writefile(destino + ".treino.txt", "Treinamento local V3\nData: " + format_date(now(), "%Y-%m-%d %H:%M:%S %z") + "\nDispositivo: " + device_info() + "\nTreino: " + args[1] + "\nValidacao: " + args[2] + "\n");
        double melhor = modelo.validate();
        print("CE inicial validacao: " + melhor + " | perplexidade: " + text_perplexity(melhor));
        modelo.save(destino);
        int paciencia = 8;
        int sem_melhora = 0;
        int epoca = 0;
        int continuar_treino = 1;
        string metricas = "epoca,ce_treino_online,ce_validacao,perplexidade,acuracia\n";
        while (continuar_treino) {
            double erro_treino = modelo.epoch(taxa);
            double erro_validacao = modelo.validate();
            double perplexidade = text_perplexity(erro_validacao);
            double acerto = modelo.accuracy();
            int total_epocas = modelo.epochs();
            modelo.save(destino + ".ultimo");
            if (erro_validacao < melhor) {
                melhor = erro_validacao;
                sem_melhora = 0;
                modelo.save(destino);
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
        modelo.load(args[2]);
        double erro = modelo.evaluate(args[1]);
        print("CE=" + format_number(erro, 9) + " perplexidade=" + text_perplexity(erro) + " acuracia=" + modelo.accuracy());
        return;
    }
    int gerar_solicitado = comando == "gerar" && args.size() == 4;
    if (gerar_solicitado) {
        int quantidade = text_int(args[3], 1, 2048);
        modelo.load(args[1]);
        print(modelo.generate(args[2], quantidade));
        return;
    }
    int chat_solicitado = comando == "chat" && args.size() <= 2;
    if (chat_solicitado) {
        string arquivo = "checkpoints/v3/ghost_g_v31.bin";
        if (args.size() == 2) { arquivo = args[1]; }
        conversa(arquivo);
        return;
    }
    int info_solicitado = comando == "info" && args.size() == 2;
    if (info_solicitado) {
        modelo.load(args[1]);
        modelo.info();
        return;
    }
    ajuda_v3();
    text_error("Comando invalido.");
}
