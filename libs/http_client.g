// ============================================================
// Biblioteca Padrao: GhostHTTPClient v1.0
// Modulo para consumo de APIs REST e requests externos
// ============================================================

function string http_client_get(string url) {
    return http_get(url);
}

function string http_client_post(string url, string json_body) {
    return http_post(url, json_body);
}
