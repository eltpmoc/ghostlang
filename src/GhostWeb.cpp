#include "GhostWeb.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace {
constexpr size_t limit = 2097152;
const std::string official = "https://www.elysee.fr/en/french-presidency/the-presidents-of-the-republic";
const std::string api = "https://pt.wikipedia.org/w/api.php";
std::string normalized(std::string value) {
    for (auto pair : {std::pair<const char*,const char*>{"á","a"}, {"à","a"}, {"ã","a"}, {"â","a"},
            {"Á","a"}, {"À","a"}, {"Ã","a"}, {"Â","a"}, {"é","e"}, {"ê","e"}, {"É","e"}, {"Ê","e"},
            {"í","i"}, {"Í","i"}, {"ó","o"}, {"ô","o"}, {"õ","o"}, {"Ó","o"}, {"Ô","o"}, {"Õ","o"},
            {"ú","u"}, {"ü","u"}, {"Ú","u"}, {"Ü","u"}, {"ç","c"}, {"Ç","c"}}) {
        size_t p = 0;
        while ((p = value.find(pair.first, p)) != std::string::npos) value.replace(p, std::string(pair.first).size(), pair.second);
    }
    for (char& c : value) { if (c >= 'A' && c <= 'Z') c += 'a'-'A'; if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9')) c = ' '; }
    value = std::regex_replace(value, std::regex(" +"), " ");
    return std::regex_replace(value, std::regex("^ +| +$"), "");
}
std::string decode(std::string s) {
    for (auto pair : {std::pair<const char*,const char*>{"&lt;","<"}, {"&gt;",">"},
                      {"&quot;","\""}, {"&apos;","'"}, {"&amp;","&"}}) {
        size_t p = 0;
        while ((p = s.find(pair.first, p)) != std::string::npos) {
            s.replace(p, std::string(pair.first).size(), pair.second);
            p += std::string(pair.second).size();
        }
    }
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
        return (c < 32 && c != '\n' && c != '\t') || c == 127;
    }), s.end());
    return s;
}
std::string attr(const std::string& tag, const std::string& name) {
    auto p = tag.find(" " + name + "=\"");
    if (p == std::string::npos) return "";
    p += name.size() + 3;
    auto e = tag.find('"', p);
    return e == std::string::npos ? "" : decode(tag.substr(p, e-p));
}
std::string encode(const std::string& value) {
    const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : value) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_') out += c;
        else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
    }
    return out;
}
std::string utc() {
    auto t = std::time(nullptr);
    std::tm tm{};
#ifdef __linux__
    gmtime_r(&t, &tm);
#else
    tm = *std::gmtime(&t);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M UTC");
    return out.str();
}
std::string get(const std::string& url) {
#ifdef __linux__
    // Fixed hosts, HTTPS, no redirects, no shell or curlrc. Only GET.
    if (url != official && url.rfind(api + "?", 0) != 0) throw std::runtime_error("Destino nao permitido.");
    int pipefd[2];
    if (pipe2(pipefd, O_CLOEXEC) != 0) throw std::runtime_error("Falha ao abrir transporte.");
    std::vector<std::string> args = {"/usr/bin/curl", "-q", "--silent", "--fail", "--proto", "=https",
        "--connect-timeout", "4", "--max-time", "10", "--max-filesize", "2097152",
        "--user-agent", "GhostLangLocalAI/3.3", "--write-out", "\nGHOST_HTTP_CODE:%{http_code}", url};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipefd[0]);
    posix_spawn_file_actions_addclose(&actions, pipefd[1]);
    posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    pid_t pid;
    int err = posix_spawn(&pid, argv[0], &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    close(pipefd[1]);
    if (err) { close(pipefd[0]); throw std::runtime_error("curl indisponivel em /usr/bin/curl."); }
    std::string body;
    bool failed = false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(12);
    while (true) {
        if (std::chrono::steady_clock::now() >= deadline) { failed = true; break; }
        pollfd fd{pipefd[0], POLLIN, 0};
        int ready = poll(&fd, 1, 200);
        if (ready < 0) { if (errno == EINTR) continue; failed = true; break; }
        if (!ready) continue;
        char chunk[8192];
        auto n = read(pipefd[0], chunk, sizeof(chunk));
        if (!n) break;
        if (n < 0) { if (errno == EINTR) continue; failed = true; break; }
        body.append(chunk, static_cast<size_t>(n));
        if (body.size() > limit + 64) { failed = true; break; }
    }
    close(pipefd[0]);
    if (failed) kill(pid, SIGKILL);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    if (failed || !WIFEXITED(status) || WEXITSTATUS(status)) throw std::runtime_error("Falha de rede, tempo limite ou resposta recusada.");
    const std::string marker = "\nGHOST_HTTP_CODE:200";
    if (body.size() < marker.size() || body.compare(body.size()-marker.size(), marker.size(), marker) != 0) throw std::runtime_error("A fonte nao retornou HTTP 200.");
    body.resize(body.size()-marker.size());
    return body;
#else
    if (url != official && url.rfind(api + "?", 0) != 0) throw std::runtime_error("Destino nao permitido.");
    std::string cmd = "curl.exe -q --silent --fail --proto =https --connect-timeout 4 --max-time 10 --max-filesize 2097152 --user-agent \"GhostLangLocalAI/3.3\" --write-out \"\\nGHOST_HTTP_CODE:%{http_code}\" --url \"" + url + "\"";
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("curl.exe indisponivel no Windows.");
    std::string body;
    char chunk[8192];
    while (fgets(chunk, sizeof(chunk), pipe) != nullptr) {
        body += chunk;
        if (body.size() > limit + 64) { _pclose(pipe); throw std::runtime_error("Resposta muito grande."); }
    }
    int status = _pclose(pipe);
    if (status != 0) throw std::runtime_error("Falha de rede, tempo limite ou resposta recusada.");
    const std::string marker = "\nGHOST_HTTP_CODE:200";
    if (body.size() < marker.size() || body.compare(body.size()-marker.size(), marker.size(), marker) != 0) throw std::runtime_error("A fonte nao retornou HTTP 200.");
    body.resize(body.size()-marker.size());
    return body;
#endif
}
}

namespace ghost_web {
bool local_clock_query(const std::string& value) {
    auto text = normalized(value);
    if (std::regex_search(text, std::regex("\\b(filme|livro|musica|serie|horas ela volta)\\b"))) return false;
    return std::regex_search(text, std::regex(
        "\\b(que horas (sao|e)|que hora e|qual (e )?o horario( atual| agora| de hoje)?$|"
        "(horario|hora|data|dia|ano).*(hoje|agora|atual)|"
        "(hoje|agora).*(horario|hora|data|dia)|que dia e|em que (dia|ano) estamos|"
        "(nocao|nossao) de tempo|quantas horas( sao)?( agora)?$)\\b"));
}
bool relevant_title(const std::string& topic, const std::string& title) {
    if (local_clock_query(topic)) return false;
    const std::set<std::string> ignored = {"a","o","as","os","e","de","da","do","das","dos","em","no","na","um","uma","que","qual","quais","quem","sao","como","sobre","por","para","explique","descreva","historia","funciona","funcionam"};
    auto terms = [&](const std::string& input) {
        std::set<std::string> out; std::istringstream words(normalized(input)); std::string word;
        while (words >> word) if (!ignored.count(word)) out.insert(word);
        return out;
    };
    auto query = terms(topic), heading = terms(title);
    size_t hits = 0;
    for (const auto& term : query) if (heading.count(term)) ++hits;
    // Conservative lexical evidence, not semantic verification.
    return !query.empty() && hits >= std::min(size_t(2), query.size()) && hits * 2 >= query.size();
}
bool current_french_president(const std::string& value) {
    std::istringstream words(value);
    std::string term, clean;
    while (words >> term) {
        if (term == "frnaca") term = "franca";
        if (!clean.empty()) clean += ' ';
        clean += term;
    }
    // Historical questions and comparisons must not get today's office holder.
    for (const auto* allowed : {"presidente franca", "presidente da franca", "atual presidente da franca",
            "presidente atual da franca", "quem e o presidente da franca", "qual e o presidente da franca",
            "quem e o atual presidente da franca"}) {
        if (clean == allowed) return true;
    }
    return false;
}
bool public_keywords(const std::string& value) {
    if (value.empty() || value.size() > 160) return false;
    int words = 0;
    bool in_word = false;
    for (unsigned char c : value) {
        // Deliberately restrict to public topic words, excluding URLs, email, numbers and code.
        if (!(c >= 128 || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == ' ' || c == '-')) return false;
        if (c == ' ') in_word = false;
        else if (!in_word) { in_word = true; ++words; }
    }
    return words > 0 && words <= 16;
}
std::string president(const std::string& html) {
    if (html.size() > limit) return "";
    size_t p = 0;
    std::string found;
    while ((p = html.find("<a ", p)) != std::string::npos) {
        auto end = html.find("</a>", p);
        if (end == std::string::npos) break;
        auto tag = html.substr(p, end-p);
        p = end + 4;
        auto start = tag.find("<span>");
        auto stop = tag.find("</span>", start);
        if (attr(tag, "href").rfind("/en/", 0) != 0 || start == std::string::npos || stop == std::string::npos) continue;
        std::string state = tag.substr(stop + 7);
        state.erase(std::remove_if(state.begin(), state.end(), [](unsigned char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; }), state.end());
        if (state != "Inoffice") continue;
        auto name = decode(tag.substr(start + 6, stop - start - 6));
        if (name.empty() || name.size() > 120 || name.find('<') != std::string::npos || !found.empty()) return "";
        found = name;
    }
    return found;
}
std::string page_id(const std::string& xml) {
    if (xml.size() > limit) return "";
    auto p = xml.find("<p ");
    auto e = xml.find('>', p);
    if (p == std::string::npos || e == std::string::npos) return "";
    auto id = attr(xml.substr(p, e-p), "pageid");
    if (id.empty() || id.size() > 12 || id.find_first_not_of("0123456789") != std::string::npos) return "";
    return id;
}
std::string encyclopedia(const std::string& xml) {
    if (xml.size() > limit) return "";
    auto p = xml.find("<page ");
    auto e = xml.find('>', p);
    if (p == std::string::npos || e == std::string::npos) return "";
    auto title = attr(xml.substr(p, e-p), "title");
    auto start = xml.find("<extract ", e);
    auto stop = xml.find("</extract>", start);
    if (start == std::string::npos || stop == std::string::npos || title.empty()) return "";
    start = xml.find('>', start) + 1;
    auto extract = decode(xml.substr(start, stop-start));
    if (extract.empty() || extract.size() > 16000) return "";
    return "[Pesquisa na internet — trecho da Wikipédia]\nTema encontrado: " + title + "\n\n" + extract +
        "\n\nFonte: https://pt.wikipedia.org/wiki/" + encode(title) + "\nConsultado em: " + utc() +
        "\nTrecho relacionado à busca; pode não responder à pergunta inteira nem confirmar fatos atuais.";
}
}
std::string web_search_public(const std::string& keywords) {
    if (ghost_web::local_clock_query(keywords)) return "[Relogio local] Data e hora devem ser consultadas no relógio do sistema. No chat V4, use /agora.";
    if (!ghost_web::public_keywords(keywords)) return "[Internet] Use até 16 palavras-chave públicas (160 bytes), sem números, URLs, e-mails ou código. Não envie dados pessoais ou segredos.";
    try {
        if (ghost_web::current_french_president(keywords)) {
            auto name = ghost_web::president(get(official));
            if (name.empty()) return "[Internet] A fonte oficial não permitiu identificar com segurança quem está no cargo.";
            auto correction = keywords.find("frnaca") != std::string::npos ? "Interpretando Frnaça como França.\n" : "";
            return std::string("[Consulta à fonte oficial] ") + correction + "O presidente da França é " + name +
                ". A Presidência francesa o identifica como ocupante atual do cargo.\nFonte: " + official + "\nConsultado em: " + utc();
        }
        auto id = ghost_web::page_id(get(api + "?action=query&list=search&srlimit=1&format=xml&srsearch=" + encode(keywords)));
        if (id.empty()) return "[Internet] Não encontrei um artigo relacionado. Tente palavras-chave mais específicas.";
        auto answer = ghost_web::encyclopedia(get(api + "?action=query&prop=extracts&exintro=1&explaintext=1&exchars=1200&format=xml&pageids=" + id));
        const std::string marker = "Tema encontrado: ";
        auto start = answer.find(marker);
        auto title = start == std::string::npos ? "" : answer.substr(start + marker.size(), answer.find('\n', start) - start - marker.size());
        if (!answer.empty() && !ghost_web::relevant_title(keywords, title)) return "[Sem fonte suficiente] O título encontrado não corresponde suficientemente ao tema pedido. Não usei nem salvei esse resultado. Reformule com o nome do tema.";
        return answer.empty() ? "[Internet] A fonte não forneceu um trecho utilizável." : answer;
    } catch (const std::exception& e) {
        return std::string("[Internet indisponivel] ") + e.what() + " A consulta local continua disponível; não confirmei a informação solicitada.";
    }
}
