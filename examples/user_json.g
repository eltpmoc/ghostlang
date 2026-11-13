import "libs/json.g";

function main(){
    // Criando um objeto JSON
    json obj = {
        "nome": "Prioneiro Quântico",
        "ano": 2026,
        "linguagem": "GhostLang"
    };

    // Convertendo o objeto JSON para string
    string jsonString = obj.toString();

    println("Objeto JSON:");
    println(jsonString);

    // Parseando a string JSON de volta para um objeto
    json parsedObj = json.parse(jsonString);

    println("Nome do Prioneiro: " + parsedObj["nome"]);
    println("Ano de consolidação da linguagem: " + parsedObj["ano"]);
}
