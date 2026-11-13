#include "GhostTemporal.hpp"
#include "GhostWeb.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
std::tm local(std::time_t value) {
    std::tm out{};
#ifdef _WIN32
    std::tm* pt = std::localtime(&value);
    if (!pt) throw std::runtime_error("Relogio fora do intervalo suportado.");
    out = *pt;
#else
    if (!localtime_r(&value, &out)) throw std::runtime_error("Relogio fora do intervalo suportado.");
#endif
    return out;
}
std::string format(std::time_t value, const char* pattern) {
    auto tm = local(value); std::ostringstream out; out << std::put_time(&tm, pattern); return out.str();
}
std::string day(std::time_t value) {
    const char* weekdays[] = {"domingo", "segunda-feira", "terca-feira", "quarta-feira", "quinta-feira", "sexta-feira", "sabado"};
    return format(value, "%d/%m/%Y") + " (" + weekdays[local(value).tm_wday] + ")";
}
std::time_t calendar_day(std::time_t value, int offset) {
    auto tm = local(value);
    // Calendar addition, not offset*86400: DST days need not last 24 hours.
    tm.tm_hour = 12; tm.tm_min = 0; tm.tm_sec = 0; tm.tm_mday += offset; tm.tm_isdst = -1;
    auto result = std::mktime(&tm);
    if (result == std::time_t(-1)) throw std::runtime_error("Data fora do intervalo suportado.");
    return result;
}
std::string duration(std::int64_t milliseconds) {
    auto seconds = std::max(std::int64_t(0), milliseconds) / 1000;
    return std::to_string(seconds / 3600) + " h " + std::to_string(seconds / 60 % 60) + " min " + std::to_string(seconds % 60) + " s";
}
bool match(const std::string& text, const char* expression) { return std::regex_search(text, std::regex(expression)); }
const char* source = " Fonte: relogio deste computador; fuso local do sistema.";
}
GhostTemporalContext::GhostTemporalContext() : GhostTemporalContext(
    [] { return std::time(nullptr); },
    [] { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }) {}
GhostTemporalContext::GhostTemporalContext(WallClock wall, TickClock ticks) : wall_(std::move(wall)), ticks_(std::move(ticks)) { clear(); }
void GhostTemporalContext::clear() {
    started_ = wall_(); start_tick_ = ticks_(); count_ = 0;
    last_wall_ = previous_wall_ = 0; last_tick_ = previous_tick_ = 0;
}
void GhostTemporalContext::observe() {
    previous_wall_ = last_wall_; previous_tick_ = last_tick_;
    last_wall_ = wall_(); last_tick_ = ticks_(); ++count_;
}
std::string GhostTemporalContext::clock() const {
    const auto current = wall_();
    return "[Relogio local] Data: " + format(current,"%d/%m/%Y") + ". Hora: " + format(current,"%H:%M:%S") +
        ". Fuso do sistema: " + format(current,"%Z (UTC%z)") + ". Dia da semana: " + day(current) + "." + source;
}
std::string GhostTemporalContext::info() const {
    std::string out = "[Contexto temporal] " + clock() + "\nConversa iniciada em " + format(started_, "%d/%m/%Y %H:%M:%S %Z (UTC%z)") +
        ". Tempo decorrido: " + duration(ticks_()-start_tick_) + " (contador monotonico).\nMensagens recebidas: " + std::to_string(count_) + ".";
    if (count_) out += " Ultima mensagem recebida em " + format(last_wall_, "%d/%m/%Y %H:%M:%S %Z (UTC%z)") +
        "; ha " + duration(ticks_()-last_tick_) + ".";
    out += "\nTenho nocao operacional de tempo: consulto calendario e acompanho intervalos nesta conversa. Isso nao implica experiencia subjetiva. /limpar reinicia estas medidas; nao retenho horarios pessoais entre sessoes.";
    return out;
}
std::string GhostTemporalContext::previous() const {
    if (count_ < 2) return "[Contexto temporal] Ainda nao ha uma mensagem anterior nesta conversa para medir o intervalo.";
    return "[Contexto temporal] Entre sua mensagem anterior (" + format(previous_wall_, "%d/%m/%Y %H:%M:%S %Z (UTC%z)") +
        ") e esta passaram " + duration(last_tick_-previous_tick_) + ". Medicao pelo contador monotonico, ate a chegada desta mensagem.";
}
std::string GhostTemporalContext::answer(const std::string& text) const {
    if (text.size()>4000) return "";
    // Keep titles, scientific questions and durations as separate topics.
    if (match(text, "\\b(filme|livro|musica|serie|horas ela volta)\\b")) return "";
    if (match(text, "(quanto tempo|quando|demorei).*(mensagem|mensagens|pergunta|responder|voltar)")) return previous();
    if (match(text, "(quanto tempo|quando).*(convers|sessao|estamos aqui)|\\b(nocao|nossao|percepcao) (do|de) tempo\\b")) return info();
    std::smatch calculation;
    if (std::regex_search(text, calculation, std::regex("\\b(daqui a|ha) ([0-9]+) (minutos?|horas?|dias?)$"))) {
        if (calculation[2].length()>4 || std::stoi(calculation[2])>3660)
            return "[Contexto temporal] Use um intervalo de 0 a 3660 minutos, horas ou dias.";
        int amount = std::stoi(calculation[2]); if(calculation[1]=="ha") amount = -amount;
        auto current = wall_(); std::string unit=calculation[3];
        if (unit.rfind("dia",0)==0) return "[Calendario local] " + calculation.str() + ": " + day(calendar_day(current, amount)) + "." + source;
        auto result = current + std::time_t(amount) * (unit.rfind("hora",0)==0 ? 3600 : 60);
        return "[Calculo temporal] " + calculation.str() + ": " + format(result,"%d/%m/%Y %H:%M:%S %Z (UTC%z)") +
            ". Calculado a partir de " + format(current,"%d/%m/%Y %H:%M:%S %Z (UTC%z)") + ". Nao e um lembrete agendado." + source;
    }
    if (match(text, "^(hoje|ontem|anteontem|amanha|depois de amanha)$") ||
        (match(text,"\\b(hoje|ontem|anteontem|amanha)\\b") && match(text,"que dia|qual.*(dia|data)|que data"))) {
        auto current=wall_(); std::string out="[Calendario local] ";
        for (auto item : {std::pair<const char*,int>{"anteontem",-2},{"ontem",-1},{"hoje",0},{"depois de amanha",2},{"amanha",1}}) {
            if (std::string(item.first)=="amanha" && text.find("depois de amanha")!=std::string::npos) continue;
            auto pattern=std::string("\\b")+item.first+"\\b";
            if (match(text,pattern.c_str())) out += std::string(item.first)+": "+day(calendar_day(current,item.second))+"; ";
        }
        return out + "Fuso: " + format(current,"%Z (UTC%z)") + "." + source;
    }
    if (ghost_web::local_clock_query(text)) return clock();
    return "";
}
