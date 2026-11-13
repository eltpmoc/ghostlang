// ============================================================
// Biblioteca Padrao: GhostGUI v1.0
// Janelas, alertas nativos e navegacao desktop
// ============================================================

function int gui_alert(string title, string msg) {
    return ghost_gui_message_box(title, msg, 0);
}

function int gui_confirm(string title, string msg) {
    return ghost_gui_message_box(title, msg, 1);
}

function int gui_open_browser(string url) {
    return ghost_gui_open_url(url);
}

function int gui_create_window(string title, int width, int height) {
    return ghost_gui_window_create(title, width, height);
}

function void gui_show_window(int handle) {
    ghost_gui_window_show(handle);
}

function void gui_close_window(int handle) {
    ghost_gui_window_close(handle);
}
