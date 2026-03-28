#ifndef APP_STATE_H
#define APP_STATE_H

#include <d3d11.h>

#include <atomic>
#include <filesystem>
#include <functional>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "aviutl2_sdk/config2.h"
#include "aviutl2_sdk/logger2.h"
#include "aviutl2_sdk/plugin2.h"
#include "d3d_manager.h"
#include "window_manager.h"

/// @brief アプリケーション全体の状態を管理する構造体
struct ApplicationState {
    // AviUtl2 SDK ハンドラー
    EDIT_HANDLE* edit_handle     = nullptr;
    LOG_HANDLE* log_handle       = nullptr;
    CONFIG_HANDLE* config_handle = nullptr;
    DWORD version;  // AviUtl2 のバージョン

    // ウィンドウの表示状態
    bool is_window_visible = false;

    // マネージャー
    D3DManager d3d_manager;
    WindowManager window_manager;

    // スレッド
    std::thread gui_thread;

    // WM_SIZE で呼ぶためのコールバック
    std::move_only_function<void()> render;

    // 設定ファイルのパス
    std::filesystem::path settings_file_path;

    void cleanup()
    {
        if (gui_thread.joinable()) {
            gui_thread.join();
        }
    }
};

namespace gradient_editor {
extern ApplicationState g_app_state;
}

#endif  // APP_STATE_H
