#ifndef GHOST_FORMAT_CODE_HPP
#define GHOST_FORMAT_CODE_HPP

#include <string>

// Formatador oficial de código-fonte GhostLang (ghost fmt)
// Normaliza indentação (4 espaços), operadores, vírgulas, comentários e linhas em branco.
std::string ghost_format_code(const std::string& source);

#endif
