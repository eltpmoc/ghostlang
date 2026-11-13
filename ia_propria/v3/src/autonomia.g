// Selecao conservadora de um tema publico; nao envia historico ou arquivos.
function string tema_pesquisa(string pergunta) {
    if (pergunta.size() > 240) { return ""; }
    string texto = minusculas(pergunta);
    if (regex_match(texto, "[a-z ?!.-]+") == 0) { return ""; }
    texto = regex_replace(texto, " +", " ");
    texto = regex_replace(texto, "^ +|[ ?!.]+$", "");
    if (regex_search(texto, "\\b(eu|meu|minha|meus|minhas|nosso|nossa|senha|segredo|token|chave|cpf|endereco|telefone|email|paciente|cliente|confidencial|interno|privado|diagnostico|tenho|sinto|estou)\\b")) { return ""; }
    // Material especifico deste repositorio e pedidos de execucao continuam locais.
    if (regex_search(texto, "\\b(ghostlang|fantasmas|ghost|crie|execute|rode|apague|instale|envie|salve)\\b") || regex_search(texto, "(teoria|numeros) (dos )?g\\b")) { return ""; }
    texto = regex_replace(texto, "^(por favor )?(o que e|o que sao|quem e|quem foi|qual e|quais sao|como funciona|como funcionam|explique|descreva|fale sobre|pesquise sobre|pesquise|procure sobre|procure|busque sobre|busque) +", "");
    texto = regex_replace(texto, "^(o|a|os|as|um|uma) +", "");
    if (regex_search(texto, "\\b(voce|isso|aquilo|ele|ela|entendi|obrigado|ajude|pode)\\b")) { return ""; }
    if (texto.size() < 3 || texto.size() > 160) { return ""; }
    return texto;
}

function string consultar_memoria(string tema, int internet, int atualizar) {
    // Primeiro tenta o conhecimento armazenado, inclusive com a rede desligada.
    var local = knowledge_consult(tema, 0, 0);
    if (local.allowed == 0) { return local.text; }
    if (regex_search(tema, "frnaca")) { print("[Normalizacao] Interpretando Frnaça como França."); }
    if (local.found && local.stale == 0 && atualizar == 0) { return local.text; }
    if (internet == 0) { return local.text; }
    print("[Pesquisa automatica] Preciso consultar ou atualizar o tema publico: " + tema + ". Aguarde...");
    var resultado = knowledge_consult(tema, 1, atualizar);
    return resultado.text;
}
