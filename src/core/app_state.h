#ifndef APP_STATE_H
#define APP_STATE_H

#include <d3d11.h>

#include <atomic>
#include <functional>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "aviutl2_sdk.h"
#include "d3d_manager.h"
#include "window_manager.h"

namespace gradient_editor {

/// @brief アプリケーション全体の状態を管理する構造体
struct ApplicationState {
    // AviUtl2 SDK ハンドラー
    EDIT_HANDLE* edit_handle = nullptr;
    LOG_HANDLE* logger       = nullptr;
    CONFIG_HANDLE* config    = nullptr;
    DWORD version;  // AviUtl2 のバージョン

    // マネージャー
    D3DManager d3d_manager;
    WindowManager window_manager;

    // スレッド
    std::thread gui_thread;

    // WM_SIZE で呼ぶためのコールバック
    std::move_only_function<void()> render;

    void cleanup()
    {
        if (gui_thread.joinable()) {
            gui_thread.join();
        }
    }
};

extern ApplicationState g_app_state;

}  // namespace gradient_editor

#endif  // APP_STATE_H
