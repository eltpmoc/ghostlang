"""Emite consulta, formatação e continuidade de assunto em GhostLang."""
import json
from pathlib import Path

def lit(text): return json.dumps(text, ensure_ascii=False)

def generate(cards, root, stop, normalize):
    code=['// Gerado de fichas revisadas; a consulta nao usa pesos neurais.',
          'function string minusculas(string texto) {']
    for upper,lower in zip('ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz'):
        code.append(f'    texto = regex_replace(texto, "{upper}", "{lower}");')
    for chars,lower in [('ÁÀÂÃÄáàâãä','a'),('ÉÈÊËẼéèêëẽ','e'),('ÍÌÎÏíìîï','i'),('ÓÒÔÕÖóòôõö','o'),('ÚÙÛÜúùûü','u'),('Çç','c')]:
        code.append(f'    texto = regex_replace(texto, {lit("("+"|".join(chars)+")")}, "{lower}");')
    code += ['    return texto;', '}',
             'function string normalizar(string texto) {',
             '    texto = regex_replace(minusculas(texto), "[^a-z0-9]+", " ");',
             '    var palavras = string_split(texto, 32);', '    string saida = " ";',
             '    int n = 0;', '    while (n < palavras.size()) {',
             '        string palavra = palavras[n];', '        string busca = " " + palavra + " ";',
             f'        string ignorar = {lit(" "+" ".join(sorted(stop))+" ")};',
             '        if (palavra.size() > 0 && string_contains(ignorar, busca) == "false" && string_contains(saida, busca) == "false") { saida = saida + palavra + " "; }',
             '        n = n + 1;', '    }', '    return saida;', '}',
             'function double similaridade(string pergunta, string exemplo) {',
             '    var palavras = string_split(pergunta, 32);', '    var termos = string_split(exemplo, 32);',
             '    double comuns = 0.0;', '    double total = 0.0;', '    double tamanho = 0.0;',
             '    int i = 0;', '    while (i < palavras.size()) {',
             '        if (palavras[i].size() > 0) {', '            total = total + 1.0;',
             '            if (string_contains(exemplo, " " + palavras[i] + " ") == "true") { comuns = comuns + 1.0; }',
             '        }', '        i = i + 1;', '    }', '    i = 0;',
             '    while (i < termos.size()) { if (termos[i].size() > 0) { tamanho = tamanho + 1.0; } i = i + 1; }',
             '    if (total < 1.0 || tamanho < 1.0) { return 0.0; }',
             '    if (tamanho > total) { total = tamanho; }', '    return comuns / total;', '}',
             'function int localizar(string texto) {',
             '    if (texto.size() > 4000) { return -1; }',
             '    string pergunta = normalizar(texto);',
             '    double melhor = 0.0;', '    double segundo = 0.0;',
             '    double pontuacao = 0.0;', '    double atual = 0.0;', '    int ficha = -1;']
    for index,card in enumerate(cards):
        code.append('    pontuacao = 0.0;')
        for alias in dict.fromkeys(normalize(q) for q in card['perguntas']+card.get('consultas',[])):
            code += [f'    atual = similaridade(pergunta, {lit(" "+alias+" ")});',
                     '    if (atual > pontuacao) { pontuacao = atual; }']
        code += ['    if (pontuacao > melhor) {', '        segundo = melhor;',
                 f'        melhor = pontuacao; ficha = {index};',
                 '    } else { if (pontuacao > segundo) { segundo = pontuacao; } }']
    code += ['    if (melhor < 0.68 || melhor - segundo < 0.10) { return -1; }', '    return ficha;', '}',
             'function int pedido_exemplo(string texto) {',
             '    return regex_search(minusculas(texto), "\\b(exemplo|exemplos|codigo|demonstre)\\b");', '}',
             'function int pedido_continuacao(string texto) {',
             '    return normalizar(texto) == " " && regex_search(minusculas(texto), "\\b(exemplo|exemplos|codigo|detalhe|detalhes|explique|resuma|resumo)\\b");', '}',
             'function string responder_ficha(int ficha, string pedido) {',
             '    int exemplo = pedido_exemplo(pedido);',
             '    int breve = regex_search(minusculas(pedido), "\\b(resuma|resumo|breve|curto)\\b");',
             '    string resposta = "";']
    # Double-escape regex word-boundaries in the emitted .g string literals.
    code=[line.replace('"\\b','"\\\\b').replace(')\\b"',')\\\\b"') for line in code]
    for index,card in enumerate(cards):
        code += [f'    if (ficha == {index}) {{',
                 f'        resposta = {lit("[Consulta documental] "+card["resposta"])};']
        if card.get('detalhes'):
            code.append(f'        if (breve == 0) {{ resposta = resposta + {lit(chr(10)*2+card["detalhes"])}; }}')
        if card.get('exemplo'):
            source=root/card['exemplo']
            codeblock=source.read_text().strip()
            answer='\n\nExemplo completo em GhostLang:\n```ghost\n'+codeblock+'\n```'
            answer+='\n\n'+card['explicacao_exemplo']
            answer+='\n\nResultado esperado:\n'+card['saida_exemplo']
            answer+='\n\nPara executar, na raiz do projeto:\n'
            answer+='build_native/ghost '+card['exemplo']+' -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo'
            code.append(f'        if (exemplo == 1) {{ resposta = resposta + {lit(answer)}; }}')
        else:
            code.append('        if (exemplo == 1) { resposta = resposta + "\\n\\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }')
        sources=[card['fonte']]+card.get('fontes_adicionais',[])
        if card.get('exemplo'): sources.append(card['exemplo'])
        code += [f'        return resposta + {lit(chr(10)+"Fonte: "+"; ".join(sources))};','    }']
    code += ['    return "[Sem fonte suficiente] Não encontrei uma resposta inequívoca para esse assunto. Posso explicar GhostLang, matrizes, textmodel, CUDA e Números G com exemplos revisados. Não consigo criar programas arbitrários a partir desta base.";', '}',
             'function string consultar(string texto) {',
             '    if (texto.size() > 4000) { return "Mensagem longa demais (4000 bytes)."; }',
             '    int ficha = localizar(texto);',
             '    if (ficha >= 0) { return responder_ficha(ficha, texto); }',
             '    string separado = regex_replace(minusculas(texto), " e ", "\\n");',
             '    var partes = string_split(separado, 10);',
             '    if (partes.size() == 2) {',
             '        int primeira = localizar(partes[0]); int segunda = localizar(partes[1]);',
             '        if (primeira >= 0 && segunda >= 0 && primeira != segunda) {',
             '            return responder_ficha(primeira, partes[0]) + "\\n\\n" + responder_ficha(segunda, partes[1]);',
             '        }', '    }', '    return responder_ficha(-1, texto);', '}']
    return '\n'.join(code)+'\n'
