#include "GhostKnowledge.hpp"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;
void check(bool condition, const char* label) { if (!condition) throw std::runtime_error(label); }

static std::string create_temp_folder(const std::string& prefix) {
    auto base = fs::temp_directory_path() / (prefix + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(base);
    return base.string();
}

int main() {
    std::string folder = create_temp_folder("ghost-knowledge");
    struct Cleanup { fs::path dir; ~Cleanup() { std::error_code ec; fs::remove_all(dir, ec); } } cleanup{folder};
    using namespace ghost_knowledge;
    std::string dir = folder;
    const std::string wiki = "[Pesquisa na internet — trecho da Wikipédia]\nTema encontrado: Física\nTexto público em português.\nFonte: https://pt.wikipedia.org/wiki/Fisica\nConsultado em: exemplo UTC\n";
    int calls = 0;
    auto fetch = [&](const std::string& topic) {
        ++calls; check(topic == "fisica", "canonical topic sent, no history");
        return wiki;
    };
    auto first = consult("Fisica", true, false, dir, fetch);
    check(first.found && first.saved && !first.cached && calls == 1, "new knowledge saved");
    auto again = consult("  fisica ", true, false, dir, fetch);
    check(again.cached && calls == 1 && again.text.find("Memoria local") != std::string::npos, "cache avoids network");
    auto offline = consult("fisica", false, true, dir, fetch);
    check(offline.cached && calls == 1, "offline never calls transport, even refresh");
    auto clock = consult("que horas sao agora", true, true, dir, fetch);
    check(!clock.allowed && !clock.saved && calls == 1, "clock never fetched or saved");
    Record unrelated;
    auto mismatch = consult("biologia", true, false, dir, [&](const auto&) { return wiki; });
    check(!mismatch.found && !mismatch.saved && !load(dir, "biologia", unrelated), "wrong topic not saved");
    auto missing = consult("biologia", false, false, dir, fetch);
    check(!missing.found && calls == 1, "offline miss");
    consult("fisica", true, true, dir, fetch);
    check(calls == 2, "explicit refresh");
    Record record;
    check(load(dir, "fisica", record) && record.content == wiki, "disk roundtrip");
    auto legacy = fs::path(dir) / "legacy";
    std::string legacy_error;
    check(save(legacy.string(), record, legacy_error), "legacy fixture created");
    for (const auto& file : fs::directory_iterator(legacy)) if (file.path().extension() == ".gkm") {
        auto wrong = wiki;
        wrong.replace(wrong.find("Tema encontrado: Física"), std::string("Tema encontrado: Física").size(), "Tema encontrado: Biologia");
        std::ofstream(file.path()) << "GHOST_KNOWLEDGE_1\n" << record.captured << '\n'
            << std::quoted(record.topic) << '\n' << std::quoted(record.source) << '\n' << std::quoted(wrong) << '\n';
    }
    check(!load(legacy.string(), "fisica", unrelated), "legacy unrelated title ignored");

    std::string error;
    record.captured = std::time(nullptr) - lifetime(record) - 1;
    check(save(dir, record, error), "expired fixture");
    auto stale = consult("fisica", false, false, dir, fetch);
    check(stale.cached && stale.stale && calls == 2, "expired offline labeled");
    auto failed = consult("fisica", true, false, dir, [](const auto&) { return "[Internet indisponivel] Teste"; });
    check(failed.cached && failed.stale && failed.text.find("desatualizada") != std::string::npos, "failed update preserves old record");
    auto updated = consult("fisica", true, false, dir, fetch);
    check(updated.saved && calls == 3, "expired online updates automatically");
    check(load(dir, "fisica", record) && std::time(nullptr) - record.captured < 5, "timestamp updated");
    auto invalid = consult("email@exemplo.com", true, false, dir, fetch);
    check(!invalid.found && calls == 3, "invalid query never fetched");
    // Legacy records must also be filtered without deleting the user's files.
    Record bad{"que horas sao agora", "https://pt.wikipedia.org/wiki/Que_Horas_Ela_Volta", wiki, static_cast<std::int64_t>(std::time(nullptr))};
    check(!save(dir, bad, error), "clock entry cannot enter persistent knowledge");
    auto garbage = consult("biologia", true, false, dir, [](const auto&) { return "erro sem fonte"; });
    check(!garbage.found && !load(dir, "biologia", record), "error not learned");
    garbage = consult("biologia", true, false, dir, [](const auto&) {
        return "[Pesquisa na internet]\nFonte: https://evil.example/fake\nConsultado em: agora\n";
    });
    check(!garbage.found && !load(dir, "biologia", record), "unexpected source not learned");
    const std::string official = "https://www.elysee.fr/en/french-presidency/the-presidents-of-the-republic";
    Record president{"presidente da franca", official,
        "[Consulta à fonte oficial] Nome Antigo\nFonte: " + official + "\nConsultado em: exemplo\n",
        static_cast<std::int64_t>(std::time(nullptr) - 3601)};
    check(save(dir, president, error), "official fixture");
    auto historical = consult("quem e o presidente da frnaca", false, false, dir, fetch);
    check(historical.stale && historical.text.find("Nome Antigo") == std::string::npos &&
        historical.text.find("Não confirmo") != std::string::npos, "expired office holder never asserted current");
    for (auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".gkm") { std::ofstream(entry.path()) << "truncated"; }
    }
    check(!load(dir, "fisica", record), "corrupt record ignored");
    check(consult("fisica", true, false, dir, fetch).saved, "corrupt record can be refreshed");
    auto blocked = fs::path(dir) / "not-a-directory";
    std::ofstream(blocked) << "file";
    auto no_write = consult("fisica", true, false, blocked.string(), fetch);
    check(no_write.found && !no_write.saved && no_write.text.find("não foi salva") != std::string::npos, "write failure reported");
    auto full = fs::path(dir) / "full";
    fs::create_directory(full);
    for (int n = 0; n < 500; ++n) std::ofstream(full / (std::to_string(n) + ".gkm")) << "existing";
    auto capacity = consult("fisica", true, false, full.string(), fetch);
    check(capacity.found && !capacity.saved && capacity.text.find("500") != std::string::npos, "bounded memory, no deletion");
    std::cout << "knowledge memory: OK (offline)\n";
}
