// ============================================================
// Biblioteca Padrao: GhostHTTP v2.0
// Modulo completo para Servidor Web em GhostLang
// Suporte: JSON, HTML, CSS, JavaScript, arquivos estáticos
// ============================================================

// Formato interno de resposta: STATUS:###|TYPE:mime|BODY:conteudo

// --- Primitiva Base ---
function string http_response_type(string code, string type, string body) {
    return "STATUS:" + code + "|TYPE:" + type + "|BODY:" + body;
}

// --- JSON Responses ---
function string http_response(string code, string body) {
    return http_response_type(code, "application/json", body);
}

function string http_ok(string json_body) {
    return http_response("200", json_body);
}

function string http_created(string json_body) {
    return http_response("201", json_body);
}

function string http_bad_request(string message) {
    return http_response("400", "{\"error\": \"" + message + "\"}");
}

function string http_unauthorized(string message) {
    return http_response("401", "{\"error\": \"" + message + "\"}");
}

function string http_not_found(string message) {
    return http_response("404", "{\"error\": \"" + message + "\"}");
}

function string http_internal_error(string message) {
    return http_response("500", "{\"error\": \"" + message + "\"}");
}

// --- HTML / JS / CSS Inline ---
function string http_html(string html_content) {
    return http_response_type("200", "text/html", html_content);
}

function string http_js(string js_content) {
    return http_response_type("200", "application/javascript", js_content);
}

function string http_css(string css_content) {
    return http_response_type("200", "text/css", css_content);
}

// --- Arquivos do Disco (Usando C++ Nativo) ---

// Serve um arquivo com Content-Type explicito
function string http_file(string filepath, string content_type) {
    return ghost_http_file(filepath, content_type);
}

// Serve um arquivo HTML do disco
function string http_html_file(string filepath) {
    return ghost_http_file(filepath, "text/html");
}

// Serve um arquivo JS do disco
function string http_js_file(string filepath) {
    return ghost_http_file(filepath, "application/javascript");
}

// Serve um arquivo CSS do disco
function string http_css_file(string filepath) {
    return ghost_http_file(filepath, "text/css");
}

// Serve qualquer arquivo detectando o Content-Type pelo sufixo
function string http_static(string filepath) {
    return ghost_http_auto_file(filepath);
}

