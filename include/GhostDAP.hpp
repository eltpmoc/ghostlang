#ifndef GHOST_DAP_HPP
#define GHOST_DAP_HPP

#include <string>

// Adaptador do Debug Adapter Protocol (VS Code e outros editores) para a GhostLang.
// Fala DAP por stdin/stdout e controla o gdb pela interface MI (gdb --interpreter=mi3).
// O programa .g é compilado com símbolos (-g e #line) por um processo filho
// ("ghost debug --saida <exe> arquivo.g"), então as linhas vistas no editor são as do .g.
namespace GhostDAP {

// ghostExe: caminho do próprio ghost (para compilar); gdb: executável do gdb a usar.
int run(const std::string& ghostExe, const std::string& gdb);

}

#endif
