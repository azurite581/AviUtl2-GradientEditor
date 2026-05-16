#include "gradient_widget.h"

#include "imgui.h"
#include "imgui_internal.h"


namespace custom_ui {

Microsoft::WRL::ComPtr<ID3D11Device> g_d3d_device                = nullptr;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> g_d3d_device_context = nullptr;
GradientRenderer::RenderResources g_resources;

// グラデーションデータを保持するマップ
std::unordered_map<std::string, std::unique_ptr<GradientData>> g_editor_gradients;
std::unordered_map<std::string, std::unique_ptr<GradientData>> g_button_gradients;

void initDX11(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
    g_d3d_device         = device;
    g_d3d_device_context = context;
    GradientRenderer::init(g_d3d_device, g_d3d_device_context, g_resources);
}

void cleanup()
{
    g_editor_gradients.clear();
    g_button_gradients.clear();

    g_resources.cleanup();

    if (g_d3d_device) {
        g_d3d_device.Reset();
        g_d3d_device = nullptr;
    }
    if (g_d3d_device_context) {
        g_d3d_device_context.Reset();
        g_d3d_device_context = nullptr;
    }
}

GradientData* drawGradientEditor(
    const std::string label,
    const ImVec2& display_size,
    const GradientData& data,
    GradientEditorFlags flags,
    bool replace_data,
    GradientEditorConfig config)
{
    // レンダラーの初期化に失敗していたら早期終了
    if (!g_d3d_device || !g_d3d_device_context) {
        return nullptr;
    }

    auto it = g_editor_gradients.find(label);

    // 指定されたラベルをキーとするグラデーションデータが存在しなかった場合は新規作成してマップに追加
    if (it == g_editor_gradients.end()) {
        auto gradient_data = std::make_unique<GradientData>();
        gradient_data->init(g_d3d_device, static_cast<int32_t>(display_size.x), static_cast<int32_t>(display_size.y));
        gradient_data->m_color_markers.resetMarkers(gradient_data->m_default_color_markers);
        gradient_data->m_alpha_markers.resetMarkers(gradient_data->m_default_alpha_markers);
        gradient_data->setColorBlurWidth(data.m_blur_width);
        gradient_data->setAlphaBlurWidth(data.m_alpha_blur_width);
        gradient_data->setColorSpace(data.m_color_space);
        gradient_data->setInterpDir(data.m_interp_dir);
        it = g_editor_gradients.emplace(label, std::move(gradient_data)).first;
    }

    auto* gradient_data = it->second.get();
    auto* color_markers = it->second.get()->getColorMarkers();
    auto* alpha_markers = it->second.get()->getAlphaMarkers();

    // コンフィグをセット
    color_markers->setIOEnable(config.io_enable);
    color_markers->setMarkerMaxCount(config.max_marker_count);
    color_markers->setMarkerSize(config.marker_size);
    color_markers->setMidpointSize(config.midpoint_size);

    alpha_markers->setIOEnable(config.io_enable);
    alpha_markers->setMarkerMaxCount(config.max_marker_count);
    alpha_markers->setMarkerSize(config.marker_size);
    alpha_markers->setMidpointSize(config.midpoint_size);

    const float marker_half_width = config.marker_size.x * 0.5f;
    const float window_padding_x  = ImGui::GetStyle().WindowPadding.x;

    // データを置換する場合
    if (replace_data) {
        gradient_data->getColorMarkers()->resetMarkers(data.m_color_markers.getMarkers());
        gradient_data->getAlphaMarkers()->resetMarkers(data.m_alpha_markers.getMarkers());
        gradient_data->setColorBlurWidth(data.m_blur_width);
        gradient_data->setAlphaBlurWidth(data.m_alpha_blur_width);
        gradient_data->setColorSpace(data.m_color_space);
        gradient_data->setInterpDir(data.m_interp_dir);
    }

    ImVec2 dsize           = ImVec2(ImMax(1.0f, display_size.x), ImMax(1.0f, display_size.y));
    int32_t current_width  = static_cast<int32_t>(dsize.x);
    int32_t current_height = static_cast<int32_t>(dsize.y);

    // テクスチャサイズが表示サイズと異なる場合、再初期化を行う
    if (gradient_data->getTextureWidth() != current_width ||
        gradient_data->getTextureHeight() != current_height) {
        gradient_data->init(g_d3d_device, current_width, current_height);
    }

    // 表示サイズは動的に変わる可能性があるため、毎回セットする
    gradient_data->setGradientDisplayWidth(dsize.x);
    gradient_data->setGradientDisplayHeight(dsize.y);

    // ピクセルシェーダーに渡すコンスタントバッファーの値を設定
    GradientRenderer::PixelConstantBuffer buffer_values = gradient_data->gradientData2pixelConstantBuffer();
    // グラデーションをレンダリング
    GradientRenderer::runOffscreenRendering(
        g_d3d_device_context,
        g_resources,
        gradient_data->getPixelConstantBuffer(),
        &buffer_values,
        gradient_data->getTextureWidth(), gradient_data->getTextureHeight(),
        gradient_data->getRtv(), gradient_data->getSrv());

    if (!(flags & GradientEditorFlags_NoMarker)) ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));

    if (flags & GradientEditorFlags_AlphaMarker) {
        ImVec2 marker_region_size = ImVec2(display_size.x, alpha_markers->getMarkerRegionHeight());

        // 両端に位置するマーカーのはみ出しサイズを考慮
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(cursor.x + marker_half_width, cursor.y));
        marker_region_size.x -= marker_half_width + window_padding_x;

        ImGui::InvisibleButton("alpha_markers_draw_region", marker_region_size);
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();

        alpha_markers->setMarkerRegion(p0, p1);
        alpha_markers->setMarkerUpward(false);  // 下向きにする
        alpha_markers->drawMarkers();
        alpha_markers->drawMidpoints();
    }

    ImVec2 gradient_region_size = dsize;
    ImVec2 cursor               = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(cursor.x + marker_half_width, cursor.y));
    gradient_region_size.x -= marker_half_width + window_padding_x;
    gradient_region_size.x = (std::max)(1.0f, gradient_region_size.x);

    // グラデーション本体
    ImGui::Image((ImTextureID)(intptr_t)gradient_data->getOutputSrv(), gradient_region_size);
    ImVec2 p0             = ImGui::GetItemRectMin();
    ImVec2 p1             = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // グラデーションの描画領域を基準にマーカーの座標を計算するため、必須
    color_markers->setGradientRegion(p0, p1);
    alpha_markers->setGradientRegion(p0, p1);

    // ボーダー
    if (ImGui::GetStyle().FrameBorderSize == 1.0f) {
        ImVec4 border_color = ImGui::GetStyle().Colors[ImGuiCol_Border];
        draw_list->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(border_color), 0, 1.0f, 0);
    }
    if (!(flags & GradientEditorFlags_NoMarker)) ImGui::PopStyleVar();

    // マーカーの描画
    if (!(flags & GradientEditorFlags_NoMarker)) {
        ImVec2 marker_region_size = ImVec2(dsize.x, color_markers->getMarkerRegionHeight());

        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(cursor.x + marker_half_width, cursor.y));
        marker_region_size.x -= marker_half_width + window_padding_x;

        ImGui::InvisibleButton("markers_draw_region", marker_region_size);
        p0 = ImGui::GetItemRectMin();
        p1 = ImGui::GetItemRectMax();

        color_markers->setMarkerRegion(p0, p1);
        color_markers->drawMarkers();
        color_markers->drawMidpoints();
    }

    ImVec2 mouse_pos = ImGui::GetIO().MousePos;

    // 新しく挿入されるマーカーの色
    ImVec4 new_marker_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 color_marker_new_value{1.0f, 1.0f, 1.0f, 1.0f}, alpha_marker_new_value{0.0f, 0.0f, 0.0f, 1.0f};
    if (flags & GradientEditorFlags_newMarkerColorFromClick) {
        // 動的な表示サイズの変更に対応するため、表示サイズではなくテクスチャサイズ上での位置を取得する
        // テクスチャサイズは最初に与えられた表示サイズになる。以降はそのテクスチャを拡縮して表示する
        ImVec2 mouse_pos_on_texture = [&mouse_pos, &gradient_data, &p0, &dsize]() {
            float t = gradient_data->getTextureWidth() / dsize.x;
            ImVec2 mouse_pos_on_texture;
            mouse_pos_on_texture.x = (mouse_pos.x - p0.x) * t;
            mouse_pos_on_texture.y = (mouse_pos.y - p0.y) * t;
            return mouse_pos_on_texture;
        }();
        std::vector<float> texture_color = gradient_data->getTextureColor(g_d3d_device, g_d3d_device_context, static_cast<int32_t>(mouse_pos_on_texture.x), static_cast<int32_t>(mouse_pos_on_texture.y));
        new_marker_color                 = ImVec4(texture_color[0], texture_color[1], texture_color[2], texture_color[3]);
    } else {
        color_marker_new_value = color_markers->getSelectedMarkerValue();
    }
    ImGui::Dummy({0, 0});

    color_markers->setNewMarkerValue(color_marker_new_value);
    alpha_markers->setNewMarkerValue(alpha_marker_new_value);
    color_markers->updateMarkerAndMidpointPosition(mouse_pos);
    alpha_markers->updateMarkerAndMidpointPosition(mouse_pos);

    return gradient_data;
}

ID3D11ShaderResourceView* getGradientSrv(
    std::unordered_map<std::string, std::unique_ptr<GradientData>>& gradient_datas,
    const std::string label,
    const ImVec2& display_size,
    const GradientData& data)
{
    // レンダラーの初期化に失敗していたら早期終了
    if (!g_d3d_device || !g_d3d_device_context) {
        return nullptr;
    }

    auto it = gradient_datas.find(label);
    // 指定されたラベルをキーとするグラデーションデータが存在しなかった場合、
    // 新規作成してマップに追加
    if (it == gradient_datas.end()) {
        auto gradient_data = std::make_unique<GradientData>();
        gradient_data->init(g_d3d_device, static_cast<int32_t>(display_size.x), static_cast<int32_t>(display_size.y));
        gradient_data->m_color_markers.resetMarkers(gradient_data->m_default_color_markers);
        gradient_data->m_alpha_markers.resetMarkers(gradient_data->m_default_alpha_markers);
        gradient_data->setColorBlurWidth(data.m_blur_width);
        gradient_data->setAlphaBlurWidth(data.m_alpha_blur_width);
        gradient_data->setColorSpace(data.m_color_space);
        gradient_data->setInterpDir(data.m_interp_dir);
        it = gradient_datas.emplace(label, std::move(gradient_data)).first;
    } else {
        // 存在する場合はデータを上書き
        gradient_datas[label].get()->getColorMarkers()->resetMarkers(data.m_color_markers.getMarkers());
        gradient_datas[label].get()->getAlphaMarkers()->resetMarkers(data.m_alpha_markers.getMarkers());
        gradient_datas[label].get()->setColorBlurWidth(data.m_blur_width);
        gradient_datas[label].get()->setAlphaBlurWidth(data.m_alpha_blur_width);
        gradient_datas[label].get()->setColorSpace(data.m_color_space);
        gradient_datas[label].get()->setInterpDir(data.m_interp_dir);
    }

    auto* gradient_data = it->second.get();

    ImVec2 dsize           = ImVec2(ImMax(1.0f, display_size.x), ImMax(1.0f, display_size.y));
    int32_t current_width  = static_cast<int32_t>(dsize.x);
    int32_t current_height = static_cast<int32_t>(dsize.y);

    // テクスチャサイズが表示サイズと異なる場合、再初期化を行う
    if (gradient_data->getTextureWidth() != current_width ||
        gradient_data->getTextureHeight() != current_height) {
        gradient_data->init(g_d3d_device, current_width, current_height);
    }

    // 表示サイズは動的に変わる可能性があるため毎回セットし直す
    gradient_data->setGradientDisplayWidth(dsize.x);
    gradient_data->setGradientDisplayHeight(dsize.y);

    // ピクセルシェーダーに渡すコンスタントバッファーの値を設定
    GradientRenderer::PixelConstantBuffer buffer_values = gradient_data->gradientData2pixelConstantBuffer();

    // グラデーションをレンダリング
    GradientRenderer::runOffscreenRendering(
        g_d3d_device_context,
        g_resources,
        gradient_data->getPixelConstantBuffer(),
        &buffer_values,
        gradient_data->getTextureWidth(),
        gradient_data->getTextureHeight(),
        gradient_data->getRtv(),
        gradient_data->getSrv());

    return gradient_data->getOutputSrv();
}

bool drawGradientButton(const std::string label, const ImVec2& display_size, const GradientData& data)
{
    ImVec2 dsize         = ImVec2(ImMax(1.0f, display_size.x), ImMax(1.0f, display_size.y));
    ImVec2 gradient_size = ImVec2(dsize.x - ImGui::GetStyle().FramePadding.x * 2.0f, dsize.y - ImGui::GetStyle().FramePadding.y * 2.0f);
    gradient_size        = ImVec2(ImMax(1.0f, gradient_size.x), ImMax(1.0f, gradient_size.y));

    ID3D11ShaderResourceView* gradient_srv = getGradientSrv(g_button_gradients, label, gradient_size, data);
    return ImGui::ImageButton(label.c_str(), (ImTextureID)(intptr_t)gradient_srv, gradient_size);
}

}  // namespace custom_ui
