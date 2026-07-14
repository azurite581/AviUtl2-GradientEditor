#ifndef MAIN_VIEW_H
#define MAIN_VIEW_H

#include <atomic>

#include "config2_wrapper_interface.h"
#include "gradient_config.h"
#include "gradient_data.h"
#include "history_window.h"
#include "imgui_utils.h"
#include "logger_wrapper_interface.h"
#include "preset_window.h"
#include "script_bridge.h"


class MainView {
public:
    struct WindowVisible {
        bool preset_window  = false;
        bool history_window = false;
    };

    MainView(LoggerWrapperInterface* logger_wrapper, ConfigWrapperInterface* config_wrapper);
    void render();
    void writeHistories();
    void setWindowVisible(const WindowVisible& visible) noexcept { m_window_visible = visible; }
    WindowVisible getWindowVisible() const noexcept { return m_window_visible; }
    void setPluginInfo(const std::string& name, const std::string& version, const std::string& author)
    {
        m_plugin_name = name;
        m_plugin_version = version;
        m_plugin_author = author;
    }

    void onChangeFocusObject();
    void updateObjectInfo() { m_force_update_obj_info.store(true); }
    void resetApplyState() { m_force_apply_state_off.store(imgui_utils::ForceState::ForceOff); }
    void resetEffectIndex() { m_force_reset_effect_index.store(true); }

private:
#if defined(MARKER_COUNT)
    static constexpr uint32_t MAX_MARKER_COUNT = MARKER_COUNT;
#else
    static inline constexpr uint32_t MAX_MARKER_COUNT = 30;
#endif
    static constexpr const char* COLOR_SPACE_NAMES[]   = {"sRGB", "Linear sRGB", "HSV", "HSL", "L*a*b", "LCh", "Oklab", "Oklch", "Kubelka-Munk"};
    static constexpr const char* INTERP_DIR_NAMES[]    = {"短経路", "長経路"};
    static constexpr const wchar_t* EFFECT_GROUP_NAME  = L"@GradientEditor";
    static constexpr const wchar_t* EFFECT_NAMES[]     = {L"MultiGradient", L"GradientMap"};
    static constexpr const wchar_t* CONFIG_FOLDER_NAME = L"GradientEditorPreset";
    static constexpr const wchar_t* PRESET_FILE_NAME   = L"gradient_editor_preset.json";
    static constexpr const wchar_t* HISTORY_FILE_NAME  = L"gradient_editor_history.json";
    static constexpr const char* COLOR_PICKER_POPUP_ID = "color_picker_popup";

    struct Scale {
        // ImGui::GetFrameHeight() を基準とした相対スケール
        struct Relative {
            static constexpr float GRADIENT_HEIGHT          = 1.5f;
            static constexpr float GRADIENT_MARGIN_Y        = 0.25f;
            static constexpr float GRADIENT_MARKER_WIDTH    = 0.5f;
            static constexpr float GRADIENT_MARKER_HEIGHT   = 0.5f;
            static constexpr float GRADIENT_MIDPOINT_WIDTH  = 0.5f;
            static constexpr float GRADIENT_MIDPOINT_HEIGHT = 0.5f;
            static constexpr float ITEM_NAME_BUTTON_WIDTH   = 5.0f;
            static constexpr float EFFECT_INDEX_SPIN_WIDTH  = 4.0f;
        };

        struct Absolute {
            static constexpr float PRESET_WINDOW_RATIO = 0.35f;
        };
    };

    bool colorPickerPopup(const char* label, ImVec4& current_color, ImVec4& previous_color, const PALETTE_INFO& palette_info);
    void showAboutPopup(const char* name, bool* p_open, ImGuiWindowFlags flags = 0);
    void showUISettingsPopup(const char* name, bool* p_open, ImGuiWindowFlags flags = 0);
    void renderGradientEditor();
    void renderColorPropertyEditor(GradientData* data);
    void renderAlphaPropertyEditor(GradientData* data);

    LoggerWrapperInterface* m_logger_wrapper;
    ConfigWrapperInterface* m_config_wrapper;

    std::string m_plugin_name{}, m_plugin_version{}, m_plugin_author{};

    GradientData* m_data = nullptr;
    GradientData m_replacement_gradient_data;
    ScriptBridge m_script_bridge;
    GradientConfigManager m_config_manager;
    Preset m_preset_config;
    History m_history_config;

    PresetWindow m_preset_window;
    HistoryWindow m_history_window;
    WindowVisible m_window_visible;

    uint32_t m_effect_name_index     = 0;
    int32_t m_effect_index           = 0;
    int32_t m_target_move_index      = 0;
    int32_t m_frame_count            = 2;
    OBJECT_LAYER_FRAME m_layer_frame = {0, 0, 0};
    PALETTE_INFO m_palette_info{};

    bool m_apply                                                 = false;
    bool m_load                                                  = false;
    bool m_is_initialized                                        = false;
    bool m_redraw                                                = false;
    bool m_object_created                                        = false;
    std::atomic<bool> m_force_update_obj_info                    = false;
    std::atomic<bool> m_force_reset_effect_index                 = false;
    std::atomic<imgui_utils::ForceState> m_force_apply_state_off = imgui_utils::ForceState::None;

    ImU32 m_object_video_color_start = 0;
    ImU32 m_object_video_color_stop  = 0;
    ImU32 m_frame_cursor_color       = 0;

    ImVec4 m_popup_current_color{1.0f, 1.0f, 1.0f, 1.0f};
    ImVec4 m_popup_previous_color{1.0f, 1.0f, 1.0f, 1.0f};

    static constexpr int32_t NEW_OBJECT_LENGTH                     = 81;
    static constexpr const char* NEW_MULTI_GRADIENT_ALIAS_TEMPLATE = R"(
[Object]
[Object.0]
effect.name=図形
図形の種類=背景
サイズ=100
縦横比=0.00
ライン幅=4000
色=ffffff
角を丸くする=0
[Object.1]
effect.name=標準描画
X=0.00
Y=0.00
Z=0.00
Group=1
中心X=0.00
中心Y=0.00
中心Z=0.00
X軸回転=0.00
Y軸回転=0.00
Z軸回転=0.00
Group2=1
拡大率=100.000
縦横比=0.000
透明度=0.00
合成モード=通常
[Object.2]
effect.name=MultiGradient@GradientEditor
強さ=100.00
中心X=0.00
中心Y=0.00
Group=1
角度=90.0
幅=100
背景透明度=0.00
形状=線形
シフト=0.00
境界モード=境界色
合成モード=通常
幅をオブジェクトに合わせる=1
グラデーションデータ.hide=1
色=0x000000,0xffffff
色の透明度=0.00,0.00
位置=0.00,1.00
中間点=0.50
マーカー数=2
ぼかし幅=100
色空間=sRGB
補間経路=短経路
アルファ値=1.00,1.00
アルファ位置=0.00,1.00
アルファ中間点=0.50
アルファマーカー数=2
アルファぼかし幅=100
)";

    static constexpr const char* NEW_GRADIENT_MAP_ALIAS_TEMPLATE = R"(
[Object]
[Object.0]
effect.name=画像ファイル
ファイル=
表示番号=0,0,再生範囲,0
再生速度=100.00
ループ再生=0
連番ファイル=0
[Object.1]
effect.name=標準描画
X=0.00
Y=0.00
Z=0.00
Group=1
中心X=0.00
中心Y=0.00
中心Z=0.00
X軸回転=0.00
Y軸回転=0.00
Z軸回転=0.00
Group2=1
拡大率=100.000
縦横比=0.000
透明度=0.00
合成モード=通常
[Object.2]
effect.name=GradientMap@GradientEditor
強さ=100.00
背景透明度=0.00
ルーマ=Rec. 601
シフト=0.00
境界モード=境界色
合成モード=通常
グラデーションデータ.hide=1
色=0x000000,0xffffff
色の透明度=0.00,0.00
位置=0.00,1.00
中間点=0.50
マーカー数=2
ぼかし幅=100
色空間=sRGB
補間経路=短経路
アルファ値=1.00,1.00
アルファ位置=0.00,1.00
アルファ中間点=0.50
アルファマーカー数=2
アルファぼかし幅=100
)";

    static constexpr const char* NEW_OBJECT_ALIAS_TAMPLATES[2] = {NEW_MULTI_GRADIENT_ALIAS_TEMPLATE, NEW_GRADIENT_MAP_ALIAS_TEMPLATE};

    static constexpr const char* MUTLI_GRADIENT_ALIAS_TEMPLATE = R"(
[Object.{}]
effect.name=MultiGradient@GradientEditor
強さ=100.00
中心X=0.00
中心Y=0.00
Group=1
角度=90.0
幅=100
背景透明度=0.00
形状=線形
シフト=0.00
境界モード=境界色
合成モード=通常
幅をオブジェクトに合わせる=1
グラデーションデータ.hide=1
色=0x000000,0xffffff
色の透明度=0.00,0.00
位置=0.00,1.00
中間点=0.50
マーカー数=2
ぼかし幅=100
色空間=sRGB
補間経路=短経路
アルファ値=1.00,1.00
アルファ位置=0.00,1.00
アルファ中間点=0.50
アルファマーカー数=2
アルファぼかし幅=100
)";

    static constexpr const char* GRADIENT_MAP_ALIAS_TEMPLATE = R"(
[Object.{}]
effect.name=GradientMap@GradientEditor
強さ=100.00
背景透明度=0.00
ルーマ=Rec. 601
シフト=0.00
境界モード=境界色
合成モード=通常
グラデーションデータ.hide=1
色=0x000000,0xffffff
色の透明度=0.00,0.00
位置=0.00,1.00
中間点=0.50
マーカー数=2
ぼかし幅=100
色空間=sRGB
補間経路=短経路
アルファ値=1.00,1.00
アルファ位置=0.00,1.00
アルファ中間点=0.50
アルファマーカー数=2
アルファぼかし幅=100
)";

    static constexpr const char* SCRIPT_TAMPLATES[2] = {MUTLI_GRADIENT_ALIAS_TEMPLATE, GRADIENT_MAP_ALIAS_TEMPLATE};
};

#endif  // MAIN_VIEW_H
