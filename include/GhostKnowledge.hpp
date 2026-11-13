#pragma once
#include <cstdint>
#include <functional>
#include <string>

struct GhostKnowledgeResult {
    bool allowed = true;
    bool found = false;
    bool cached = false;
    bool saved = false;
    bool stale = false;
    std::string text;
};
// Topic selected by the GhostLang coordinator, never the conversation history.
GhostKnowledgeResult knowledge_consult(const std::string& topic, bool online = true, bool refresh = false);
std::string knowledge_info();
namespace ghost_knowledge {
struct Record {
    std::string topic, source, content;
    std::int64_t captured = 0;
};
using Fetch = std::function<std::string(const std::string&)>;
// Storage and transport separated for deterministic offline tests.
std::string key(const std::string& topic);
std::int64_t lifetime(const Record& record);
bool load(const std::string& directory, const std::string& topic, Record& record);
bool save(const std::string& directory, const Record& record, std::string& error);
GhostKnowledgeResult consult(const std::string& topic, bool online, bool refresh,
                            const std::string& directory, const Fetch& fetch);
}
