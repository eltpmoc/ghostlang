import "v3/src/conhecimento.g"
import "v3/src/dialogo.g"
import "v3/src/autonomia.g"

// Planejamento revisado e identificado como local, separado dos pesos neurais.
function string planejar_trabalho_v5(string pergunta) {
    string texto = intencao_dialogo(pergunta);
    if (regex_search(texto, "\\b(vamos|podemos)\\b") && regex_search(texto, "\\b(construir|contruir|criar|desenvolver|fazer)\\b") && regex_search(texto, "\\b(trabalho|projeto|algo)\\b")) {
        return "[Planejamento local] Podemos começar com um trabalho pequeno e útil. Minha sugestão é escolher um problema concreto e produzir uma primeira versão que possamos testar.\n\nAlgumas opções: um estudo com fontes sobre GhostLang e Números G, um programa em GhostLang ou uma avaliação prática desta IA. Para qualquer opção, podemos definir o objetivo, reunir o material, construir a primeira versão e verificar os resultados.\n\nVocê quer um trabalho de pesquisa, um programa ou tem outro tema em mente?";
    }
    return "";
}

function void chat_v5(string checkpoint) {
    var modelo = transformer_model_v5(1024, 192, 6, 4, 4, 42);
    modelo.load(checkpoint);
    var tempo = temporal_context();
    int modo = 0;
    int internet = 1;
    int progressivo = 1;
    int atraso = text_stream_delay();
    int assunto = -1;
    var turnos = string_split("", 32);
    turnos.clear();
    int limite = modelo.context_size() - 1;
    print("IA V5 Quântica | Transformer com Qubits (GhostNumberG) nativos.");
    print("/assistente | /neural | /fontes | /agora | /tempo | /limpar | /sair");
    print("/internet on|off | /buscar TERMOS | /memoria | /stream [on|off] | /velocidade N");
    print("Assistente consulta temas publicos quando necessario e guarda referencias. /neural usa somente os pesos experimentais. /internet off bloqueia novas consultas.");
    string linha = "";
    while (1) {
        input("Voce> ", linha);
        if (input_closed() || linha == "/sair") { return; }
        if (linha.size() > 0 && linha.size() <= 4000 && regex_search(linha, "^/") == 0) { tempo.observe(); }
        int comando = 0;
        if (linha == "/neural") { modo = 1; turnos.clear(); assunto = -1; comando = 1; print("Modo neural V5: pesos quânticos próprios, sem respostas prontas ou pesquisa automatica."); }
        if (linha == "/assistente") { modo = 0; turnos.clear(); assunto = -1; comando = 1; print("Modo assistente V5: fontes, memoria e geracao quântica experimental."); }
        if (linha == "/fontes") { modo = 2; turnos.clear(); assunto = -1; comando = 1; print("Modo fontes: base revisada e memoria; sem geracao neural."); }
        if (linha == "/limpar") { tempo.clear(); turnos.clear(); assunto = -1; comando = 1; print("Contexto da conversa limpo; medidas temporais reiniciadas; memoria persistente preservada."); }
        if (linha == "/internet off") { internet = 0; comando = 1; print("Internet desativada."); }
        if (linha == "/internet on") { internet = 1; comando = 1; print("Pesquisa automatica ativada no modo assistente; apenas temas publicos sao enviados."); }
        if (linha == "/tempo") { comando = 1; print(tempo.info()); }
        if (linha == "/agora") { comando = 1; print(tempo.clock()); }
        if (linha == "/memoria") { comando = 1; print(knowledge_info()); }
        if (linha == "/stream") { comando = 1; print("Streaming: " + progressivo + ". Use /stream on ou /stream off."); }
        if (linha == "/stream on") { comando = 1; progressivo = 1; print("Streaming ativado."); }
        if (linha == "/stream off") { comando = 1; progressivo = 0; print("Streaming desativado."); }
        if (regex_search(linha, "^/velocidade")) {
            comando = 1;
            if (regex_match(linha, "^/velocidade (100|[0-9]{1,2})$")) {
                var partes = string_split(linha, 32);
                atraso = text_int(partes[1], 0, 100);
                print("Pausa por caractere: " + atraso + " ms.");
            } else { print("Use /velocidade N, de 0 a 100."); }
        }
        if (regex_search(linha, "^/buscar ")) {
            comando = 1;
            if (internet == 0) { print("Internet desativada."); }
            else { text_stream(consultar_memoria(minusculas(linha.substr(8)), internet, 1), 0); }
        }
        if (comando == 0 && regex_search(linha, "^/")) { comando = 1; print("Comando desconhecido. Use /assistente, /neural, /fontes, /agora, /tempo, /limpar, /internet on|off, /buscar TERMOS, /memoria, /stream ou /sair."); }
        if (comando == 0 && linha.size() > 0) {
            if (linha.size() > 4000) { print("Mensagem longa demais."); }
            else {
                string resposta = "";
                int pronta = 0;
                int guardar = 1;
                string temporal = tempo.answer(intencao_dialogo(linha));
                if (temporal.size() > 0) {
                    pronta = 1; guardar = 0; assunto = -1;
                    if (modo == 1) { resposta = "[Ferramenta necessaria] Os pesos do modo /neural nao acessam o contexto temporal. Use /tempo para consultar a sessao, /agora para data e hora, ou /assistente para interpretar referencias temporais automaticamente."; }
                    else { resposta = temporal; }
                }
                if (modo != 1 && pronta == 0) {
                    resposta = planejar_trabalho_v5(linha);
                    if (resposta.size() == 0) { resposta = resposta_social(linha); }
                    string intencao = intencao_dialogo(linha);
                    if (regex_match(intencao, "quem e voce|o que e voce|quem voce e")) { resposta = "[Conversa local] Sou a IA V5 Quântica, coordenada em GhostLang. Meu modelo usa um Transformer Quântico (GhostMatrixG e Qubits CUDA). Acesso memoria local e fontes. O processamento neural usa algebra de Números G."; }
                    if (regex_search(intencao, "habilidade|capacidade|que (voce )?sabe fazer|quais (sao )?suas")) { resposta = "[Conversa local] Posso explicar GhostLang e Números G, mostrar exemplos e pesquisar automaticamente temas públicos quando falta informação e guardar referências na memória local."; }
                    if (regex_search(intencao, "arquitetura") && regex_search(intencao, "v3") == 0) { resposta = "[Consulta documental] A V5 usa um Transformer Quântico em CUDA. Tensores alocam floats r, i, theta (GhostQubits) na VRAM. O processamento forward multiplica estados complexos e propaga incerteza. Treinamento supervisionado com Cross-Entropy (Entropia)."; }
                    if (resposta.size() > 0) { pronta = 1; }
                    int ficha = localizar(linha);
                    if (retomar_dialogo(linha) && assunto >= 0) { ficha = assunto; }
                    if (pronta == 0 && ficha >= 0) { resposta = responder_ficha(ficha, linha); pronta = 1; assunto = ficha; }
                    if (pronta == 0) {
                        assunto = -1;
                        string tema = tema_pesquisa(linha);
                        if (tema.size() > 0) {
                            var memoria = knowledge_consult(tema, 0, 0);
                            if (memoria.found && memoria.stale == 0) { resposta = memoria.text; pronta = 1; }
                            if (pronta == 0 && internet == 1) { resposta = consultar_memoria(tema, internet, 0); pronta = 1; }
                            if (pronta == 0 && memoria.found) { resposta = memoria.text; pronta = 1; }
                            if (pronta == 0 && internet == 0) { resposta = "[Internet] Este tema requer pesquisa externa, mas o acesso à rede está desativado. Use /internet on para permitir a busca, ou pergunte sobre tópicos locais."; pronta = 1; }
                        }
                    }
                    if (pronta == 0 && modo == 2) { resposta = consultar(linha); pronta = 1; }
                }
                if (pronta == 1) {
                    if (progressivo == 1) { text_stream(resposta, atraso); } else { print(resposta); }
                } else {
                    string atual = "Usuário: " + linha + "\nAssistente:";
                    if (atual.size() > limite) {
                        resposta = "[Limite de contexto] A pergunta excede a janela deste checkpoint. Envie uma pergunta menor; nao recortei sua mensagem.";
                        print(resposta); guardar = 0;
                    } else {
                        string prompt = atual;
                        int indice = turnos.size() - 1;
                        int cabe = 1;
                        while (indice >= 0 && cabe == 1) {
                            if (prompt.size() + turnos[indice].size() > limite) { cabe = 0; }
                            else { prompt = turnos[indice] + prompt; indice = indice - 1; }
                        }
                        if (indice >= 0) { print("[Contexto] Turnos antigos omitidos para preservar a pergunta atual completa."); }
                        var geracao = modelo.generate_checked(prompt, 256);
                        if (geracao.accepted) {
                            resposta = geracao.text;
                            if (progressivo == 1) { text_stream("[Geracao neural V5] " + resposta, atraso); }
                            else { print("[Geracao neural V5] " + resposta); }
                        } else {
                            guardar = 0;
                            print("[Geracao neural recusada] " + geracao.reason + ". O modelo ainda nao conseguiu produzir uma resposta utilizavel. Use /assistente para conversar com apoio das ferramentas ou /fontes para consultar o material local.");
                        }
                    }
                }
                if (guardar == 1) {
                    string turno = "Usuário: " + linha + "\nAssistente: " + resposta + "\n";
                    if (turno.size() > limite) { turnos.clear(); }
                    else {
                        turnos.push_back(turno);
                        int tamanho = 0;
                        int indice = turnos.size() - 1;
                        while (indice >= 0) { tamanho = tamanho + turnos[indice].size(); indice = indice - 1; }
                        for (; tamanho > limite;) { tamanho = tamanho - turnos[0].size(); turnos.erase(turnos.begin()); }
                    }
                }
            }
        }
    }
}

function void teste_matriz_quimera() {
    print("=== Teste Nativo de Qubits em GhostLang ===");
    
    // Criando um Qubit
    var qubit1 = number_g(0.5, 0.5, 1.0);
    print("Qubit 1 (GhostNumberG) criado com sucesso.");
    
    var qubit2 = number_g(0.8, -0.2, 0.5);
    print("Qubit 2 (GhostNumberG) criado com sucesso.");

    // Criando Matriz Quântica
    print("Instanciando matriz_g 2x2...");
    var mat = matrix_g(2, 2);
    
    // Inserindo Qubits na Matriz
    mat.set(0, 0, qubit1);
    mat.set(0, 1, qubit2);
    mat.set(1, 0, number_g(-0.5, 0.1, 0.2));
    mat.set(1, 1, number_g(1.0, 0.0, 0.0));
    
    print("Lendo os Qubits inseridos na Matriz:");
    var lido1 = mat.get(0, 0);
    var lido2 = mat.get(0, 1);
    
    print("Lido(0,0):");
    lido1.print();
    print("Lido(0,1):");
    lido2.print();
    
    print("Sucesso total! A engine C++ aceitou as estruturas diretamente pelo script GhostLang.");
}

function void main(string args[]) {
    print(device_info());
    var modelo = transformer_model_v5(1024, 192, 6, 4, 4, 42);
    if (args.size() == 0) { chat_v5("checkpoints/v5/transformer_01.bin"); return; }
    string comando = args[0];
    if (comando == "teste_matriz") { teste_matriz_quimera(); return; }
    if (comando == "autoteste") { modelo.self_test(); return; }
    if (comando == "hardware") { modelo.info(); return; }
    if (comando == "chat") {
        string checkpoint = "checkpoints/v5/transformer_01.bin";
        if (args.size() == 2) { checkpoint = args[1]; }
        chat_v5(checkpoint); return;
    }
    if (comando == "gerar" && args.size() == 4) { modelo.load(args[1]); print(modelo.generate(args[2], text_int(args[3], 1, 2048))); return; }
    if (comando == "info" && args.size() == 2) { modelo.load(args[1]); modelo.info(); return; }
    if (comando == "treinar" && args.size() == 5) {
        string destino = args[4];
        if (file_exists(destino) || file_exists(destino + ".ultimo")) { text_error("Destino ja existe. Use outro nome para preservar o modelo."); }
        int blocos = text_int(args[2], 1, 10000);
        double taxa = text_decimal(args[3], 0.000001, 0.005);
        modelo.prepare_csv(args[1]);
        double melhor = 999.0;
        print("Treinamento Quântico CSV Iniciado...");
        modelo.save(destino);
        int n = 0;
        while (n < blocos) {
            double taxa_atual = taxa;
            if (n == 0) { taxa_atual = taxa * 0.25; }
            if (n > blocos * 0.7) { taxa_atual = taxa * 0.3; }
            double treino = modelo.steps(100, taxa_atual);
            double validacao = modelo.validate(32);
            modelo.save(destino + ".ultimo");
            if (validacao < melhor) { melhor = validacao; modelo.save(destino); }
            print("Passo " + modelo.updates() + " | CE treino " + treino + " | CE validacao " + validacao + " | melhor " + melhor);
            n = n + 1;
        }
        modelo.info(); return;
    }
    if (comando == "continuar" && args.size() == 7) {
        if (file_exists(args[6]) || file_exists(args[6] + ".ultimo")) { text_error("Destino ja existe."); }
        modelo.load(args[5]); modelo.prepare(args[1], args[2]);
        int blocos = text_int(args[3], 1, 1000);
        double taxa = text_decimal(args[3], 0.000001, 0.005);
        double melhor = 999.0;
        modelo.save(args[4]);
        int n = 0;
        while (n < blocos) {
            double treino = modelo.steps(100, taxa);
            double validacao = modelo.validate(32);
            modelo.save(args[4] + ".ultimo");
            if (validacao < melhor) { melhor = validacao; modelo.save(args[4]); }
            print("Passo " + modelo.updates() + " | CE treino " + treino + " | CE validacao " + validacao + " | melhor " + melhor);
            n = n + 1;
        }
        return;
    }
    print("V5 Quântica: hardware | autoteste | teste_matriz | chat [CHECKPOINT] | info CHECKPOINT | gerar CHECKPOINT PROMPT MAX_BYTES");
    print("treinar TREINO_CSV BLOCOS_DE_100 TAXA DESTINO");
}
