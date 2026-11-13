#pragma once
#include <string>
// Public topic keywords chosen by the coordinator or /buscar; no conversation history.
std::string web_search_public(const std::string& keywords);
namespace ghost_web {
std::string president(const std::string& html);
std::string encyclopedia(const std::string& xml);
std::string page_id(const std::string& xml);
bool public_keywords(const std::string& value);
bool current_french_president(const std::string& value);
bool local_clock_query(const std::string& value);
bool relevant_title(const std::string& topic, const std::string& title);
}
// Intent only; the clock comes from GhostLang's now()/format_date().
inline bool local_clock_query(const std::string& value) { return ghost_web::local_clock_query(value); }
