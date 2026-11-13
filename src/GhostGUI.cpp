#include "GhostGUI.hpp"

#include <iostream>
#include <map>
#include <mutex>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace {
    std::mutex gui_mutex;
#ifdef _WIN32
    std::map<int, HWND> active_windows;
    int next_win_handle = 1;
    bool class_registered = false;

    LRESULT CALLBACK GhostWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_CLOSE:
                DestroyWindow(hwnd);
                return 0;
            case WM_DESTROY:
                return 0;
            default:
                return DefWindowProcA(hwnd, msg, wParam, lParam);
        }
    }

    void ensure_class() {
        if (!class_registered) {
            WNDCLASSA wc = {};
            wc.lpfnWndProc = GhostWndProc;
            wc.hInstance = GetModuleHandleA(NULL);
            wc.lpszClassName = "GhostWindowClass";
            wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            RegisterClassA(&wc);
            class_registered = true;
        }
    }
#endif

    bool is_headless() {
        const char* h = std::getenv("GHOST_HEADLESS");
        return h != nullptr && std::string(h) == "1";
    }
}

int ghost_gui_message_box(const std::string& title, const std::string& message, int type) {
    if (is_headless() || type == -1) {
        std::cout << "[GhostGUI] " << title << ": " << message << std::endl;
        return 1; // IDOK
    }
#ifdef _WIN32
    return MessageBoxA(NULL, message.c_str(), title.c_str(), type);
#else
    std::cout << "[" << title << "] " << message << std::endl;
    return 1;
#endif
}

int ghost_gui_open_url(const std::string& url) {
    if (is_headless()) {
        std::cout << "[GhostGUI] Open URL: " << url << std::endl;
        return 0;
    }
#ifdef _WIN32
    HINSTANCE res = ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return ((INT_PTR)res > 32) ? 0 : 1;
#else
    std::string cmd = "xdg-open \"" + url + "\" >/dev/null 2>&1";
    return std::system(cmd.c_str());
#endif
}

int ghost_gui_window_create(const std::string& title, int width, int height) {
    std::lock_guard<std::mutex> lock(gui_mutex);
    if (is_headless()) {
        std::cout << "[GhostGUI] Janela criada (headless): " << title << " (" << width << "x" << height << ")" << std::endl;
        return 1;
    }
#ifdef _WIN32
    ensure_class();
    HWND hwnd = CreateWindowExA(
        0,
        "GhostWindowClass",
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL, NULL,
        GetModuleHandleA(NULL),
        NULL
    );
    if (!hwnd) return 0;
    int h = next_win_handle++;
    active_windows[h] = hwnd;
    return h;
#else
    std::cout << "[GhostGUI] Janela criada: " << title << std::endl;
    return 1;
#endif
}

void ghost_gui_window_show(int handle) {
    std::lock_guard<std::mutex> lock(gui_mutex);
    if (is_headless()) return;
#ifdef _WIN32
    auto it = active_windows.find(handle);
    if (it != active_windows.end()) {
        ShowWindow(it->second, SW_SHOW);
        UpdateWindow(it->second);
    }
#endif
}

void ghost_gui_window_close(int handle) {
    std::lock_guard<std::mutex> lock(gui_mutex);
    if (is_headless()) return;
#ifdef _WIN32
    auto it = active_windows.find(handle);
    if (it != active_windows.end()) {
        DestroyWindow(it->second);
        active_windows.erase(it);
    }
#endif
}

bool ghost_gui_window_poll(int handle) {
    std::lock_guard<std::mutex> lock(gui_mutex);
    if (is_headless()) return false;
#ifdef _WIN32
    auto it = active_windows.find(handle);
    if (it == active_windows.end()) return false;
    MSG msg;
    while (PeekMessageA(&msg, it->second, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return IsWindow(it->second) != 0;
#else
    return false;
#endif
}
