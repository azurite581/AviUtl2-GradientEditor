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
    void renderColorPropertyEditor(GradientData* data);
    void renderAlphaPropertyEditor(GradientData* data);

    LoggerWrapperInterface* m_logger_wrapper;
    ConfigWrapperInterface* m_config_wrapper;

    GradientData* m_data = nullptr;
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

    bool m_apply   = false;
    bool m_load    = false;
    bool m_is_init = false;

    ImU32 m_object_video_color_start = 0;
    ImU32 m_object_video_color_stop  = 0;
    ImU32 m_frame_cursor_color       = 0;

    static constexpr int32_t NEW_OBJECT_LENGTH = 81;
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
中心X=0.0
中心Y=0.0
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
アルファ値=1.00,1.00
アルファ位置=0.00,1.00
アルファマーカー数=2
ぼかし幅=100
色空間=sRGB
補間経路=短経路
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
アルファ値=1.00,1.00
アルファ位置=0.00,1.00
アルファマーカー数=2
ぼかし幅=100
色空間=sRGB
補間経路=短経路
)";

    static constexpr const char* NEW_OBJECT_ALIAS_TAMPLATES[2] = {NEW_MULTI_GRADIENT_ALIAS_TEMPLATE, NEW_GRADIENT_MAP_ALIAS_TEMPLATE};

    static constexpr const char* MUTLI_GRADIENT_ALIAS_TEMPLATE = R"(
[Object.{}]
effect.name=MultiGradient@GradientEditor
強さ=100.00
中心X=0.0
中心Y=0.0
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
アルファ値=1.00,1.00
アルファ位置=0.00,1.00
アルファマーカー数=2
ぼかし幅=100
色空間=sRGB
補間経路=短経路
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
アルファ値=1.00,1.00
アルファ位置=0.00,1.00
アルファマーカー数=2
ぼかし幅=100
色空間=sRGB
補間経路=短経路
)";

    static constexpr const char* SCRIPT_TAMPLATES[2] = {MUTLI_GRADIENT_ALIAS_TEMPLATE, GRADIENT_MAP_ALIAS_TEMPLATE};

};

#endif  // MAIN_VIEW_H
