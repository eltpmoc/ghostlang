// test_llama.g
// Script GhostLang para testar a integracao nativa com modelos LLAMA GGUF

print("Iniciando GhostLLM...");

// Substitua "modelo.gguf" pelo caminho do seu modelo .gguf
llm cerebro_llama = load_gguf("h:\\PROJETOS\\PROJETOS\\ghost-linux\\modelo_ia\\llama-3.1-8b-instruct-q4_k_m.gguf");

print("Modelo carregado. Enviando prompt...");
string resposta = cerebro_llama.prompt("Olá, IA! O que você sabe sobre a linguagem GhostLang?");

print("Resposta recebida da IA:");
print(resposta);
