#ifndef MAIN_VIEW_H
#define MAIN_VIEW_H

#include "app_state.h"
#include "config2_wrapper_interface.h"
#include "gradient_preset.h"
#include "logger_wrapper_interface.h"
#include "preset_controller.h"
#include "preset_window.h"
#include "script_bridge.h"

namespace gradient_editor {

class MainView {
public:
    MainView(LoggerWrapperInterface* logger_wrapper, ConfigWrapperInterface* config_wrapper);
    void render();

private:
    LoggerWrapperInterface* m_logger_wrapper;
    ConfigWrapperInterface* m_config_wrapper;

    void renderGradientEditor();
    void renderPropertyEditor(GradientData* data);

    ScriptBridge m_script_bridge;
    PresetManager m_preset_manager;
    preset_file::GradientPresetFile m_preset_file;
    PresetWindow m_preset_window;
    struct WindowVisible {
        bool preset_window = true;
    };
    WindowVisible m_window_visible;

    // UI State
    uint32_t m_effect_name_index     = 0;
    int32_t m_effect_index           = 0;
    int32_t m_target_move_index      = 0;
    int32_t m_frame_count            = 2;
    OBJECT_LAYER_FRAME m_layer_frame = {0, 0, 0};

    bool m_apply   = false;
    bool m_load    = false;
    bool m_is_init = false;

    // Colors for AviUtl2 objects
    ImU32 m_object_video_color_start = 0;
    ImU32 m_object_video_color_stop  = 0;
    ImU32 m_frame_cursor_color       = 0;
};

}  // namespace gradient_editor

#endif  // MAIN_VIEW_H
