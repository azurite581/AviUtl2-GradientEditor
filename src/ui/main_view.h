#ifndef MAIN_VIEW_H
#define MAIN_VIEW_H

#include "app_state.h"
#include "config2_wrapper_interface.h"
#include "gradient_config.h"
#include "history_window.h"
#include "logger_wrapper_interface.h"
#include "preset_window.h"
#include "script_bridge.h"

class MainView {
public:
    MainView(LoggerWrapperInterface* logger_wrapper, ConfigWrapperInterface* config_wrapper);
    void render();
    void writeHistories();

private:
    struct WindowVisible {
        bool preset_window  = true;
        bool history_window = true;
    };

    void renderGradientEditor();
    void renderPropertyEditor(GradientData* data);

    LoggerWrapperInterface* m_logger_wrapper;
    ConfigWrapperInterface* m_config_wrapper;

    GradientData* m_data = nullptr;
    ScriptBridge m_script_bridge;
    GradientConfigManager m_config_manager;
    PresetConfig m_preset_config;
    HistoryConfig m_history_config;

    PresetWindow m_preset_window;
    HistoryWindow m_history_window;
    WindowVisible m_window_visible;

    uint32_t m_effect_name_index     = 0;
    int32_t m_effect_index           = 0;
    int32_t m_target_move_index      = 0;
    int32_t m_frame_count            = 2;
    OBJECT_LAYER_FRAME m_layer_frame = {0, 0, 0};

    bool m_apply   = false;
    bool m_load    = false;
    bool m_is_init = false;

    ImU32 m_object_video_color_start = 0;
    ImU32 m_object_video_color_stop  = 0;
    ImU32 m_frame_cursor_color       = 0;
};

#endif  // MAIN_VIEW_H
