#include "app.h"
#include "app_state.h"
#include "constants.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "aviutl2_sdk/config2.h"
#include "aviutl2_sdk/logger2.h"
#include "aviutl2_sdk/plugin2.h"

#include <fstream>
#include <future>
#include <thread>

//---------------------------------------------------------------------
//	プラグイン情報用マクロ
//---------------------------------------------------------------------
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)
#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)

#define PLUGIN_NAME "Gradient Editor"
#define PLUGIN_FILE_NAME "GradientEditor"
#define PLUGIN_AUTHOR "azurite"
#ifndef PLUGIN_VERSION_CORE
#define PLUGIN_VERSION_CORE 0.4.1
#endif

#define PLUGIN_VERSION_STR L"v" WIDEN(STRINGIFY(PLUGIN_VERSION_CORE))
#define PLUGIN_INFO    \
    WIDEN(PLUGIN_NAME) \
    L" " PLUGIN_VERSION_STR L" " WIDEN(PLUGIN_AUTHOR)

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

//---------------------------------------------------------------------
//	ウィンドウプロシージャ
//---------------------------------------------------------------------
LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;
    switch (msg) {
        case WM_MOUSEACTIVATE:
            ::SetFocus(gradient_editor::g_app_state.window_manager.getWindowHandle());
            return MA_ACTIVATE;
        case WM_CONTEXTMENU:
            return 0;
        case WM_SIZE:
            if (wparam == SIZE_MINIMIZED) return 0;
            gradient_editor::g_app_state.d3d_manager.setResizeWidth(static_cast<UINT>(LOWORD(lparam)));
            gradient_editor::g_app_state.d3d_manager.setResizeHeight(static_cast<UINT>(HIWORD(lparam)));
            if (!gradient_editor::g_app_state.window_manager.isResizing() && gradient_editor::g_app_state.render) {
                gradient_editor::g_app_state.render();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wparam & 0xfff0) == SC_KEYMENU) return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        case WM_ENTERSIZEMOVE:
            gradient_editor::g_app_state.window_manager.setResizing(true);
            return 0;
        case WM_EXITSIZEMOVE:
            gradient_editor::g_app_state.window_manager.setResizing(false);
            return 0;
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

//---------------------------------------------------------------------
//	AviUtl2 Plugin 関連
//---------------------------------------------------------------------
COMMON_PLUGIN_TABLE common_plugin_table = {
    WIDEN(PLUGIN_NAME),
    PLUGIN_INFO,
};

EXTERN_C __declspec(dllexport) DWORD RequiredVersion()
{
    return 2003600;
}

EXTERN_C __declspec(dllexport) void InitializeLogger(LOG_HANDLE* handle)
{
    gradient_editor::g_app_state.log_handle = handle;
}

EXTERN_C __declspec(dllexport) void InitializeConfig(CONFIG_HANDLE* handle)
{
    gradient_editor::g_app_state.config_handle = handle;

    // 設定ファイルの作成
    std::filesystem::path settings_file_path{gradient_editor::g_app_state.config_handle->app_data_path};
    settings_file_path /= L"Plugin";
    settings_file_path /= PLUGIN_FILE_NAME;
    settings_file_path.replace_extension("ini");
    gradient_editor::g_app_state.settings_file_path = settings_file_path;

    if (!std::filesystem::exists(settings_file_path)) {
        std::ofstream ofs(settings_file_path);
    }
}

EXTERN_C __declspec(dllexport) bool InitializePlugin(DWORD version)
{
    gradient_editor::g_app_state.version = version;
    if (version < 2003600) {
        return false;
    }
    return true;
}

EXTERN_C __declspec(dllexport) void UninitializePlugin()
{
    // WM_QUIT を App::run() 内のメッセージループに通知
    HWND hwnd = gradient_editor::g_app_state.window_manager.getWindowHandle();
    if (hwnd) {
        ::PostMessage(hwnd, WM_QUIT, 0, 0);
    }
    gradient_editor::g_app_state.cleanup();
}

EXTERN_C __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable(void)
{
    return &common_plugin_table;
}

EXTERN_C __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host)
{
    if (gradient_editor::g_app_state.version < 2003500) {
        host->set_plugin_information(PLUGIN_INFO);
    }
    gradient_editor::g_app_state.edit_handle = host->create_edit_handle();

    std::promise<HWND> p;
    auto f                = p.get_future();
    gradient_editor::g_app_state.gui_thread = std::thread(guiThreadMain, std::move(p));

    HWND hwnd = f.get();
    host->register_window_client(gradient_editor::g_app_state.config_handle->translate(gradient_editor::g_app_state.config_handle, WINDOW_NAME_DEFAULT), hwnd);
}
