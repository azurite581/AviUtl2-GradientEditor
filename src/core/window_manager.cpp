#include "window_manager.h"

bool WindowManager::createPluginWindow(const wchar_t* window_name, [[maybe_unused]] const float scale, WNDPROC wnd_proc)
{
    // 自身のウィンドウを作成
    m_wc.cbSize        = sizeof(WNDCLASSEX);
    m_wc.lpszClassName = window_name;
    m_wc.lpfnWndProc   = wnd_proc;
    m_wc.hInstance     = GetModuleHandle(nullptr);
    m_wc.hbrBackground = ::CreateSolidBrush(RGB(30, 30, 30));
    m_wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    if (!RegisterClassEx(&m_wc)) {
        return false;
    }

    // ウィンドウ作成
    m_hwnd = CreateWindowEx(
        0, window_name, window_name,
        WS_POPUP,  // 親設定前なのでPOPUPで作る
        0, 0,
        CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    return true;
}

void WindowManager::unregisterClass()
{
    ::UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
}

void WindowManager::destroyPluginWindow()
{
    ::DestroyWindow(m_hwnd);
    ::UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
}
