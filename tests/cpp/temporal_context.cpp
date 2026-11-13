#include "GhostTemporal.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <ctime>

#ifdef _WIN32
// Equivalentes Windows (MSVCRT/UCRT) das funcoes POSIX usadas no teste
static std::time_t timegm(std::tm* tm) { return _mkgmtime(tm); }
static int setenv(const char* name, const char* value, int) { return _putenv_s(name, value); }
#endif

void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::time_t utc(int year, int month, int date, int hour=0, int minute=0) {
    std::tm tm{}; tm.tm_year=year-1900; tm.tm_mon=month-1; tm.tm_mday=date; tm.tm_hour=hour; tm.tm_min=minute;
    return timegm(&tm);
}
int main() {
    setenv("TZ","UTC",1); tzset();
    auto wall=utc(2024,2,28,23,59); std::int64_t ticks=1000;
    GhostTemporalContext context([&]{return wall;},[&]{return ticks;});
    context.observe();
    check(context.answer("quanto tempo desde minha mensagem anterior").find("Ainda nao ha")!=std::string::npos,"no invented previous message");
    check(context.answer("que dia sera amanha").find("29/02/2024")!=std::string::npos,"leap day");
    check(context.answer("que dia sera depois de amanha").find("01/03/2024")!=std::string::npos,"month boundary");
    check(context.answer("que horas serao daqui a 2 minutos").find("29/02/2024 00:01:00")!=std::string::npos,"midnight arithmetic");
    ticks+=65000; wall-=7200; context.observe();
    check(context.answer("quanto tempo passou desde minha mensagem anterior").find("0 h 1 min 5 s")!=std::string::npos,"monotonic survives wall-clock rollback");
    check(context.answer("ha quanto tempo estamos conversando").find("0 h 1 min 5 s")!=std::string::npos,"session elapsed");
    check(context.answer("voce tem nocao de tempo").find("nocao operacional")!=std::string::npos,"operational meaning explicit");
    for (auto query : {"noticias de hoje", "que horas ela volta", "qual a data de estreia do filme amanha", "explique a teoria do tempo", "quanto tempo dura um dia"})
        check(context.answer(query).empty(),"unrelated questions preserved");
    check(context.answer("daqui a 999999999999999 dias").find("0 a 3660")!=std::string::npos,"large interval bounded");
    context.clear(); context.observe();
    check(context.answer("quanto tempo desde minha mensagem anterior").find("Ainda nao ha")!=std::string::npos,"clear removes message times");
    check(context.info().find("0 h 0 min 0 s")!=std::string::npos,"clear resets elapsed time");
    wall=utc(2024,12,31,23); check(context.answer("amanha").find("01/01/2025")!=std::string::npos,"year rollover");
#ifdef _WIN32
    setenv("TZ","EST5EDT",1); tzset(); // O CRT do Windows nao entende nomes IANA
#else
    setenv("TZ","America/New_York",1); tzset();
#endif
    wall=utc(2024,3,10,5,30); // 00:30 local on a 23-hour DST day.
    check(context.answer("amanha").find("11/03/2024 (segunda-feira)")!=std::string::npos,"spring DST calendar day");
    check(context.answer("ontem").find("09/03/2024")!=std::string::npos,"previous calendar day");
    wall=utc(2024,11,3,4,30); // 00:30 local on a 25-hour DST day.
    check(context.answer("amanha").find("04/11/2024")!=std::string::npos,"fall DST calendar day");
    check(context.answer("daqui a 1 dia").find("04/11/2024")!=std::string::npos,"day arithmetic is calendar-based");
    wall=utc(2024,11,3,6,30);
    check(context.clock().find("UTC-0500")!=std::string::npos,"configured timezone offset");
    std::cout<<"Temporal context: OK (calendar, DST, elapsed time, reset, routing).\n";
}
