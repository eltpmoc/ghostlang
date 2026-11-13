// Controle de intencao e continuidade local. Nao representa raciocinio neural.
function string intencao_dialogo(string texto) {
    texto = regex_replace(minusculas(texto), "[^a-z0-9 ]", " ");
    texto = regex_replace(texto, " +", " ");
    texto = regex_replace(texto, "^ +| +$", "");
    return texto;
}

function string resposta_social(string texto) {
    string simples = intencao_dialogo(texto);
    if (regex_match(simples, "(ola|oi|bom dia|boa tarde|boa noite|ola tudo bem|oi tudo bem|tudo bem)")) {
        return "[Conversa local] Olá! Posso explicar GhostLang e Números G, mostrar exemplos e pesquisar automaticamente temas públicos quando falta informação e guardar referências na memória local. O que você quer explorar?";
    }
    if (regex_match(simples, "(quem e voce|o que e voce|quem voce e|quais sao suas capacidades|o que voce pode fazer)")) {
        return "[Conversa local] Sou a IA própria deste projeto, coordenada em GhostLang. Uso uma rede pequena treinada localmente, CPU/CUDA, uma base revisada de GhostLang e Números G e pesquisas automáticas com referências e uma memória local persistente. Mantenho o último assunto durante a sessão. O modo /assistente prioriza fontes; /neural usa os pesos experimentais e pode produzir respostas erradas. Guardo referências consultadas para reutilizá-las em outras sessões, com fonte e prazo de atualização. Isso amplia minha base consultável; não altera os pesos da rede.";
    }
    if (regex_search(simples, "(voce|como|pode|posso|nao).*(errar|erra|infalivel)")) {
        return "[Conversa local] Posso errar, sim. Se minha resposta anterior deu a entender que sou infalível, ela estava incorreta. Minha rede pequena pode escolher um texto sem relação com sua pergunta. No modo /assistente, priorizo a base revisada; ao pesquisar, mostro a fonte consultada e a data. Qual trecho você quer que eu verifique?";
    }
    if (regex_match(simples, "(obrigado|obrigada|valeu|muito obrigado|muito obrigada)")) {
        return "[Conversa local] De nada! Podemos continuar no mesmo assunto ou explorar outro tema.";
    }
    return "";
}

function int retomar_dialogo(string texto) {
    string simples = intencao_dialogo(texto);
    return regex_match(simples, "(nao entendi|nao compreendi|explique melhor|pode explicar melhor|explique mais|continue|e um exemplo|de um exemplo|mais detalhes|por que|porque|resuma|qual e a fonte|de onde veio essa informacao)") || pedido_continuacao(texto);
}
