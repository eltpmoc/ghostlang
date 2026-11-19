// Janela Win32 de verdade (sem GHOST_HEADLESS): cria, mostra, confere no sistema que ela
// existe, esta visivel e tem o titulo pedido, e fecha. Os testes gui_std rodam headless.
import "../../libs/gui.g"

function main() {
    int win = gui_create_window("GhostLang Janela Teste", 320, 200);
    print("criada: ", win > 0);
    print("visivel antes de mostrar: ", gui_window_visible(win));
    gui_show_window(win);
    print("visivel: ", gui_window_visible(win));
    print("titulo: ", gui_window_title(win));
    gui_close_window(win);
    print("visivel depois de fechar: ", gui_window_visible(win));
}
