#ifndef APP_H
#define APP_H

#include <d3d11.h>

#include <atomic>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "aviutl2_sdk/config2.h"
#include "aviutl2_sdk/logger2.h"
#include "aviutl2_sdk/plugin2.h"
#include "d3d_manager.h"
#include "main_view.h"
#include "window_manager.h"

#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.7.1"
#endif

/// @brief アプリケーション全体の状態を管理する構造体
class App {
public:
    static constexpr const wchar_t* WINDOW_NAME      = L"Gradient Editor";
    static constexpr const wchar_t* PLUGIN_NAME      = L"Gradient Editor";
    static constexpr const wchar_t* PLUGIN_INFO      = "Gradient Editor v" WIDEN(PLUGIN_VERSION) " azurite";
    static constexpr const wchar_t* PLUGIN_FILE_NAME = L"GradientEditor";

    struct Scale {
        // ImGui::GetFrameHeight() を基準とした相対スケール
        struct Relative {
            static constexpr float ITEM_SPACING_X       = 0.3f;
            static constexpr float ITEM_SPACING_Y       = 0.12f;
            static constexpr float ITEM_INNER_SPACING_X = 0.06f;
        };

        struct Absolute {
            static constexpr float DEFAULT_FONT_SIZE       = 13.0f;
            static constexpr float ICON_FONT_GLYPHOFFSET_Y = 1.5f;
            static constexpr float FRAME_ROUNDING          = 0.0f;
            static constexpr float GRAB_MIN_SIZE           = 2.0f;
            static constexpr float FRAME_BORDER_SIZE       = 1.0f;
            static constexpr float TAB_ROUNDING            = 0.0f;
            static constexpr float DOCKING_SEPARATOR_SIZE  = 1.0f;
            static constexpr float SCROLLBAR_ROUNDING      = 0.0f;
        };
    };

    void run(std::promise<HWND>&& hwnd_promise);
    void cleanup();

    EDIT_HANDLE* m_edit_handle     = nullptr;
    LOG_HANDLE* m_log_handle       = nullptr;
    CONFIG_HANDLE* m_config_handle = nullptr;
    DWORD m_version;

    D3DManager m_d3d_manager;
    WindowManager m_window_manager;
    std::unique_ptr<MainView> m_main_view;

    // スレッド
    std::thread m_gui_thread;

    // WM_SIZE で呼ぶためのコールバック
    std::move_only_function<void()> m_render;

    std::filesystem::path m_settings_file_path;
    HWND m_host_app_hwnd = nullptr;

    std::atomic<bool> m_project_loaded = false;

    struct Settigs {
        uint32_t ui_scale    = 100;
        uint32_t preset_tab  = 0;
        uint32_t history_tab = 0;
    } m_settings;

private:
    void applyAviutl2Style();
    void renderFrame();
    void readSettings();
    void writeSettings();

    // ウィンドウの表示状態
    bool m_is_window_visible = false;
};

extern App g_app;

#endif
