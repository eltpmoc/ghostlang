// chat_llama.g
// Script GhostLang para testar a IA de forma iterativa com Memoria (Chat)

function void main(string args[]) {
    print("Iniciando Ghost Chat...");
    
    string modelo = "";
    string memoria = "[\n";
    
    if (args.size() > 0) {
        modelo = args[0];
    }
    
    // Verifica se foi passado -load memoria.json
    if (args.size() > 2) {
        if (args[1] == "-load") {
            if (file_exists(args[2])) {
                memoria = openfile(args[2]);
                // Remover o fechamento ] do JSON se existir, para continuarmos adicionando
                if (memoria.length() > 2) {
                    memoria = memoria.substr(0, memoria.length() - 2); 
                    memoria = memoria + ",\n";
                }
                print("[Ghost OS] Memoria carregada do arquivo: " + args[2]);
            } else {
                print("[Ghost OS] Arquivo de memoria nao encontrado.");
            }
        }
    }

    llm cerebro_llama = load_gguf(modelo);

    string user_input = "";

    print("=====================================================================");
    print("GhostLang AI Chat Iniciado!");
    print("Digite '/bye' a qualquer momento para sair.");
    print("Digite '/salvar <arquivo.json>' para salvar a memoria.");
    print("Digite '/load <arquivo.json>' para carregar a memoria.");
    print("Digite '/limpar' para limpar a memoria.");
    print("Digite '/reset' para resetar o chat.");
    print("Digite '/exibir' para exibir a memoria.");
    print("Digite '/modelo <arquivo.gguf>' para carregar um novo modelo de IA.");
    print("Digite '/prompt <prompt>' para fazer uma pergunta.");
    print("Digite '/help' para exibir a ajuda.");
    print("Digite '/clean' para limpar a tela.");
    print("=====================================================================");


    print("\n[Ghost OS] Chat iniciado! Digite '/help' para ver os comandos.");

    while (user_input != "/bye") {
        input("\nVoce: ", user_input);
        
        if (user_input == "/bye") {
            print("Saindo do chat... Ate mais!");
        } else {
            if (user_input.find("/salvar ") == 0) {
                string filename = user_input.substr(8);
                writefile(filename, memoria + "\n]");
                print("[Ghost OS] Memoria salva com sucesso em: " + filename);
            } else {
                // Limpar a tela do chat
                if (user_input.find("/clean") == 0) {
                    print("\033[2J\033[H");
                } else {
                    if (user_input.find("/load ") == 0) {
                        string filename = user_input.substr(6);
                    if (file_exists(filename)) {
                        memoria = openfile(filename);
                        // Remover o fechamento ] do JSON se existir, para continuarmos adicionando
                        if (memoria.length() > 2) {
                            memoria = memoria.substr(0, memoria.length() - 2); 
                            memoria = memoria + ",\n";
                        }
                        print("[Ghost OS] Memoria carregada do arquivo: " + filename);
                    } else {
                        print("[Ghost OS] Arquivo de memoria nao encontrado.");
                    }
                } else {
                    if (user_input == "/limpar") {
                        memoria = "[\n";
                        print("[Ghost OS] Memoria limpa.");
                    } else {
                        if (user_input == "/reset") {
                            memoria = "[\n";
                            print("[Ghost OS] Chat resetado.");
                        } else {
                            if (user_input == "/exibir") {
                                print("[Ghost OS] Memoria atual:\n" + memoria + "]");
                            } else {
                                if (user_input == "/help") {
                                    print("========================================");
                                    print("GhostLang AI Chat Iniciado!");
                                    print("Digite '/bye' a qualquer momento para sair.");
                                    print("Digite '/salvar <arquivo.json>' para salvar a memoria.");
                                    print("Digite '/load <arquivo.json>' para carregar a memoria.");
                                    print("Digite '/limpar' para limpar a memoria.");
                                    print("Digite '/reset' para resetar o chat.");
                                    print("Digite '/exibir' para exibir a memoria.");
                                    print("Digite '/modelo <arquivo.gguf>' para carregar uma nova memoria.");
                                    print("Digite '/prompt <prompt>' para fazer uma pergunta.");
                                    print("Digite '/help' para exibir a ajuda.");
                                    print("Digite '/clean' para limpar a tela.");
                                    print("========================================");
                                } else {
                                    if (user_input.find("/modelo ") == 0) {
                                        string filename = user_input.substr(8);
                                        cerebro_llama = load_gguf(filename);
                                        print("[Ghost OS] Modelo carregado com sucesso: " + filename);
                                    } else {
                                        if (user_input.find("/prompt ") == 0) {
                                            string prompt = user_input.substr(8);
                                            string resposta = cerebro_llama.prompt(prompt);
                                            print(resposta);
                                        } else {
                                            // Constroi o prompt final passando todo o historico JSON + a nova pergunta
                                            string context_block = "";
                                            if (memoria.length() > 5) {
                                                context_block = "Abaixo esta o historico de nossa conversa em JSON. Use-o como memoria de curto prazo, mas responda SEMPRE em texto natural, NAO responda em JSON.\n" + memoria + "]\n\n";
                                            } else {
                                                context_block = "Voce eh uma IA inteligente. Responda ao usuario em texto natural de forma amigavel e conversacional.\n\n";
                                            }
                                            
                                            string full_prompt = context_block + "Responda agora ao usuario: " + user_input + "\n### Resposta da IA:\n";
                                            
                                            print("\nGhost IA (pensando...):");
                                            string resposta = cerebro_llama.prompt(full_prompt);
                                            
                                            // Tratamento basico para remover aspas e quebras da resposta para nao quebrar o JSON
                                            if (memoria.length() > 2) {
                                                memoria = memoria + ",\n";
                                            }
                                            
                                            // Remove quebras de linha que possam quebrar o JSON string
                                            memoria = memoria + "  {\"user\": \"" + user_input + "\", \"ai\": \"" + resposta + "\"}";
                                        }
                                    }
                                }
                            }
                        }
                    } 
                }
            }
        }
    }
}
}
