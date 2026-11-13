#pragma once
#include <cstdint>
#include <ctime>
#include <functional>
#include <string>

// Wall clock for calendar dates; monotonic milliseconds for elapsed durations.
// Clock injection supports deterministic tests without changing the system time.
class GhostTemporalContext {
public:
    using WallClock = std::function<std::time_t()>;
    using TickClock = std::function<std::int64_t()>;
    GhostTemporalContext();
    GhostTemporalContext(WallClock wall, TickClock ticks);
    void clear();
    void observe(); // Once per accepted user message, before answering it.
    std::string clock() const;
    std::string info() const;
    // Receives Portuguese text normalized by GhostLang's intencao_dialogo().
    // Empty means this tool cannot answer; it does not infer arbitrary events.
    std::string answer(const std::string& question) const;
private:
    WallClock wall_;
    TickClock ticks_;
    std::time_t started_, last_wall_ = 0, previous_wall_ = 0;
    std::int64_t start_tick_, last_tick_ = 0, previous_tick_ = 0;
    unsigned long count_ = 0;
    std::string previous() const;
};
inline GhostTemporalContext temporal_context() { return GhostTemporalContext(); }
