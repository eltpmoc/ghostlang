print("Iniciando GhostLLM no REPL...");
llm cerebro = load_gguf("dummy.gguf");
print("Instancia llm criada. Enviando prompt...");
string resposta = cerebro.prompt("Qualquer coisa");
print(resposta);
