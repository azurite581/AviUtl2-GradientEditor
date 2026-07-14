#include "app.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <fstream>
#include <future>
#include <thread>

#include "aviutl2_sdk/config2.h"
#include "aviutl2_sdk/logger2.h"
#include "aviutl2_sdk/plugin2.h"

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
            ::SetFocus(g_app.m_window_manager.getWindowHandle());
            return MA_ACTIVATE;
        case WM_CONTEXTMENU:
            return 0;
        case WM_SIZE:
            if (wparam == SIZE_MINIMIZED) return 0;
            g_app.m_d3d_manager.setResizeWidth(static_cast<UINT>(LOWORD(lparam)));
            g_app.m_d3d_manager.setResizeHeight(static_cast<UINT>(HIWORD(lparam)));
            if (!g_app.m_window_manager.isResizing() && g_app.m_render) {
                g_app.m_render();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wparam & 0xfff0) == SC_KEYMENU) return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        case WM_ENTERSIZEMOVE:
            g_app.m_window_manager.setResizing(true);
            return 0;
        case WM_EXITSIZEMOVE:
            g_app.m_window_manager.setResizing(false);
            return 0;
    }
    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
}

//---------------------------------------------------------------------
//	GUI スレッド
//---------------------------------------------------------------------
void guiThreadMain(std::promise<HWND>&& hwnd_promise)
{
    g_app.run(std::move(hwnd_promise));
}

//---------------------------------------------------------------------
//	AviUtl2 Plugin 関連
//---------------------------------------------------------------------
COMMON_PLUGIN_TABLE common_plugin_table = {
    .name        = App::PLUGIN_NAME,
    .information = App::PLUGIN_INFO,
};

EXTERN_C __declspec(dllexport) DWORD RequiredVersion()
{
    return 2003600;
}

EXTERN_C __declspec(dllexport) void InitializeLogger(LOG_HANDLE* handle)
{
    g_app.m_log_handle = handle;
}

EXTERN_C __declspec(dllexport) void InitializeConfig(CONFIG_HANDLE* handle)
{
    g_app.m_config_handle = handle;

    // 設定ファイルの作成
    std::filesystem::path settings_file_path{g_app.m_config_handle->app_data_path};
    settings_file_path /= L"Plugin";
    settings_file_path /= App::PLUGIN_FILE_NAME;
    settings_file_path.replace_extension("ini");
    g_app.m_settings_file_path = settings_file_path;

    if (!std::filesystem::exists(settings_file_path)) {
        std::ofstream ofs(settings_file_path);
    }
}

EXTERN_C __declspec(dllexport) bool InitializePlugin(DWORD version)
{
    g_app.m_version = version;
    if (version < 2005000) {
        return false;
    }
    return true;
}

EXTERN_C __declspec(dllexport) void UninitializePlugin()
{
    g_app.cleanup();
}

EXTERN_C __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable(void)
{
    return &common_plugin_table;
}

EXTERN_C __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host)
{
    g_app.m_edit_handle     = host->create_edit_handle();
    g_app.m_host_app_hwnd = g_app.m_edit_handle->get_host_app_window();

    std::promise<HWND> p;
    auto f           = p.get_future();
    g_app.m_gui_thread = std::thread(guiThreadMain, std::move(p));
    HWND hwnd = f.get();

    host->register_window_client(g_app.m_config_handle->translate(g_app.m_config_handle, App::WINDOW_NAME), hwnd);
    host->register_project_load_handler([](PROJECT_FILE*) {
        g_app.m_project_loaded.store(true);
    });
    host->register_event_listener(EVENT_TYPE::CHANGE_FOCUS_OBJECT, &g_app, [](void* param){
        auto* app = reinterpret_cast<App*>(param);
        app->m_main_view->onChangeFocusObject();
    });
}
