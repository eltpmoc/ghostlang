#pragma once

#include <string>

// ============================================================
// GhostGUI: Modulo nativo desktop, janelas e dialogos
// ============================================================

int ghost_gui_message_box(const std::string& title, const std::string& message, int type);
int ghost_gui_open_url(const std::string& url);
int ghost_gui_window_create(const std::string& title, int width, int height);
void ghost_gui_window_show(int handle);
void ghost_gui_window_close(int handle);
bool ghost_gui_window_poll(int handle);
// Estado real da janela no sistema (Windows): existe e esta visivel; titulo lido do Win32.
// Sem janela de verdade (headless ou fora do Windows) devolvem false / "".
bool ghost_gui_window_visible(int handle);
std::string ghost_gui_window_title(int handle);
