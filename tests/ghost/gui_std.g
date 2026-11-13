import "libs/gui.g";

function int main() {
    int r = ghost_gui_message_box("GhostDesktop", "Janela pronta", -1);
    int win = gui_create_window("Minha Janela", 800, 600);
    gui_show_window(win);
    gui_close_window(win);
    print("gui_compilado: OK");
    return 0;
}
