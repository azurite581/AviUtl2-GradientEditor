#ifndef GRADIENT_WIDGET_H
#define GRADIENT_WIDGET_H

#include <wrl/client.h>

#include <cfloat>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "gradient_data.h"
#include "gradient_renderer.h"

namespace custom_ui {

enum GradientEditorFlags_ {
    GradientEditorFlags_None                    = 0,       // フラグなし
    GradientEditorFlags_NoMidpoint              = 1 << 1,  // 中間点非表示
    GradientEditorFlags_NoMarker                = 1 << 2,  // マーカー非表示
    GradientEditorFlags_newMarkerColorFromClick = 1 << 3,  // 新規マーカーの色をクリック位置の色にする
    GradientEditorFlags_AlphaMarker             = 1 << 4,
};
using GradientEditorFlags = int32_t;

void initDX11(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);

void cleanup();

ID3D11ShaderResourceView* getGradientSrv(
    std::unordered_map<std::string, std::unique_ptr<GradientData>>& gradient_datas,
    const std::string label,
    const ImVec2& display_size,
    const GradientData& data);

bool drawGradientButton(
    const std::string label,
    const ImVec2& display_size,
    const GradientData& data);

// 描画前にユーザーが設定できるオプション
struct GradientEditorConfig {
    uint32_t max_marker_count = 30;  // 最大マーカー数。最大マーカー数を超えると新規マーカー追加不可
    ImVec2 marker_size{20.0f, 20.0f};
    ImVec2 midpoint_size{20.0f, 20.0f};
    bool io_enable = true;  // マウス入力を受け付けるかどうか
};

/// @brief グラデーションを描画する関数
/// @param label グラデーションの一意なラベル
/// @param display_size 表示サイズ
/// @param flags フラグ
/// @param data グラデーションデータ
/// @param replace_data グラデーションデータを data で置き換えるかどうか
/// @param config グラデーションエディターの設定
/// @return グラデーションエディターのハンドル
GradientData* drawGradientEditor(
    const std::string label,
    const ImVec2& display_size,
    const GradientData& data,
    bool redraw                 = false,
    bool replace_data           = false,
    GradientEditorFlags flags   = GradientEditorFlags_None,
    GradientEditorConfig config = GradientEditorConfig()
);

}  // namespace custom_ui

#endif  // !GRADIENT_WIDGET_H
