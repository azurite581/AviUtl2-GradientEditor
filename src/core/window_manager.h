#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

class WindowManager {
public:
    WindowManager() = default;

    // コピー・ムーブ禁止
    WindowManager(const WindowManager&)            = delete;
    WindowManager& operator=(const WindowManager&) = delete;
    WindowManager(WindowManager&&)                 = delete;
    WindowManager& operator=(WindowManager&&)      = delete;

    bool createPluginWindow(const wchar_t* window_name, const float scale, WNDPROC wnd_proc);
    void unregisterClass();
    void destroyPluginWindow();
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    HWND getWindowHandle() const { return m_hwnd; }

    void setResizing(const bool state) noexcept { m_is_resizing = state; }
    [[nodiscard]] bool isResizing() const noexcept { return m_is_resizing; }

private:
    HWND m_hwnd;
    WNDCLASSEXW m_wc   = {};
    bool m_is_resizing = false;
};

#endif WINDOW_MANAGER_H
