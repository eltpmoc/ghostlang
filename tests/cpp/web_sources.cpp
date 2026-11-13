#include "GhostWeb.hpp"
#include <iostream>
#include <stdexcept>

void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    using namespace ghost_web;
    check(public_keywords("computacao quantica"), "public topic");
    for (auto query : {"Que horas são agora?", "Olá, quantas horas?", "Qual é a data de hoje?", "Que dia é hoje?",
            "Estou perguntado se você tem nossão de tempo e qual é o Horário e data de hoje", "Qual é o horário?"})
        check(ghost_web::local_clock_query(query), "clock intent");
    for (auto query : {"Que Horas Ela Volta?", "explique o filme Que Horas Ela Volta", "Quantas horas dura um dia?",
            "Historia do relogio", "O que e tempo na fisica", "computacao quantica"})
        check(!ghost_web::local_clock_query(query), "non-clock topic preserved");
    check(relevant_title("computacao quantica", "Computação quântica"), "accented relevant title");
    check(relevant_title("que horas ela volta", "Que Horas Ela Volta?"), "actual movie topic");
    check(!relevant_title("que horas sao agora", "Que Horas Ela Volta?"), "movie is not clock");
    check(!relevant_title("biologia marinha", "Biologia molecular"), "weak overlap rejected");
    check(web_search_public("que horas sao agora").find("Relogio local") != std::string::npos, "clock never uses transport");

    check(current_french_president("presidente da frnaca"), "explicit typo correction");
    check(!current_french_president("primeiro presidente da franca"), "historical query is not current");
    check(!current_french_president("presidente da franca e alemanha"), "comparison is not single office");
    check(!public_keywords("   "), "empty topic");
    for (auto query : {"email@exemplo.com", "https://exemplo.com", "$(touch /tmp/test)", "123456", "\033[2J"})
        check(!public_keywords(query), "private/code payload rejected");
    check(!public_keywords(std::string(161, 'a')), "query size bound");
    auto holder = "<a href=\"/en/test\"><span>Nome Dinâmico</span>\n In office </a>";
    check(president(holder) == "Nome Dinâmico", "dynamic incumbent");
    check(president("<a href=\"/en/past\"><span>Antigo</span>2017 - 2022</a>").empty(), "no historical fallback");
    check(president(std::string(holder)+holder).empty(), "ambiguous current holder");
    check(president("<a href=\"https://other/\"><span>Falso</span>In office</a>").empty(), "expected source link");
    check(page_id("<api><search><p pageid=\"12699\" title=\"Teste\" /></search></api>") == "12699", "search page id");
    check(page_id("<p pageid=\"12&bad=1\" />").empty(), "page id injection");
    check(page_id("<api><search /></api>").empty(), "empty results");
    auto text = encyclopedia("<page title=\"Computação &amp; teste\"><extract xml:space=\"preserve\">Texto em português.\033[2J</extract></page>");
    check(text.find("Computação & teste") != std::string::npos, "entities and UTF8");
    check(text.find("Fonte: https://pt.wikipedia.org/wiki/") != std::string::npos, "attribution");
    check(text.find("Consultado em:") != std::string::npos, "timestamp");
    check(text.find('\033') == std::string::npos, "terminal escape stripped");
    check(encyclopedia("<page title=\"Teste\"><extract /></page>").empty(), "empty extract");
    check(encyclopedia(std::string(2097153,'x')).empty(), "response size bound");
    check(web_search_public("email@exemplo.com").find("palavras-chave") != std::string::npos, "invalid payload rejected before network");
    std::cout << "web sources: OK (offline)\n";
}
