// ============================================================
// Biblioteca Padrao: GhostJson v1.0
// Manipulacao e parsing de JSON nativo no GhostLang
// ============================================================

// O tipo 'json' e uma estrutura nativa do GhostLang implementada no runtime C++
// com parsing de alta performance, acesso indexado por chave obj["campo"] e metodos:
// - obj.toString() / obj.to_string()
// - json.parse(string)
// - json.stringify(obj)
// - obj.get(chave)
// - obj.has(chave)

function string json_to_string(json obj) {
    return obj.toString();
}

function json parse_json(string str) {
    return json.parse(str);
}
