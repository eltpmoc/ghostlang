// ============================================================
// Biblioteca Padrao: GhostConsole v1.0
// Output de Terminal e formatacao com cores ANSI
// ============================================================

function void console_log(string message) {
    print(message);
}

function void console_error(string message) {
    // Red ANSI code
    print("\033[31m" + message + "\033[0m");
}

function void console_warn(string message) {
    // Yellow ANSI code
    print("\033[33m" + message + "\033[0m");
}

function void console_success(string message) {
    // Green ANSI code
    print("\033[32m" + message + "\033[0m");
}

function void console_info(string message) {
    // Cyan ANSI code
    print("\033[36m" + message + "\033[0m");
}
