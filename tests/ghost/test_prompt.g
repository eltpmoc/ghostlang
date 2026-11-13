llm cerebro = load_gguf("h:\\PROJETOS\\PROJETOS\\ghost-linux\\modelo_ia\\llama-3.1-8b-instruct-q4_k_m.gguf");
string rsp = cerebro.prompt("Diga 'Oi'");
print(rsp);
