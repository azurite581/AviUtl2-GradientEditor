#include "gradient_widget.h"
#include "imgui_internal.h"

namespace CustomUI {

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

    // 指定されたラベルをキーとするグラデーションデータが存在しなかった場合、
    // 新規作成してマップに追加
    if (it == g_editor_gradients.end()) {
        auto gradient_data = std::make_unique<GradientData>();
        gradient_data->init(g_d3d_device, static_cast<int32_t>(display_size.x), static_cast<int32_t>(display_size.y));
        gradient_data->getMarkerManager()->setDefaultMarkers(data.m_marker_manager.getMarkers());
        gradient_data->getMarkerManager()->setDefaultAlphaMarkers(data.m_marker_manager.getAlphaMarkers());
        gradient_data->setBlurWidth(data.m_blur_width);
        gradient_data->setColorSpace(data.m_color_space);
        gradient_data->setInterpDir(data.m_interp_dir);
        it = g_editor_gradients.emplace(label, std::move(gradient_data)).first;
    }

    auto* gradient_data   = it->second.get();
    auto* gradient_marker = it->second.get()->getMarkerManager();

    // データを置換する場合
    if (replace_data) {
        gradient_data->getMarkerManager()->setDefaultMarkers(data.m_marker_manager.getMarkers());
        gradient_data->getMarkerManager()->setDefaultAlphaMarkers(data.m_marker_manager.getAlphaMarkers());
        gradient_data->setBlurWidth(data.m_blur_width);
        gradient_data->setColorSpace(data.m_color_space);
        gradient_data->setInterpDir(data.m_interp_dir);
    }

    ImVec2 dsize = ImVec2(ImMax(1.0f, display_size.x), ImMax(1.0f, display_size.y));
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

    float marker_half_width = gradient_marker->getMarkerWidth() * 0.5f;

    gradient_marker->setIOEnable(config.io_enable);
    gradient_marker->setMarkerWidth(static_cast<uint32_t>(config.marker_width));  // マーカーの幅をセット

    // 中間点をグラデーションの上に描画
    if (!(flags & GradientEditorFlags_NoMidpoint) && !((flags & GradientEditorFlags_MidpointBelowGradient)) && !(flags & GradientEditorFlags_AlphaMarker)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
        ImVec2 midpoint_region_size = ImVec2(dsize.x, static_cast<float>(gradient_marker->getMidpointHeight()));
        if (!(flags & GradientEditorFlags_NotAlignSideToMarker)) {
            ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(cursor.x + marker_half_width, cursor.y));
            midpoint_region_size.x -= gradient_marker->getMarkerWidth();
        }
        ImGui::InvisibleButton("midpoints", midpoint_region_size);
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();
        gradient_marker->setMidpointRegion(p0, p1);
        gradient_marker->drawMidpoints();

        ImGui::PopStyleVar();
    }

    if (flags & GradientEditorFlags_AlphaMarker) {
        ImVec2 marker_region_size = ImVec2(display_size.x, static_cast<float>(gradient_marker->getMarkerRegionHeight()));
        if (!(flags & GradientEditorFlags_NotAlignSideToMarker)) {
            ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(cursor.x + marker_half_width, cursor.y));
            marker_region_size.x -= gradient_marker->getMarkerWidth();
        }
        ImGui::InvisibleButton("alpha_markers", marker_region_size);
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();

        gradient_marker->setAlphaMarkerRegion(p0, p1);
        gradient_marker->drawAlphaMarkers();
    }

    if (!(flags & GradientEditorFlags_NoMarker)) ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));

    ImVec2 gradient_region_size = dsize;

    if (!(flags & GradientEditorFlags_NotAlignSideToMarker)) {
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(cursor.x + marker_half_width, cursor.y));
        gradient_region_size.x = ImMax(1.0f, gradient_region_size.x - gradient_marker->getMarkerWidth());
    }

    // グラデーション本体
    ImGui::Image((ImTextureID)(intptr_t)gradient_data->getOutputSrv(), gradient_region_size);
    ImVec2 p0             = ImGui::GetItemRectMin();
    ImVec2 p1             = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    gradient_marker->setGradientRegion(p0, p1);

    // ボーダー
    if (ImGui::GetStyle().FrameBorderSize == 1.0f) {
        ImVec4 border_color = ImGui::GetStyle().Colors[ImGuiCol_Border];
        draw_list->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(border_color), 0, 0, 1.0f);
    }
    if (!(flags & GradientEditorFlags_NoMarker)) ImGui::PopStyleVar();

    // マーカーの描画
    if (!(flags & GradientEditorFlags_NoMarker)) {
        ImVec2 marker_region_size = ImVec2(dsize.x, static_cast<float>(gradient_marker->getMarkerRegionHeight()));
        if (!(flags & GradientEditorFlags_NotAlignSideToMarker)) {
            ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(cursor.x + marker_half_width, cursor.y));
            marker_region_size.x = ImMax(1.0f, marker_region_size.x - gradient_marker->getMarkerWidth());
        }

        ImGui::InvisibleButton("markers", marker_region_size);
        p0 = ImGui::GetItemRectMin();
        p1 = ImGui::GetItemRectMax();

        gradient_marker->setMarkerRegion(p0, p1);
        gradient_marker->drawMarkers();

        // 中間点をグラデーションの下に描画する場合はここで描画
        // マーカーと中間点が重なっていた場合に中間点を優先するため、マーカーの更新は中間点の更新より後に行う
        if ((flags & GradientEditorFlags_AlphaMarker) || (!(flags & GradientEditorFlags_NoMidpoint) && (flags & GradientEditorFlags_MidpointBelowGradient))) {
            gradient_marker->setMidpointRegion(p0, p1);
            gradient_marker->drawMidpoints();
        }
    }

    ImVec2 mouse_pos = ImGui::GetIO().MousePos;

    // マーカーをダブルクリックしたときにカラーピッカーを表示する
    gradient_marker->onDoubleClickedMarker(mouse_pos);
    gradient_marker->onDoubleClickedAlphaMarker(mouse_pos);

    // 新しく挿入されるマーカーの色
    ImVec4 new_marker_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (flags & GradientEditorFlags_newMarkerColorFromClick) {
        // 動的な表示サイズの変更に対応するため、表示サイズではなくテクスチャサイズ上での位置を取得する
        // テクスチャサイズは最初に与えられた表示サイズになる。以降はそのテクスチャを拡縮して表示する
        ImVec2 mouse_pos_on_texture = [&mouse_pos, &gradient_data, &gradient_marker, &dsize]() {
            float t = gradient_data->getTextureWidth() / dsize.x;
            ImVec2 mouse_pos_on_texture;
            mouse_pos_on_texture.x = (mouse_pos.x - gradient_marker->getGradientRegionP0().x) * t;
            mouse_pos_on_texture.y = (mouse_pos.y - gradient_marker->getGradientRegionP0().y) * t;
            return mouse_pos_on_texture;
        }();
        std::vector<float> texture_color = gradient_data->getTextureColor(g_d3d_device, g_d3d_device_context, static_cast<int32_t>(mouse_pos_on_texture.x), static_cast<int32_t>(mouse_pos_on_texture.y));
        new_marker_color                 = ImVec4(texture_color[0], texture_color[1], texture_color[2], texture_color[3]);
    } else {
        new_marker_color = gradient_marker->getSelectedMarkerColor();
    }
    ImGui::Dummy({0, 0});

    // 中間点をクリックしているかに基づいてマーカーの更新を行うかどうかを判断するため、マーカーの更新は中間点の更新より後に行う必要がある
    gradient_marker->updateMidpoint(mouse_pos);
    gradient_marker->updateMarker(mouse_pos, new_marker_color, config.max_marker_count);
    gradient_marker->updateAlphaMarker(mouse_pos, 1.0f, config.max_marker_count);

    ImGui::PushID(gradient_marker);
    gradient_marker->showColorPickerPopup();
    gradient_marker->showAlphaSliderPopup();
    ImGui::PopID();

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
        gradient_data->getMarkerManager()->setDefaultMarkers(data.m_marker_manager.getMarkers());
        gradient_data->getMarkerManager()->setDefaultAlphaMarkers(data.m_marker_manager.getAlphaMarkers());
        gradient_data->setBlurWidth(data.m_blur_width);
        gradient_data->setColorSpace(data.m_color_space);
        gradient_data->setInterpDir(data.m_interp_dir);
        it = gradient_datas.emplace(label, std::move(gradient_data)).first;
    } else {
        // 存在する場合はデータを上書き
        gradient_datas[label].get()->getMarkerManager()->setDefaultMarkers(data.m_marker_manager.getMarkers());
        gradient_datas[label].get()->getMarkerManager()->setDefaultAlphaMarkers(data.m_marker_manager.getAlphaMarkers());
        gradient_datas[label].get()->setBlurWidth(data.m_blur_width);
        gradient_datas[label].get()->setColorSpace(data.m_color_space);
        gradient_datas[label].get()->setInterpDir(data.m_interp_dir);
    }

    auto* gradient_data = it->second.get();

    ImVec2 dsize = ImVec2(ImMax(1.0f, display_size.x), ImMax(1.0f, display_size.y));
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
    ImVec2 dsize = ImVec2(ImMax(1.0f, display_size.x), ImMax(1.0f, display_size.y));
    ImVec2 gradient_size = ImVec2(dsize.x - ImGui::GetStyle().FramePadding.x * 2.0f, dsize.y - ImGui::GetStyle().FramePadding.y * 2.0f);
    gradient_size = ImVec2(ImMax(1.0f, gradient_size.x), ImMax(1.0f, gradient_size.y));

    ID3D11ShaderResourceView* gradient_srv = getGradientSrv(g_button_gradients, label, gradient_size, data);
    return ImGui::ImageButton(label.c_str(), (ImTextureID)(intptr_t)gradient_srv, gradient_size);
}

}  // namespace CustomUI
