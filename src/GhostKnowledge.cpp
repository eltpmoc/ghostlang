#include "GhostKnowledge.hpp"
#include "GhostWeb.hpp"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;
namespace {
constexpr std::size_t max_records = 500;
constexpr std::size_t max_record_bytes = 24000;
const std::string official = "https://www.elysee.fr/en/french-presidency/the-presidents-of-the-republic";
std::string directory() {
    auto value = std::getenv("GHOST_KNOWLEDGE_DIR");
    return value && *value ? value : "v3/memoria";
}
std::string date(std::int64_t seconds) {
    std::time_t value = static_cast<std::time_t>(seconds);
    std::tm tm{};
#ifdef __linux__
    gmtime_r(&value, &tm);
#else
    tm = *std::gmtime(&value);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M UTC");
    return out.str();
}
fs::path path_for(const std::string& dir, const std::string& key) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : key) { hash ^= c; hash *= 1099511628211ULL; }
    std::ostringstream name;
    name << std::hex << hash << ".gkm";
    return fs::path(dir) / name.str();
}
bool accepted_source(const std::string& source) {
    return source == official || (source.rfind("https://pt.wikipedia.org/wiki/", 0) == 0 &&
        source.size() > 30 && source.size() < 2048 && source.find_first_of("\r\n\t ") == std::string::npos);
}
bool valid(const ghost_knowledge::Record& r) {
    if (r.topic != ghost_knowledge::key(r.topic) || !ghost_web::public_keywords(r.topic) ||
        r.captured <= 0 || r.captured > std::time(nullptr) + 300 ||
        r.content.empty() || r.content.size() > 20000 || !accepted_source(r.source)) return false;
    if (r.content.find("\nFonte: " + r.source + "\n") == std::string::npos) return false;
    if (r.source == official) {
        if (r.content.rfind("[Consulta à fonte oficial]", 0) != 0) return false;
    } else {
        if (r.content.rfind("[Pesquisa na internet", 0) != 0) return false;
        const std::string marker = "Tema encontrado: ";
        auto start = r.content.find(marker);
        if (start == std::string::npos) return false;
        start += marker.size();
        if (!ghost_web::relevant_title(r.topic, r.content.substr(start, r.content.find('\n', start) - start))) return false;
    }
    for (unsigned char c : r.content) if ((c < 32 && c != '\n' && c != '\t') || c == 127) return false;
    return true;
}
bool decode_record(const fs::path& file, ghost_knowledge::Record& r) {
    std::error_code error;
    if (!fs::is_regular_file(fs::symlink_status(file, error)) || error || fs::file_size(file, error) > max_record_bytes || error) return false;
    std::ifstream input(file, std::ios::binary);
    std::string version;
    std::getline(input, version);
    if (version != "GHOST_KNOWLEDGE_1") return false;
    if (!(input >> r.captured >> std::quoted(r.topic) >> std::quoted(r.source) >> std::quoted(r.content))) return false;
    input >> std::ws;
    return input.eof() && valid(r);
}
std::string remembered(const ghost_knowledge::Record& r, bool stale) {
    std::string header = stale ? "[Memoria local desatualizada] " : "[Memoria local] ";
    header += "Tema: " + r.topic + " | obtido em " + date(r.captured) + ".\n";
    if (stale && r.source == official)
        return header + "O registro sobre o ocupante do cargo venceu. Não confirmo quem ocupa o cargo agora sem atualizar a fonte.\nFonte: " + r.source;
    if (stale) header += "Prazo de atualização vencido; este trecho arquivado não foi confirmado agora.\n";
    auto content = r.content;
    auto end = content.find('\n');
    if (content.rfind("[Pesquisa na internet", 0) == 0 && end != std::string::npos)
        content = "[Trecho arquivado da Wikipédia]" + content.substr(end);
    else if (content.rfind("[Consulta à fonte oficial]", 0) == 0)
        content.replace(0, std::string("[Consulta à fonte oficial]").size(), "[Registro da fonte oficial]");
    return header + content + "\nReutilizado da base local; nenhuma nova consulta à rede foi feita para este trecho.";
}
}

namespace ghost_knowledge {
std::string key(const std::string& topic) {
    std::istringstream input(topic);
    std::string word, result;
    while (input >> word) {
        for (char& c : word) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (!result.empty()) result += ' ';
        result += word;
    }
    if (ghost_web::current_french_president(result)) return "presidente da franca";
    return result;
}
std::int64_t lifetime(const Record& r) {
    if (r.source == official) return 3600;
    std::istringstream input(r.topic);
    std::string word;
    while (input >> word)
        for (auto current : {"hoje", "atual", "atualmente", "presidente", "preco", "noticias", "ultimo", "ultima"})
            if (word == current) return 3600;
    return 7 * 24 * 3600;
}
bool load(const std::string& dir, const std::string& topic, Record& record) {
    const auto canonical = key(topic);
    if (!ghost_web::public_keywords(canonical)) return false;
    Record candidate;
    if (!decode_record(path_for(dir, canonical), candidate) || candidate.topic != canonical) return false;
    record = std::move(candidate);
    return true;
}
bool save(const std::string& dir, const Record& record, std::string& error) {
    if (!valid(record)) { error = "registro sem fonte ou formato válido"; return false; }
    std::ostringstream output;
    output << "GHOST_KNOWLEDGE_1\n" << record.captured << '\n' << std::quoted(record.topic) << '\n'
           << std::quoted(record.source) << '\n' << std::quoted(record.content) << '\n';
    const auto bytes = output.str();
    if (bytes.size() > max_record_bytes) { error = "registro grande demais"; return false; }
#ifdef __linux__
    int lock = -1, fd = -1;
    std::string temp;
    try {
        fs::create_directories(dir);
        lock = open((fs::path(dir) / ".lock").c_str(), O_CREAT | O_WRONLY | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (lock < 0 || flock(lock, LOCK_EX | LOCK_NB) != 0) throw std::runtime_error("memória ocupada ou sem permissão");
        auto destination = path_for(dir, record.topic);
        std::size_t count = 0;
        for (const auto& entry : fs::directory_iterator(dir)) if (entry.path().extension() == ".gkm") ++count;
        if (count >= max_records && !fs::exists(destination)) throw std::runtime_error("limite de 500 registros atingido; nenhum registro antigo foi apagado");
        auto pattern = (fs::path(dir) / ".record-XXXXXX").string();
        std::vector<char> name(pattern.begin(), pattern.end());
        name.push_back('\0');
        fd = mkstemp(name.data());
        if (fd < 0) throw std::runtime_error("não foi possível criar o registro");
        temp = name.data();
        std::size_t done = 0;
        while (done < bytes.size()) {
            auto n = write(fd, bytes.data() + done, bytes.size() - done);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) throw std::runtime_error("falha na escrita");
            done += static_cast<std::size_t>(n);
        }
        if (fsync(fd) != 0) throw std::runtime_error("falha ao sincronizar o registro");
        close(fd); fd = -1;
        fs::rename(temp, destination);
        close(lock);
        return true;
    } catch (const std::exception& e) {
        if (fd >= 0) close(fd);
        if (lock >= 0) close(lock);
        if (!temp.empty()) { std::error_code ignored; fs::remove(temp, ignored); }
        error = e.what();
        return false;
    }
#else
    try {
        fs::create_directories(dir);
        std::size_t count = 0;
        for (const auto& entry : fs::directory_iterator(dir)) if (entry.path().extension() == ".gkm") ++count;
        auto destination = path_for(dir, record.topic);
        if (count >= max_records && !fs::exists(destination)) throw std::runtime_error("limite de 500 registros atingido; nenhum registro antigo foi apagado");
        auto temp = (fs::path(dir) / (".record-temp-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))).string();
        std::ofstream out(temp, std::ios::binary);
        if (!out) throw std::runtime_error("não foi possível criar o registro em: " + temp);
        out.write(bytes.data(), bytes.size());
        out.close();
        if (fs::exists(destination)) {
            std::error_code ignored;
            fs::remove(destination, ignored);
        }
        fs::rename(temp, destination);
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
#endif
}
GhostKnowledgeResult consult(const std::string& topic, bool online, bool refresh,
                            const std::string& dir, const Fetch& fetch) {
    GhostKnowledgeResult result;
    const auto canonical = key(topic);
    if (ghost_web::local_clock_query(topic)) {
        result.allowed = false;
        result.text = "[Relogio local] Consulte o relógio do sistema; no chat V4, use /agora. Horários não são conhecimento enciclopédico persistente.";
        return result;
    }
    if (!ghost_web::public_keywords(topic) || !ghost_web::public_keywords(canonical)) {
        result.allowed = false;
        result.text = "[Memoria] Tema inválido: use até 16 palavras públicas e 160 bytes, sem dados pessoais, URLs ou código.";
        return result;
    }
    Record old;
    bool have = load(dir, canonical, old);
    bool stale = have && std::time(nullptr) >= old.captured + lifetime(old);
    if (have && !stale && !refresh) {
        result.found = true; result.cached = true; result.text = remembered(old, false);
        return result;
    }
    if (!online) {
        result.found = have; result.cached = have; result.stale = stale;
        result.text = have ? remembered(old, stale) : "[Sem fonte suficiente] Tema ausente da memória local e internet desativada. Não confirmei a informação.";
        return result;
    }
    std::string answer;
    try { answer = fetch(canonical); }
    catch (const std::exception&) { answer = "[Internet indisponivel] Falha ao consultar a fonte."; }
    Record fresh{canonical, "", answer, static_cast<std::int64_t>(std::time(nullptr))};
    auto pos = answer.rfind("\nFonte: ");
    if (pos != std::string::npos) {
        auto end = answer.find('\n', pos + 8);
        if (end != std::string::npos) fresh.source = answer.substr(pos + 8, end - pos - 8);
    }
    if (!valid(fresh)) {
        result.found = have; result.cached = have; result.stale = stale;
        result.text = answer.rfind("[Pesquisa na internet", 0) == 0
            ? "[Sem fonte suficiente] Resultado descartado: fonte, formato ou titulo sem correspondencia suficiente com o tema. Nao foi salvo."
            : answer;
        if (have) result.text += "\n\n" + remembered(old, stale);
        return result;
    }
    result.found = true;
    std::string error;
    result.saved = save(dir, fresh, error);
    result.text = answer + (result.saved ? "\n[Memoria] Conhecimento salvo com fonte e data para próximas consultas." :
        "\n[Memoria] Resposta disponível, mas não foi salva: " + error + ".");
    return result;
}
}

GhostKnowledgeResult knowledge_consult(const std::string& topic, bool online, bool refresh) {
    return ghost_knowledge::consult(topic, online, refresh, directory(), web_search_public);
}
std::string knowledge_info() {
    std::vector<ghost_knowledge::Record> records;
    std::size_t invalid = 0;
    std::error_code error;
    auto dir = directory();
    if (fs::exists(dir, error)) {
        fs::directory_iterator iterator(dir, error), end;
        if (error) return "[Memoria] Não foi possível ler o diretório local.";
        for (; iterator != end && !error; iterator.increment(error)) {
            if (iterator->path().extension() != ".gkm") continue;
            ghost_knowledge::Record record;
            if (decode_record(iterator->path(), record)) records.push_back(std::move(record)); else ++invalid;
        }
    }
    std::sort(records.begin(), records.end(), [](const auto& a, const auto& b) { return a.topic < b.topic; });
    std::ostringstream out;
    out << "[Memoria] " << records.size() << " registros locais com fonte; limite " << max_records << ".\nDiretório: " << dir;
    if (invalid) out << "\nRegistros inválidos ignorados: " << invalid;
    std::size_t shown = 0;
    for (const auto& record : records) {
        if (shown++ == 30) { out << "\nLista limitada aos primeiros 30 temas."; break; }
        out << "\n- " << record.topic << " | " << date(record.captured)
            << (std::time(nullptr) >= record.captured + ghost_knowledge::lifetime(record) ? " | atualizar" : " | dentro do prazo")
            << "\n  " << record.source;
    }
    return out.str();
}
