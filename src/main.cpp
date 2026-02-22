#include <future>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "aviutl2_sdk.h"
#include "core/app.h"
#include "core/app_state.h"
#include "core/constants.h"
#include "utils/aviutl2/logger_wrapper.h"

using namespace gradient_editor;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace gradient_editor {
//---------------------------------------------------------------------
//	ウィンドウプロシージャ
//---------------------------------------------------------------------
LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;
    switch (msg) {
        case WM_MOUSEACTIVATE:
            ::SetFocus(g_app_state.window_manager.getWindowHandle());
            return MA_ACTIVATE;
        case WM_CONTEXTMENU:
            // メニューバーがホバーされている時だけ右クリックメニューを有効にする
            if (MenuBar::isMenuBarHovered()) break;
            else return 0;
        case WM_SIZE:
            if (wparam == SIZE_MINIMIZED) return 0;
            g_app_state.d3d_manager.setResizeWidth(static_cast<UINT>(LOWORD(lparam)));
            g_app_state.d3d_manager.setResizeHeight(static_cast<UINT>(HIWORD(lparam)));
            if (!g_app_state.window_manager.isResizing() && g_app_state.render) {
                g_app_state.render();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wparam & 0xfff0) == SC_KEYMENU) return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        case WM_ENTERSIZEMOVE:
            g_app_state.window_manager.setResizing(true);
            return 0;
        case WM_EXITSIZEMOVE:
            g_app_state.window_manager.setResizing(false);
            return 0;
            break;
    }
    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
}

//---------------------------------------------------------------------
//	GUI スレッド
//---------------------------------------------------------------------
void guiThreadMain(std::promise<HWND>&& hwnd_promise)
{
    App app;
    app.run(std::move(hwnd_promise));
}

} // namespace gradient_editor

//---------------------------------------------------------------------
//	AviUtl2 Plugin 関連
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) DWORD RequiredVersion() {
	return 2003200;
}

EXTERN_C __declspec(dllexport) void InitializeLogger(LOG_HANDLE* handle)
{
    g_app_state.logger = handle;
}

EXTERN_C __declspec(dllexport) void InitializeConfig(CONFIG_HANDLE* handle)
{
    g_app_state.config = handle;
}

EXTERN_C __declspec(dllexport) bool InitializePlugin(DWORD version)
{
    if (version < 2003200) {
        return false;
    }
    return true;
}

EXTERN_C __declspec(dllexport) void UninitializePlugin()
{
    // WM_QUIT を App::run() 内のメッセージループに通知
    HWND hwnd = g_app_state.window_manager.getWindowHandle();
    if (hwnd) {
        ::PostMessage(hwnd, WM_QUIT, 0, 0);
    }
    g_app_state.cleanup();
}

EXTERN_C __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host)
{
    host->set_plugin_information(PLUGIN_INFORMATION);
    g_app_state.edit_handle = host->create_edit_handle();

    std::promise<HWND> p;
    auto f = p.get_future();
    g_app_state.gui_thread = std::thread(guiThreadMain, std::move(p));

    HWND hwnd = f.get();
    host->register_window_client(g_app_state.config->translate(g_app_state.config, WINDOW_NAME_DEFAULT), hwnd);
}
