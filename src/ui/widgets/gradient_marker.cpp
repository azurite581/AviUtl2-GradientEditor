#include "gradient_marker.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "imgui.h"

void MarkerData::drawMarker(
    const char* label,
    const int64_t id,
    const ImVec2 rect_p0,
    const ImVec2 rect_p1,
    const ImVec4& rect_color,
    const bool is_upward,
    const ImVec4& inner_border_color,
    const ImVec4& outer_border_color,
    const ImVec4& arrow_color
) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 p0 = rect_p0, p1 = rect_p1;

    if (is_upward) {
        draw_list->AddTriangleFilled(
            ImVec2(p0.x + marker_size.x * 0.5f, p0.y),
            ImVec2(p1.x, p0.y + marker_arrow_size.y),
            ImVec2(p0.x, p0.y + marker_arrow_size.y),
            ImGui::ColorConvertFloat4ToU32(arrow_color));
        draw_list->AddTriangle(
            ImVec2(p0.x + marker_size.x * 0.5f, p0.y),
            ImVec2(p1.x, p0.y + marker_arrow_size.y),
            ImVec2(p0.x, p0.y + marker_arrow_size.y),
            ImGui::ColorConvertFloat4ToU32(outer_border_color));
        p0.y += marker_arrow_size.y;
    } else {
        draw_list->AddTriangleFilled(
            ImVec2(p0.x + marker_size.x * 0.5f, p1.y),
            ImVec2(p1.x, p1.y - marker_arrow_size.y),
            ImVec2(p0.x, p1.y - marker_arrow_size.y),
            ImGui::ColorConvertFloat4ToU32(arrow_color));
        draw_list->AddTriangle(
            ImVec2(p0.x + marker_size.x * 0.5f, p1.y),
            ImVec2(p1.x, p1.y - marker_arrow_size.y),
            ImVec2(p0.x, p1.y - marker_arrow_size.y),
            ImGui::ColorConvertFloat4ToU32(outer_border_color));
    }

    // カラーボタンを描画
    ImGui::PushID(id);
    {
        ImVec2 backup = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(p0);
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoDragDrop;
        ImGui::ColorButton((std::string{label} + "##marker_color").c_str(), rect_color, flags, ImVec2(marker_size.x, marker_size.y));
        ImGui::SetCursorScreenPos(backup);
    }
    ImGui::PopID();

    if (!is_upward) {
        p1.y -= marker_arrow_size.y;
    }

    // 四角形の枠を描画
    draw_list->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(inner_border_color), 0, 0, 3.0f);
    draw_list->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(outer_border_color), 0, 0, 1.0f);
}

void MarkerData::drawMidpoint(const ImVec2 rect_p0, const ImVec2 rect_p1, const ImVec4& color) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 center = ImVec2(rect_p0.x + (rect_p1.x - rect_p0.x) * 0.5f, rect_p0.y + (rect_p1.y - rect_p0.y) * 0.5f);
    draw_list->AddNgon(center, midpoint_size.x * 0.5f, ImGui::ColorConvertFloat4ToU32(color), 4, 2.0f);
}

void MarkerData::highlightMarker(
        const ImVec2 rect_p0,
        const ImVec2 rect_p1,
        const ImVec4& highlight_color,
        const bool is_upward,
        const float thickness,
        const float offset
    ) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 p0 = rect_p0, p1 = rect_p1;

    if (is_upward) {
        p0.y += marker_arrow_size.y;
    } else {
        p1.y -= marker_arrow_size.y;
    }

    p0 = ImVec2(p0.x - offset, p0.y - offset);
    p1 = ImVec2(p1.x + offset, p1.y + offset);
    draw_list->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(highlight_color), 0.0f, 0, thickness);
}

void MarkerData::highlightMidpoint(
        const ImVec2 rect_p0,
        const ImVec2 rect_p1,
        const ImVec4& highlight_color
    ) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 center = ImVec2(rect_p0.x + (rect_p1.x - rect_p0.x) * 0.5f, rect_p0.y + (rect_p1.y - rect_p0.y) * 0.5f);
    draw_list->AddNgonFilled(center, midpoint_size.x * 0.5f, ImGui::ColorConvertFloat4ToU32(highlight_color), 4);
}

//
// インデックス⇔IDの相互変換
//
// IDからインデックスを取得する
int64_t MarkerManager::getMarkerIndexById(const int64_t id) const
{
    auto it = std::find_if(m_markers.begin(), m_markers.end(),
                            [id](const MarkerData& m) { return m.id == id; });

    if (it != m_markers.end()) {
        return static_cast<int64_t>(std::distance(m_markers.begin(), it));
    }

    return -1;
}

// インデックスからIDを取得する
int64_t MarkerManager::getMarkerIdByIndex(const int64_t index) const
{
    if (index < 0 || index >= static_cast<int64_t>(std::ssize(m_markers))) {
        return -1;
    }

    return m_markers[index].id;
}


//
// マウス下のマーカー/中間点を取得
//
std::pair<MarkerManager::Clicked, int64_t> MarkerManager::getMarkerIdUnderMouse(const ImVec2& mouse_pos) const
{
    // マーカー描画領域内かどうか（両端に位置するマーカーのはみ出し部分も考慮する）
    if ((m_regions.marker_p0.x - marker_size.x * 0.5 <= mouse_pos.x && mouse_pos.x < m_regions.marker_p1.x + marker_size.x * 0.5) &&
    (m_regions.marker_p0.y <= mouse_pos.y && mouse_pos.y < m_regions.marker_p1.y)) {

        for (int32_t i = 0; const auto& marker : m_markers) {
            // 中間点を優先して判定
            if (i < static_cast<int32_t>(std::ssize(m_markers)) - 1) {
                if (mouse_pos.x >= marker.midpoint_p0.x && mouse_pos.x <= marker.midpoint_p1.x &&
                    mouse_pos.y >= marker.midpoint_p0.y && mouse_pos.y <= marker.midpoint_p1.y) {
                    return {Clicked::Midpoint, marker.id};
                }
            }
            if (mouse_pos.x >= marker.marker_p0.x && mouse_pos.x <= marker.marker_p1.x &&
                mouse_pos.y >= marker.marker_p0.y && mouse_pos.y <= marker.marker_p1.y) {
                // マーカーの上ならそのマーカーのIDを返す
                return {Clicked::Marker, marker.id};
            }
            ++i;
        }

        return {Clicked::MarkerRegion, std::to_underlying(Clicked::Marker)};
    }

    return {Clicked::OutSide, std::to_underlying(Clicked::OutSide)};
}

ImVec2 MarkerManager::getMousePosOnGradient(const ImVec2& mouse_pos) const
{
    ImVec2 mouse_pos_on_gradient;
    mouse_pos_on_gradient.x = mouse_pos.x - m_regions.gradient_p0.x;
    mouse_pos_on_gradient.y = mouse_pos.y - m_regions.gradient_p0.y;

    return mouse_pos_on_gradient;
}

//
// 操作
//
void MarkerManager::reverseMarkers()
{
    auto right_midpoint_idx = getMarkerIndexById(m_state.selected_midpoint_id) + 1;
    auto right_midpoint_id  = m_state.selected_midpoint_id;

    std::vector<float> old_midpoints(static_cast<uint32_t>(std::ssize(m_markers) - 1));
    // 位置を逆順にしつつ、逆順にする前の中間点の比率を取得
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        marker.pos = std::clamp(1.0f - marker.pos, 0.0f, 1.0f);
        if (i < static_cast<uint32_t>(std::ssize(m_markers)) - 1) {
            old_midpoints[i] = marker.midpoint.ratio;
        }
        if (i == right_midpoint_idx) {
            right_midpoint_id = marker.id;
        }
    }

    std::ranges::reverse(old_midpoints.begin(), old_midpoints.end());

    sortMarkersByPos();

    // 中間点を逆順にする
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        if (i < static_cast<uint32_t>(std::ssize(m_markers)) - 1) {
            setMidpointRatio(marker.id, std::clamp(1.0f - old_midpoints[i], 0.0f, 1.0f));
        }
    }
    m_state.selected_midpoint_id = right_midpoint_id;

    updateMidpointsPos();
}

void MarkerManager::resetMidpoints()
{
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        if (i < static_cast<uint32_t>(std::ssize(m_markers)) - 1) {
            setMidpointRatio(marker.id, 0.5f);
        }
    }
}

// マーカー位置昇順にソートするヘルパー
void MarkerManager::sortMarkersByPos()
{
    std::sort(m_markers.begin(), m_markers.end(),
            [](const MarkerData& a, const MarkerData& b) {
                return a.pos < b.pos;
            });
}

// IDを基準にマーカーを昇順ソートする
void MarkerManager::sortMarkersById()
{
    std::sort(m_markers.begin(), m_markers.end(),
                [](const MarkerData& a, const MarkerData& b) {
                    return a.id < b.id;
                });
}

void MarkerManager::moveMarker(const int64_t id, const float new_pos)
{
    setMarkerPosision(id, new_pos);
    sortMarkersByPos();
    updateMidpointsPos();
}

void MarkerManager::moveMidpoint(const int64_t id, const float new_pos)
{
    auto idx = getMarkerIndexById(id);
    if (idx < 0 || std::ssize(m_markers) - 1 <= idx) return;

    float left_pos  = m_markers[idx].pos;
    float right_pos = m_markers[idx + 1].pos;
    float range     = right_pos - left_pos;

    if (range <= 0.0001f) return;

    float ratio = (new_pos - left_pos) / range;
    m_markers[idx].midpoint.ratio = std::clamp(ratio, 0.0f, 1.0f);
    m_markers[idx].midpoint.pos = new_pos;

    updateMidpointsPos();
}

void MarkerManager::addMarker(const int64_t id, const float marker_pos, const ImVec4& value, const float midpoint_ratio)
{
    MarkerData new_marker;
    new_marker.id             = id;
    new_marker.pos            = marker_pos;
    new_marker.value          = value;
    new_marker.midpoint.ratio = midpoint_ratio;

    m_markers.push_back(new_marker);

    sortMarkersByPos();
    updateMidpointsPos();  // 中間点の位置（midpoint.pos）は、追加後に前後のマーカー位置から計算する

    OutputDebugStringA("==================\n");
    for (int32_t i = 0; const auto& m : m_markers) {
        OutputDebugStringA(std::format("i={}, pos={}, id={}\n", i, m.pos, m.id).c_str());
        ++i;
    }
}

void MarkerManager::selectNextMarker()
{
    if (std::ssize(m_markers) < 2) return;

    auto idx = getMarkerIndexById(m_state.selected_marker_id);
    if (idx == -1) return;

    ++idx;

    if (idx < std::ssize(m_markers)) {
        m_state.selected_marker_id = m_markers[idx].id;
    } else {
        m_state.selected_marker_id = m_markers.front().id;
    }
}

void MarkerManager::selectBackMarker()
{
    if (std::ssize(m_markers) < 2) return;

    auto idx = getMarkerIndexById(m_state.selected_marker_id);
    if (idx == -1) return;

    --idx;

    if (idx >= 0) {
        m_state.selected_marker_id = m_markers[idx].id;
    } else {
        m_state.selected_marker_id = m_markers.back().id;
    }
}

void MarkerManager::deleteMarker(const int64_t id)
{
    if (std::ssize(m_markers) <= 2) return;

    auto idx = getMarkerIndexById(id);
    if (idx == -1) return;

    // 削除
    m_markers.erase(m_markers.begin() + idx);

    updateMidpointsPos();
    reassignMarkerID();

    // 次に選択する ID を更新 (削除した位置にある要素、または末尾ならその前)
    if (idx < std::ssize(m_markers)) {
        m_state.selected_marker_id = m_markers[idx].id;
    } else {
        m_state.selected_marker_id = m_markers.back().id;
    }

    // 選択中の中間点 ID が不正になった場合
    if (getMarkerIndexById(m_state.selected_midpoint_id) == -1 || getMarkerIndexById(m_state.selected_midpoint_id) >= std::ssize(m_markers) - 1) {
        m_state.selected_midpoint_id = m_markers[0].id;
    }
}

void MarkerManager::deleteSelectedMarker()
{
    deleteMarker(m_state.selected_marker_id);
}

void MarkerManager::distributeMarkersEvenly()
{
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        moveMarker(marker.id, i / static_cast<float>(std::ssize(m_markers) - 1));
    }
}

void MarkerManager::distributeMarkersAndMipointsEvenly()
{
    distributeMarkersEvenly();
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        if (i < static_cast<float>(std::ssize(m_markers) - 1)) {
            moveMidpoint(marker.id, (i + 1) / static_cast<float>(std::ssize(m_markers)));
        }
    }
}

bool MarkerManager::isDoubleClickedMarker(const ImVec2& mouse_pos)
{
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        auto [clicked, id] = getMarkerIdUnderMouse(mouse_pos);
        OutputDebugStringA(std::format("selected_id={}, id={}", m_state.selected_marker_id, id).c_str());
        if (clicked == Clicked::Marker) {
            return true;
        }
    }
    return false;
}

//
// 更新
//
void MarkerManager::updateMarkerAndMidpointPosition(const ImVec2& mouse_pos)
{
    if (!m_io_enable || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) return;

    // クリックされた位置にあるマーカー/中間点のIDを取得
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        auto [clicked, id] = getMarkerIdUnderMouse(mouse_pos);
        OutputDebugStringA("==========================\n");
        OutputDebugStringA(std::format("clicked={}, id={}\n", std::to_underlying(clicked), id).c_str());
        m_state.clicked = clicked;
        if (m_state.clicked == Clicked::Marker) {
            m_state.selected_marker_id = id;
        } else if (m_state.clicked == Clicked::Midpoint) {
            m_state.selected_midpoint_id = id;
        }
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        float marker_pos = getMarkerPosFromMousePos(mouse_pos);
        if (m_state.clicked == Clicked::Marker && m_state.selected_marker_id >= 0) {
            // クリック位置にマーカーを移動
            moveMarker(m_state.selected_marker_id, marker_pos);
        } else if (m_state.clicked == Clicked::Midpoint && m_state.selected_midpoint_id >= 0) {
            // クリック位置に中間点を移動
            moveMidpoint(m_state.selected_midpoint_id, marker_pos);
        }
    } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&  // 左クリックされた
        m_state.clicked == Clicked::MarkerRegion &&   // クリックされた位置がマーカー領域
        std::ssize(m_markers) < m_marker_max_count  // 現在のマーカー数がマーカーの最大数未満
    ) {
        // クリック位置にマーカーを作成
        float marker_pos = getMarkerPosFromMousePos(mouse_pos);
        addMarker(m_state.marker_id_counter, marker_pos, m_default_value, m_default_midpoint_ratio);

        m_state.selected_marker_id = m_state.marker_id_counter;  // 追加したマーカーを選択状態にする
        m_state.clicked = Clicked::Marker;

        ++m_state.marker_id_counter;
    }

    return;
}

// 比率に基づいて中間点の絶対座標を再計算する
void MarkerManager::updateMidpointsPos()
{
    for (int32_t i = 0; i < static_cast<int32_t>(std::ssize(m_markers)) - 1; ++i) {
        float left_pos  = m_markers[i].pos;
        float right_pos = m_markers[i + 1].pos;
        float ratio     = m_markers[i].midpoint.ratio;

        // 左隣のマーカーとの距離に基づいて位置を更新
        m_markers[i].midpoint.pos = left_pos + (right_pos - left_pos) * ratio;
    }
}

// IDを再度割り当てる
void MarkerManager::reassignMarkerID()
{
    sortMarkersById();  // IDを基準に昇順ソート

    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        marker.id = static_cast<int64_t>(i);
    }
    m_state.marker_id_counter = static_cast<int64_t>(std::ssize(m_markers));

    sortMarkersByPos();  // 位置を基準に昇順ソートして元の並びに戻す
}

//
// 描画
//
std::pair<ImVec2, ImVec2> MarkerManager::calcDrawPos(const float marker_pos, const float marker_width) const
{
    ImVec2 p0 = ImVec2(m_regions.marker_p0.x + (m_regions.marker_p1.x - m_regions.marker_p0.x) * marker_pos - (marker_width * 0.5f), m_regions.marker_p0.y);
    ImVec2 p1 = ImVec2(m_regions.marker_p0.x + (m_regions.marker_p1.x - m_regions.marker_p0.x) * marker_pos + (marker_width * 0.5f), m_regions.marker_p1.y);
    return {p0, p1};
}

// 各マーカーの描画位置を計算してセットする
void MarkerManager::calcMarkersDrawPos()
{
    for (auto& marker : m_markers) {
        auto [p0, p1] = calcDrawPos(marker.pos, marker.marker_size.x);
        marker.marker_p0 = p0;
        marker.marker_p1 = p1;
    }
}

// 各中間点の描画位置を計算してセットする
void MarkerManager::calcMidpointsDrawPos()
{
    for (auto& marker : m_markers) {
        auto [p0, p1] = calcDrawPos(marker.midpoint.pos, marker.midpoint_size.x);
        marker.midpoint_p0 = p0;
        marker.midpoint_p1 = p1;
    }
}

void MarkerManager::drawMarkers()
{
    calcMarkersDrawPos();  // 描画位置を計算して marker_p0, marker_p1 に格納

    const char* label = "draw_markers";

    ImGui::PushID(this);

    int32_t selected_marker_idx = -1;
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        // 選択中のマーカーは最前面に描画するため、最後に描画する
        if (marker.id == m_state.selected_marker_id) {
            selected_marker_idx = static_cast<int32_t>(i);
            continue;
        }
        marker.drawMarker(label, marker.id, marker.marker_p0, marker.marker_p1, marker.value, m_is_upward);
    }

    if (selected_marker_idx >= 0) {
        const auto& selected_marker = m_markers.at(selected_marker_idx);
        selected_marker.drawMarker(label, selected_marker.id, selected_marker.marker_p0, selected_marker.marker_p1, selected_marker.value, m_is_upward);
        selected_marker.highlightMarker(selected_marker.marker_p0, selected_marker.marker_p1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), m_is_upward);
    }

    ImGui::PopID();
}

void MarkerManager::drawMidpoints()
{
    updateMidpointsPos();  // midpoint.ratio から midpoint.pos を計算して格納
    calcMidpointsDrawPos();  // 描画座標を計算して midpoint_p0, midopint_p1 に格納

    int32_t selected_midpoint_idx = -1;
    for (const auto& [i, marker] : m_markers | std::views::take(std::ssize(m_markers) - 1) | std::views::enumerate) {
        // 選択中のマーカーは最前面に描画するため、最後に描画する
        if (marker.id == m_state.selected_midpoint_id) {
            selected_midpoint_idx = static_cast<int32_t>(i);
            continue;
        }
        marker.drawMidpoint(marker.midpoint_p0, marker.midpoint_p1);
    }

    if (selected_midpoint_idx >= 0) {
        const auto& selected_marker = m_markers.at(selected_midpoint_idx);
        selected_marker.drawMidpoint(selected_marker.midpoint_p0, selected_marker.midpoint_p1);
        selected_marker.highlightMidpoint(selected_marker.midpoint_p0, selected_marker.midpoint_p1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
}











float GradientMarkerManager::getMarkerPos(const int32_t id) const
{
    int32_t idx = getIndexById(id);
    if (idx == -1) return 0.0f;
    return m_markers[idx].pos;
}

ImVec4 GradientMarkerManager::getMarkerColor(const int32_t id) const
{
    int32_t idx = getIndexById(id);
    if (idx == -1) return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    return m_markers[idx].color;
}

float GradientMarkerManager::getMidpointRatio(const int32_t id) const
{
    int32_t idx = getIndexById(id);
    if (idx == -1) return 0.5f;
    return m_markers[idx].midpoint.ratio;
}

float GradientMarkerManager::getAlphaMarkerValue(const int32_t id) const
{
    int32_t idx = getAlphaIndexById(id);
    if (idx == -1) return 1.0f;
    return m_alpha_markers[idx].value;
}

float GradientMarkerManager::getSelectedMarkerPos() const
{
    int32_t idx = getIndexById(m_state.selected_marker_id);
    return m_markers[idx].pos;
}

ImVec4 GradientMarkerManager::getSelectedMarkerColor() const
{
    int32_t idx = getIndexById(m_state.selected_marker_id);
    return m_markers[idx].color;
}

float GradientMarkerManager::getSelectedAlphaMarkerPos() const
{
    int32_t idx = getAlphaIndexById(m_state.selected_alpha_marker_id);
    return m_alpha_markers[idx].pos;
}

float GradientMarkerManager::getSelectedAlphaMarkerValue() const
{
    int32_t idx = getAlphaIndexById(m_state.selected_alpha_marker_id);
    return m_alpha_markers[idx].value;
}

float GradientMarkerManager::getSelectedMidpointRatio() const
{
    int32_t idx = getIndexById(m_state.selected_midpoint_id);
    return m_markers[idx].midpoint.ratio;
}

float GradientMarkerManager::getSelectedAlphaMidpointRatio() const
{
    int32_t idx = getAlphaIndexById(m_state.selected_alpha_midpoint_id);
    return m_alpha_markers[idx].midpoint.ratio;
}

// IDからインデックスを取得する
int32_t GradientMarkerManager::getIndexById(const int32_t id) const
{
    auto it = std::find_if(m_markers.begin(), m_markers.end(),
                           [id](const GradientMarkerData& m) { return m.id == id; });

    if (it != m_markers.end()) {
        return static_cast<int>(std::distance(m_markers.begin(), it));
    }
    return -1;
}

int32_t GradientMarkerManager::getAlphaIndexById(const int32_t id) const
{
    auto it = std::find_if(m_alpha_markers.begin(), m_alpha_markers.end(),
                           [id](const AlphaMarkerData& m) { return m.id == id; });

    if (it != m_alpha_markers.end()) {
        return static_cast<int>(std::distance(m_alpha_markers.begin(), it));
    }
    return -1;
}

int32_t GradientMarkerManager::getIdByIndex(const uint32_t index) const
{
    if (index < 0 || index >= static_cast<uint32_t>(std::ssize(m_markers))) {
        return -1;
    }
    return m_markers[index].id;
}

int32_t GradientMarkerManager::getAlphaIdByIndex(const uint32_t index) const
{
    if (index < 0 || index >= static_cast<uint32_t>(std::ssize(m_alpha_markers))) {
        return -1;
    }
    return m_alpha_markers[index].id;
}

std::vector<float> GradientMarkerManager::getMarkerPos() const
{
    std::vector<float> pos(static_cast<uint32_t>(std::ssize(m_markers)));
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        pos[static_cast<uint32_t>(i)] = marker.pos;
    }
    return pos;
}

std::vector<ImVec4> GradientMarkerManager::getMarkerColors() const
{
    std::vector<ImVec4> colors(static_cast<uint32_t>(std::ssize(m_markers)));
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        colors[static_cast<uint32_t>(i)] = marker.color;
    }
    return colors;
}

std::vector<float> GradientMarkerManager::getAlphaMarkerValues() const
{
    std::vector<float> values(static_cast<uint32_t>(std::ssize(m_alpha_markers)));
    for (const auto& [i, marker] : m_alpha_markers | std::views::enumerate) {
        values[static_cast<uint32_t>(i)] = marker.value;
    }
    return values;
}

std::vector<float> GradientMarkerManager::getAlphaMarkerPos() const
{
    std::vector<float> pos(static_cast<uint32_t>(std::ssize(m_alpha_markers)));
    for (const auto& [i, marker] : m_alpha_markers | std::views::enumerate) {
        pos[static_cast<uint32_t>(i)] = marker.pos;
    }
    return pos;
}

std::vector<float> GradientMarkerManager::getMidpointRatios() const
{
    std::vector<float> ratios(static_cast<uint32_t>(std::ssize(m_markers)) - 1);
    for (const auto& [i, marker] : m_markers | std::views::take(std::ssize(m_markers) - 1) | std::views::enumerate) {
        ratios[static_cast<uint32_t>(i)] = marker.midpoint.ratio;
    }
    return ratios;
}

std::vector<float> GradientMarkerManager::getAlphaMidpointRatios() const
{
    std::vector<float> ratios(static_cast<uint32_t>(std::ssize(m_alpha_markers)) - 1);
    for (const auto& [i, marker] : m_alpha_markers | std::views::take(std::ssize(m_alpha_markers) - 1) | std::views::enumerate) {
        ratios[static_cast<uint32_t>(i)] = marker.midpoint.ratio;
    }
    return ratios;
}

float GradientMarkerManager::getMarkerPosFromMousePos(const ImVec2& mouse_pos) const
{
    ImVec2 mouse_pos_on_gradient = getMousePosOnGradient(mouse_pos);
    float marker_pos             = std::clamp(mouse_pos_on_gradient.x / (m_regions.gradient_p1.x - m_regions.gradient_p0.x), 0.0f, 1.0f);
    return marker_pos;
}

void GradientMarkerManager::setMarkerPos(const int32_t id, const float pos)
{
    moveMarker(id, pos);
}

void GradientMarkerManager::setMarkerColor(const int32_t id, const ImVec4& color)
{
    int32_t idx = getIndexById(id);
    if (idx == -1) return;
    m_markers[idx].color = color;
}

void GradientMarkerManager::setMidpointRatio(const int32_t id, const float ratio)
{
    moveMidpointRatio(id, ratio);
}

void GradientMarkerManager::setAlphaMidpointRatio(const int32_t id, const float ratio)
{
    moveAlphaMidpointRatio(id, ratio);
}

void GradientMarkerManager::setAlphaMarkerPos(const int32_t id, const float pos)
{
    int32_t idx = getAlphaIndexById(id);
    if (idx == -1) return;
    m_alpha_markers[idx].pos = std::clamp(pos, 0.0f, 1.0f);
}

void GradientMarkerManager::setAlphaMarkerValue(const int32_t id, const float value)
{
    int32_t idx = getAlphaIndexById(id);
    if (idx == -1) return;
    m_alpha_markers[idx].value = std::clamp(value, 0.0f, 1.0f);
}

void GradientMarkerManager::setSelectedMarkerPos(const float pos)
{
    moveMarker(m_state.selected_marker_id, pos);
}

void GradientMarkerManager::setSelectedAlphaMarkerPos(const float pos)
{
    moveAlphaMarker(m_state.selected_alpha_marker_id, pos);
}

void GradientMarkerManager::setSelectedMarkerColor(const ImVec4& color)
{
    int32_t idx = getIndexById(m_state.selected_marker_id);
    if (idx == -1) return;
    m_markers[idx].color = color;
}

void GradientMarkerManager::setSelectedAlphaMarkerValue(const float value)
{
    int32_t idx = getAlphaIndexById(m_state.selected_alpha_marker_id);
    if (idx == -1) return;
    m_alpha_markers[idx].value = value;
}

void GradientMarkerManager::setSelectedMidpointRatio(const float ratio)
{
    moveMidpointRatio(m_state.selected_midpoint_id, ratio);
}

void GradientMarkerManager::setSelectedAlphaMidpointRatio(const float ratio)
{
    moveAlphaMidpointRatio(m_state.selected_alpha_midpoint_id, ratio);
}

void GradientMarkerManager::setMidpointRegion(const ImVec2& p0, const ImVec2& p1) noexcept
{
    m_regions.midpoint_p0 = p0;
    m_regions.midpoint_p1 = p1;
}

void GradientMarkerManager::setAlphaMidpointRegion(const ImVec2& p0, const ImVec2& p1) noexcept
{
    m_regions.alpha_midpoint_p0 = p0;
    m_regions.alpha_midpoint_p1 = p1;
}

void GradientMarkerManager::setGradientRegion(const ImVec2& p0, const ImVec2& p1) noexcept
{
    m_regions.gradient_p0 = p0;
    m_regions.gradient_p1 = p1;
}

void GradientMarkerManager::setMarkerRegion(const ImVec2& p0, const ImVec2& p1) noexcept
{
    m_regions.marker_p0 = p0;
    m_regions.marker_p1 = p1;
}

void GradientMarkerManager::setAlphaMarkerRegion(const ImVec2& p0, const ImVec2& p1) noexcept
{
    m_regions.alpha_marker_p0 = p0;
    m_regions.alpha_marker_p1 = p1;
}

void GradientMarkerManager::changeMarkerCount(const uint32_t marker_count)
{
    if (marker_count < 2) return;

    uint32_t cur_marker_count = static_cast<uint32_t>(std::ssize(m_markers));
    if (marker_count == cur_marker_count) {
        return;
    } else if (marker_count > cur_marker_count) {
        for (uint32_t i = 0; i < marker_count - cur_marker_count; ++i) {
            addMarker(m_state.marker_id_counter, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f});
            ++m_state.marker_id_counter;
        }
    } else {
        for (uint32_t i = 0; i < cur_marker_count - marker_count; ++i) {
            if (!m_markers.empty()) {
                deleteMarker(getIdByIndex(static_cast<uint32_t>(std::ssize(m_markers)) - 1));
            }
        }
    }

    sortMarkers();
    updateMidpointsPos();
}

void GradientMarkerManager::changeAlphaMarkerCount(const uint32_t alpha_marker_count)
{
    if (alpha_marker_count < 2) return;

    uint32_t cur_alpha_marker_count = static_cast<uint32_t>(std::ssize(m_alpha_markers));
    if (alpha_marker_count == cur_alpha_marker_count) {
        return;
    } else if (alpha_marker_count > cur_alpha_marker_count) {
        for (uint32_t i = 0; i < alpha_marker_count - cur_alpha_marker_count; ++i) {
            addAlphaMarker(m_state.alpha_marker_id_counter, 0.0f, 1.0f);
            ++m_state.alpha_marker_id_counter;
        }
    } else {
        for (uint32_t i = 0; i < cur_alpha_marker_count - alpha_marker_count; ++i) {
            if (!m_alpha_markers.empty()) {
                deleteAlphaMarker(getAlphaIdByIndex(static_cast<uint32_t>(std::ssize(m_alpha_markers)) - 1));
            }
        }
    }

    sortAlphaMarkers();
    updateAlphaMidpointsPos();
}

void GradientMarkerManager::setDefaultMarkers(const std::vector<GradientMarkerData>& marker_data)
{
    m_markers.clear();
    for (const auto& [i, marker] : marker_data | std::views::enumerate) {
        GradientMarkerData data = {
            .id       = static_cast<int32_t>(i),
            .pos      = std::clamp(marker.pos, 0.0f, 1.0f),
            .color    = marker.color,
            .midpoint = {
                .ratio = std::clamp(marker.midpoint.ratio, 0.0f, 1.0f),
                .pos   = std::min(marker.midpoint.pos, 1.0f)}};
        m_markers.push_back(data);
    }

    m_state.selected_marker_id   = 0;
    m_state.selected_midpoint_id = 0;
    m_state.marker_id_counter    = static_cast<int32_t>(std::ssize(m_markers));

    sortMarkers();
    updateMidpointsPos();
}

void GradientMarkerManager::setDefaultAlphaMarkers(const std::vector<AlphaMarkerData>& alpha_marker_data)
{
    m_alpha_markers.clear();
    for (const auto& [i, marker] : alpha_marker_data | std::views::enumerate) {
        AlphaMarkerData data = {
            .id    = static_cast<int32_t>(i),
            .pos   = std::clamp(marker.pos, 0.0f, 1.0f),
            .value = std::clamp(marker.value, 0.0f, 1.0f)};
        m_alpha_markers.push_back(data);
    }

    m_state.selected_alpha_marker_id = 0;
    m_state.alpha_marker_id_counter = static_cast<int32_t>(std::ssize(m_alpha_markers));

    sortAlphaMarkers();
    updateAlphaMidpointsPos();
}

// マーカー位置昇順にソートするヘルパー
void GradientMarkerManager::sortMarkers()
{
    std::sort(m_markers.begin(), m_markers.end(),
              [](const GradientMarkerData& a, const GradientMarkerData& b) {
                  return a.pos < b.pos;
              });
}

void GradientMarkerManager::sortAlphaMarkers()
{
    std::sort(m_alpha_markers.begin(), m_alpha_markers.end(),
              [](const AlphaMarkerData& a, const AlphaMarkerData& b) {
                  return a.pos < b.pos;
              });
}

// ID を基準に昇順にソート
void GradientMarkerManager::sortMarkersById()
{
    std::sort(m_markers.begin(), m_markers.end(),
              [](const GradientMarkerData& a, const GradientMarkerData& b) {
                  return a.id < b.id;
              });
}

void GradientMarkerManager::moveMarker(const int32_t id, const float new_pos)
{
    int32_t idx = getIndexById(id);
    if (idx == -1) return;

    // 位置を更新
    m_markers[idx].pos = std::clamp(new_pos, 0.0f, 1.0f);

    sortMarkers();
    updateMidpointsPos();
}

void GradientMarkerManager::moveAlphaMarker(const int32_t id, const float new_pos)
{
    int32_t idx = getAlphaIndexById(id);
    if (idx == -1) return;

    // 位置を更新
    m_alpha_markers[idx].pos = std::clamp(new_pos, 0.0f, 1.0f);

    sortAlphaMarkers();
    updateAlphaMidpointsPos();
}

void GradientMarkerManager::moveMidpoint(const int32_t id, const float new_pos)
{
    int idx = getIndexById(id);
    if (std::ssize(m_markers) - 1 <= idx) return;

    float left_pos  = m_markers[idx].pos;
    float right_pos = m_markers[idx + 1].pos;
    float range     = right_pos - left_pos;

    if (range <= 0.0001f) return;

    // 比率を計算して保存
    float ratio                   = (new_pos - left_pos) / range;
    m_markers[idx].midpoint.ratio = std::clamp(ratio, 0.0f, 1.0f);

    // 表示用の座標更新
    updateMidpointsPos();
}

void GradientMarkerManager::moveAlphaMidpoint(const int32_t id, const float new_pos)
{
    int idx = getAlphaIndexById(id);
    if (std::ssize(m_alpha_markers) - 1 <= idx) return;

    float left_pos  = m_alpha_markers[idx].pos;
    float right_pos = m_alpha_markers[idx + 1].pos;
    float range     = right_pos - left_pos;

    if (range <= 0.0001f) return;

    // 比率を計算して保存
    float ratio                   = (new_pos - left_pos) / range;
    m_alpha_markers[idx].midpoint.ratio = std::clamp(ratio, 0.0f, 1.0f);

    // 表示用の座標更新
    updateAlphaMidpointsPos();
}

void GradientMarkerManager::moveMidpointRatio(const int32_t id, const float new_ratio)
{
    int idx = getIndexById(id);
    if (std::ssize(m_markers) - 1 <= idx) return;
    m_markers[idx].midpoint.ratio = std::clamp(new_ratio, 0.0f, 1.0f);
    // 表示用の座標更新
    updateMidpointsPos();
}

void GradientMarkerManager::moveAlphaMidpointRatio(const int32_t id, const float new_ratio)
{
    int idx = getAlphaIndexById(id);
    if (std::ssize(m_alpha_markers) - 1 <= idx) return;
    m_alpha_markers[idx].midpoint.ratio = std::clamp(new_ratio, 0.0f, 1.0f);
    // 表示用の座標更新
    updateAlphaMidpointsPos();
}

void GradientMarkerManager::reverseMarkers()
{
    int32_t right_midpoint_idx = getIndexById(m_state.selected_midpoint_id) + 1;
    int32_t right_midpoint_id  = m_state.selected_midpoint_id;
    std::vector<float> old_midpoints(static_cast<uint32_t>(std::ssize(m_markers) - 1));
    // 逆順にする
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        marker.pos = std::clamp(1.0f - marker.pos, 0.0f, 1.0f);
        if (i < static_cast<uint32_t>(std::ssize(m_markers)) - 1) {
            old_midpoints[i] = marker.midpoint.ratio;
        }
        if (i == right_midpoint_idx) {
            right_midpoint_id = marker.id;
        }
    }

    std::ranges::reverse(old_midpoints.begin(), old_midpoints.end());

    sortMarkers();

    // 中間点
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        if (i < static_cast<uint32_t>(std::ssize(m_markers)) - 1) {
            setMidpointRatio(marker.id, std::clamp(1.0f - old_midpoints[i], 0.0f, 1.0f));
        }
    }
    m_state.selected_midpoint_id = right_midpoint_id;

    updateMidpointsPos();
}

void GradientMarkerManager::reverseAlphaMarkers()
{
    for (const auto& [i, marker] : m_alpha_markers | std::views::enumerate) {
        marker.pos = std::clamp(1.0f - marker.pos, 0.0f, 1.0f);
    }

    sortAlphaMarkers();
}

void GradientMarkerManager::resetMidpoints()
{
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        if (i < static_cast<uint32_t>(std::ssize(m_markers)) - 1) {
            setMidpointRatio(marker.id, 0.5f);
        }
    }
}

void GradientMarkerManager::resetAlphaMidpoints()
{
    for (const auto& [i, marker] : m_alpha_markers | std::views::enumerate) {
        if (i < static_cast<uint32_t>(std::ssize(m_alpha_markers)) - 1) {
            setAlphaMidpointRatio(marker.id, 0.5f);
        }
    }
}

void GradientMarkerManager::addMarker(const int32_t id, const float marker_pos, const ImVec4& color, const float midpoint_ratio)
{
    GradientMarkerData new_marker;
    new_marker.id             = id;
    new_marker.pos            = marker_pos;
    new_marker.color          = color;
    new_marker.midpoint.ratio = midpoint_ratio;

    m_markers.push_back(new_marker);

    sortMarkers();
    updateMidpointsPos();  // 中間点の位置は追加後に前後のマーカー位置から計算する
}

void GradientMarkerManager::addAlphaMarker(const int32_t id, const float marker_pos, const float value, const float midpoint_ratio)
{
    AlphaMarkerData new_marker;
    new_marker.id    = id;
    new_marker.pos   = marker_pos;
    new_marker.value = value;
    new_marker.midpoint.ratio = midpoint_ratio;

    m_alpha_markers.push_back(new_marker);

    sortAlphaMarkers();
    updateAlphaMidpointsPos();
}

void GradientMarkerManager::onClickedMarker(const ImVec2& mouse_pos, bool use_default_action, std::move_only_function<void(void*)> func, void* param)
{
    if (!m_io_enable) return;
    if (use_default_action) {
        return;
    } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
        if (getMarkerIdUnderMouse(mouse_pos) >= 0) {
            func(param);
        }
    }
}

void GradientMarkerManager::showColorPickerPopup()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 2));
    ImGui::PushStyleVarX(ImGuiStyleVar_ItemInnerSpacing, 2);

    if (ImGui::BeginPopup("marker_color_picker")) {
        m_state.is_open_popup = true;
        changeColor(m_state.selected_marker_id, m_state.picker_cur_color);

        // メインのカラーピッカー
        ImGui::ColorPicker4("##marker_color_picker", (float*)&m_state.picker_cur_color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_AlphaBar);
        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::Text("Current");
        ImGuiColorEditFlags color_button_flags = ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf;
        ImVec2 color_button_size               = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x * 0.6f);
        ImGui::ColorButton("##current", m_state.picker_cur_color, color_button_flags, color_button_size);

        ImGui::Text("Previous");
        if (ImGui::ColorButton("##previous", m_state.picker_backup_color, color_button_flags, color_button_size)) {
            m_state.picker_cur_color = m_state.picker_backup_color;
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::Dummy(ImVec2(avail.x, avail.y - ImGui::GetFrameHeightWithSpacing()));
        if (ImGui::Button("close", ImVec2(avail.x, ImGui::GetFrameHeight()))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndGroup();
        ImGui::EndPopup();
    } else {
        m_state.is_open_popup = false;
    }

    ImGui::PopStyleVar(2);
}

void GradientMarkerManager::showAlphaSliderPopup()
{
    if (ImGui::BeginPopup("alpha_slider_popup")) {
        if (ImGui::SliderFloat("##alpha_value", &m_state.cur_alpha_value, 0.0f, 1.0f, "%.2f")) {
            setAlphaMarkerValue(m_state.selected_alpha_marker_id, m_state.cur_alpha_value);
        }
        ImGui::EndPopup();
    }
}

void GradientMarkerManager::onDoubleClickedMarker(const ImVec2& mouse_pos, bool use_default_action, std::move_only_function<void(void*)> func, void* param)
{
    if (!m_io_enable) return;

    ImGui::PushID(this);
    // デフォルトの挙動はカラーピッカーを開く
    if (use_default_action) {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
            if (getMarkerIdUnderMouse(mouse_pos) >= 0) {
                m_state.selected_marker_id  = getMarkerIdUnderMouse(mouse_pos);
                m_state.picker_backup_color = m_state.picker_cur_color;
                m_state.picker_cur_color    = getMarkerColor(m_state.selected_marker_id);
                ImGui::OpenPopup("marker_color_picker");
            }
        }
    } else {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
            if (getMarkerIdUnderMouse(mouse_pos) >= 0) {
                func(param);
            }
        }
    }
    ImGui::PopID();
}

void GradientMarkerManager::onDoubleClickedAlphaMarker(const ImVec2& mouse_pos, bool use_default_action, std::move_only_function<void(void*)> func, void* param)
{
    if (!m_io_enable) return;

    ImGui::PushID(this);
    if (use_default_action) {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
            if (getAlphaMarkerIdUnderMouse(mouse_pos) >= 0) {
                m_state.selected_alpha_marker_id  = getAlphaMarkerIdUnderMouse(mouse_pos);
                m_state.cur_alpha_value = getAlphaMarkerValue(m_state.selected_alpha_marker_id);
                ImGui::OpenPopup("alpha_slider_popup");
            }
        }
    } else {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
            if (getAlphaMarkerIdUnderMouse(mouse_pos) >= 0) {
                func(param);
            }
        }
    }
    ImGui::PopID();
}

void GradientMarkerManager::changeColor(const int32_t id, const ImVec4& new_color)
{
    int32_t idx = getIndexById(id);
    if (idx == -1) return;
    m_markers[idx].color = new_color;
}

// 比率に基づいて中間点の絶対座標を再計算する
void GradientMarkerManager::updateMidpointsPos()
{
    for (int32_t i = 0; i < static_cast<int32_t>(std::ssize(m_markers)) - 1; ++i) {
        float left_pos  = m_markers[i].pos;
        float right_pos = m_markers[i + 1].pos;
        float ratio     = m_markers[i].midpoint.ratio;

        // 左隣のマーカーとの距離に基づいて位置を更新
        m_markers[i].midpoint.pos = left_pos + (right_pos - left_pos) * ratio;
    }
}

void GradientMarkerManager::updateAlphaMidpointsPos()
{
    for (int32_t i = 0; i < static_cast<int32_t>(std::ssize(m_alpha_markers)) - 1; ++i) {
        float left_pos  = m_alpha_markers[i].pos;
        float right_pos = m_alpha_markers[i + 1].pos;
        float ratio     = m_alpha_markers[i].midpoint.ratio;

        // 左隣のマーカーとの距離に基づいて位置を更新
        m_alpha_markers[i].midpoint.pos = left_pos + (right_pos - left_pos) * ratio;
    }
}

void GradientMarkerManager::updateMarkerId()
{
    sortMarkersById();  // IDが小さい順に昇順ソート
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        // IDを再度割り当てる
        marker.id = static_cast<int32_t>(i);
    }
    m_state.marker_id_counter = static_cast<int32_t>(std::ssize(m_markers));
    sortMarkers();  // 位置順に昇順ソートして元の並びに戻す
}

void GradientMarkerManager::updateMarker(const ImVec2& mouse_pos, const ImVec4& new_marker_color)
{
    if (!m_io_enable) return;
    if (m_state.clicked_midpoint_id >= 0) return;

    // クリックされた位置にあるマーカーIDを取得
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
        m_state.clicked_marker_id = getMarkerIdUnderMouse(mouse_pos);
        if (m_state.clicked_marker_id >= 0) {
            m_state.selected_marker_id = m_state.clicked_marker_id;
        }
    }

    // マーカーをクリックかつドラッグ状態ならマーカーを動かす
    if (m_state.clicked_marker_id >= 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left)  && !m_state.is_open_popup) {
        m_state.is_marker_added = false;  // 追加モードではない
        float marker_pos        = getMarkerPosFromMousePos(mouse_pos);
        moveMarker(m_state.selected_marker_id, marker_pos);
    } else if (m_state.clicked_marker_id == std::to_underlying(Region::Marker) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_state.is_open_popup && std::ssize(m_markers) < m_marker_max_count) {
        // クリックされた位置に中間点が無く、マーカー描画領域内かつ、現在のマーカー数がマーカーの最大数未満なら
        // クリック位置にマーカーを作成
        float marker_pos = getMarkerPosFromMousePos(mouse_pos);
        addMarker(m_state.marker_id_counter, marker_pos, new_marker_color);
        m_state.is_marker_added = true;

        m_state.selected_marker_id = m_state.marker_id_counter;  // 追加したマーカーを選択状態にする
        m_state.clicked_marker_id  = m_state.marker_id_counter;

        ++m_state.marker_id_counter;
    } else {
        m_state.is_marker_added = false;
    }

    return;
}

void GradientMarkerManager::updateAlphaMarker(const ImVec2& mouse_pos, const float new_value)
{
    if (!m_io_enable) return;
    if (m_state.clicked_alpha_midpoint_id >= 0) return;

    // クリックされた位置にあるアルファマーカーIDを取得
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
        m_state.clicked_alpha_marker_id = getAlphaMarkerIdUnderMouse(mouse_pos);
        if (m_state.clicked_alpha_marker_id >= 0) {
            m_state.selected_alpha_marker_id = m_state.clicked_alpha_marker_id;
        }
    }

    // アルファマーカーをクリックかつドラッグ状態ならアルファマーカーを動かす
    if (m_state.clicked_alpha_marker_id >= 0 && m_state.clicked_alpha_marker_id >= 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
        m_state.is_alpha_marker_added = false;
        float marker_pos              = getMarkerPosFromMousePos(mouse_pos);
        moveAlphaMarker(m_state.selected_alpha_marker_id, marker_pos);
    } else if (m_state.clicked_alpha_marker_id == std::to_underlying(Region::AlphaMarker) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_state.is_open_popup && std::ssize(m_alpha_markers) < m_marker_max_count) {
        // クリックされた位置にアルファマーカーが無く、アルファマーカー描画領域内かつ、現在のアルファマーカー数がアルファマーカーの最大数未満なら
        // クリック位置にアルファマーカーを作成
        float marker_pos = getMarkerPosFromMousePos(mouse_pos);
        addAlphaMarker(m_state.alpha_marker_id_counter, marker_pos, new_value);
        m_state.is_alpha_marker_added = true;

        m_state.selected_alpha_marker_id = m_state.alpha_marker_id_counter;  // 追加したアルファマーカーを選択状態にする
        m_state.clicked_alpha_marker_id  = m_state.alpha_marker_id_counter;

        ++m_state.alpha_marker_id_counter;
    } else {
        m_state.is_alpha_marker_added = false;
    }

    return;
}

void GradientMarkerManager::updateMidpoint(const ImVec2& mouse_pos)
{
    if (!m_io_enable) return;

    // クリックされた中間点に紐づくマーカーIDを取得
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
        m_state.clicked_midpoint_id = getMidpointIdUnderMouse(mouse_pos);
        if (m_state.clicked_midpoint_id >= 0) {
            m_state.selected_midpoint_id = m_state.clicked_midpoint_id;
        }
    }

    //　中間点をクリックかつドラッグ状態なら中間点を動かす
    if (m_state.clicked_midpoint_id >= 0 && ImGui ::IsMouseDragging(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
        m_state.selected_midpoint_id = m_state.clicked_midpoint_id;
        float marker_pos = getMarkerPosFromMousePos(mouse_pos);
        moveMidpoint(m_state.selected_midpoint_id, marker_pos);
    }

    return;
}

void GradientMarkerManager::updateAlphaMidpoint(const ImVec2& mouse_pos)
{
    if (!m_io_enable) return;

    // クリックされた中間点に紐づくマーカーIDを取得
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
        m_state.clicked_alpha_midpoint_id = getAlphaMidpointIdUnderMouse(mouse_pos);
        if (m_state.clicked_alpha_midpoint_id >= 0) {
            m_state.selected_alpha_midpoint_id = m_state.clicked_alpha_midpoint_id;
        }
    }

    //　中間点をクリックかつドラッグ状態なら中間点を動かす
    if (m_state.clicked_alpha_midpoint_id >= 0 && ImGui ::IsMouseDragging(ImGuiMouseButton_Left) && !m_state.is_open_popup) {
        m_state.selected_alpha_midpoint_id = m_state.clicked_alpha_midpoint_id;
        float marker_pos = getMarkerPosFromMousePos(mouse_pos);
        moveAlphaMidpoint(m_state.selected_alpha_midpoint_id, marker_pos);
    }

    return;
}

int32_t GradientMarkerManager::getMarkerIdUnderMouse(const ImVec2& mouse_pos) const
{
    bool is_in_marker_region =
        (mouse_pos.x >= m_regions.marker_p0.x - m_config.marker_width * 0.5f) &&
        (mouse_pos.x < m_regions.marker_p1.x + m_config.marker_width * 0.5f) &&
        (mouse_pos.y >= m_regions.marker_p0.y) &&
        (mouse_pos.y < m_regions.marker_p1.y);

    if (is_in_marker_region) {
        for (const auto& marker : m_markers) {
            float marker_center_pos_x = m_regions.gradient_p0.x + (m_regions.gradient_p1.x - m_regions.gradient_p0.x) * marker.pos;
            ImVec2 p0                 = ImVec2(marker_center_pos_x - m_config.marker_width * 0.5f, m_regions.marker_p0.y);
            ImVec2 p1                 = ImVec2(marker_center_pos_x + m_config.marker_width * 0.5f, m_regions.marker_p1.y);
            if (mouse_pos.x >= p0.x && mouse_pos.x <= p1.x &&
                mouse_pos.y >= p0.y && mouse_pos.y <= p1.y) {
                return marker.id;
            }
        }
        return std::to_underlying(Region::Marker);
    }
    return std::to_underlying(Region::OutSide);
}

int32_t GradientMarkerManager::getAlphaMarkerIdUnderMouse(const ImVec2& mouse_pos) const
{
    bool is_in_alpha_marker_region =
        (mouse_pos.x >= m_regions.alpha_marker_p0.x - m_config.marker_width * 0.5f) &&
        (mouse_pos.x < m_regions.alpha_marker_p1.x + m_config.marker_width * 0.5f) &&
        (mouse_pos.y >= m_regions.alpha_marker_p0.y) &&
        (mouse_pos.y < m_regions.alpha_marker_p1.y);

    if (is_in_alpha_marker_region) {
        for (const auto& marker : m_alpha_markers) {
            float marker_center_pos_x = m_regions.gradient_p0.x + (m_regions.gradient_p1.x - m_regions.gradient_p0.x) * marker.pos;
            ImVec2 p0                 = ImVec2(marker_center_pos_x - m_config.marker_width * 0.5f, m_regions.alpha_marker_p0.y);
            ImVec2 p1                 = ImVec2(marker_center_pos_x + m_config.marker_width * 0.5f, m_regions.alpha_marker_p1.y);
            if (mouse_pos.x >= p0.x && mouse_pos.x <= p1.x &&
                mouse_pos.y >= p0.y && mouse_pos.y <= p1.y) {
                return marker.id;
            }
        }
        return std::to_underlying(Region::AlphaMarker);
    }
    return std::to_underlying(Region::OutSide);
}

// midpointId == markerid
int32_t GradientMarkerManager::getMidpointIdUnderMouse(const ImVec2& mouse_pos) const
{
    bool is_in_midpoint_region =
        (mouse_pos.x >= m_regions.midpoint_p0.x) &&
        (mouse_pos.x < m_regions.midpoint_p1.x) &&
        (mouse_pos.y >= m_regions.midpoint_p0.y) &&
        (mouse_pos.y < m_regions.midpoint_p1.y);

    if (is_in_midpoint_region) {
        for (const auto& marker : m_markers | std::views::take(std::ssize(m_markers) - 1)) {
            float midpoint_center_pos_x = m_regions.midpoint_p0.x + (m_regions.midpoint_p1.x - m_regions.midpoint_p0.x) * marker.midpoint.pos;
            ImVec2 p0                   = ImVec2(midpoint_center_pos_x - m_config.midpoint_width * 0.5f, m_regions.midpoint_p0.y);
            ImVec2 p1                   = ImVec2(midpoint_center_pos_x + m_config.midpoint_width * 0.5f, m_regions.midpoint_p1.y);
            if (mouse_pos.x >= p0.x && mouse_pos.x <= p1.x &&
                mouse_pos.y >= p0.y && mouse_pos.y <= p1.y) {
                return marker.id;
            }
        }
        return std::to_underlying(Region::Midpoint);
    }
    return std::to_underlying(Region::OutSide);
}

int32_t GradientMarkerManager::getAlphaMidpointIdUnderMouse(const ImVec2& mouse_pos) const
{
    bool is_in_midpoint_region =
        (mouse_pos.x >= m_regions.alpha_midpoint_p0.x) &&
        (mouse_pos.x < m_regions.alpha_midpoint_p1.x) &&
        (mouse_pos.y >= m_regions.alpha_midpoint_p0.y) &&
        (mouse_pos.y < m_regions.alpha_midpoint_p1.y);

    if (is_in_midpoint_region) {
        for (const auto& marker : m_alpha_markers | std::views::take(std::ssize(m_alpha_markers) - 1)) {
            float midpoint_center_pos_x = m_regions.alpha_midpoint_p0.x + (m_regions.alpha_midpoint_p1.x - m_regions.alpha_midpoint_p0.x) * marker.midpoint.pos;
            ImVec2 p0                   = ImVec2(midpoint_center_pos_x - m_config.midpoint_width * 0.5f, m_regions.alpha_midpoint_p0.y);
            ImVec2 p1                   = ImVec2(midpoint_center_pos_x + m_config.midpoint_width * 0.5f, m_regions.alpha_midpoint_p1.y);
            if (mouse_pos.x >= p0.x && mouse_pos.x <= p1.x &&
                mouse_pos.y >= p0.y && mouse_pos.y <= p1.y) {
                return marker.id;
            }
        }
        return std::to_underlying(Region::AlphaMidpoint);
    }
    return std::to_underlying(Region::OutSide);
}

ImVec2 GradientMarkerManager::getMousePosOnGradient(const ImVec2& mouse_pos) const
{
    ImVec2 mouse_pos_on_gradient;
    mouse_pos_on_gradient.x = mouse_pos.x - m_regions.gradient_p0.x;
    mouse_pos_on_gradient.y = mouse_pos.y - m_regions.gradient_p0.y;

    return mouse_pos_on_gradient;
}

void GradientMarkerManager::deleteMarker(const int32_t id)
{
    if (std::ssize(m_markers) <= 2) return;

    int idx = getIndexById(id);
    if (idx == -1) return;

    // 削除
    m_markers.erase(m_markers.begin() + idx);

    updateMidpointsPos();
    updateMarkerId();

    // 次に選択する ID を更新 (削除した位置にある要素、または末尾ならその前)
    if (idx < std::ssize(m_markers)) {
        m_state.selected_marker_id = m_markers[idx].id;
    } else {
        m_state.selected_marker_id = m_markers.back().id;
    }

    // 選択中の中間点 ID が不正になった場合
    if (getIndexById(m_state.selected_midpoint_id) == -1 || getIndexById(m_state.selected_midpoint_id) >= std::ssize(m_markers) - 1) {
        m_state.selected_midpoint_id = m_markers[0].id;
    }
}

void GradientMarkerManager::deleteAlphaMarker(const int32_t id)
{
    if (std::ssize(m_alpha_markers) <= 2) return;

    int idx = getAlphaIndexById(id);
    if (idx == -1) return;

    // 削除
    m_alpha_markers.erase(m_alpha_markers.begin() + idx);

    updateAlphaMidpointsPos();
    updateMarkerId();

    // 次に選択する ID を更新 (削除した位置にある要素、または末尾ならその前)
    if (idx < std::ssize(m_alpha_markers)) {
        m_state.selected_alpha_marker_id = m_alpha_markers[idx].id;
    } else {
        m_state.selected_alpha_marker_id = m_alpha_markers.back().id;
    }

    // 選択中の中間点 ID が不正になった場合
    if (getAlphaIndexById(m_state.selected_alpha_marker_id) == -1 || getAlphaIndexById(m_state.selected_alpha_marker_id) >= std::ssize(m_alpha_markers) - 1) {
        m_state.selected_alpha_marker_id = m_alpha_markers[0].id;
    }
}

void GradientMarkerManager::deleteSelectedMarker()
{
    deleteMarker(m_state.selected_marker_id);
}

void GradientMarkerManager::deleteSelectedAlphaMarker()
{
    deleteAlphaMarker(m_state.selected_alpha_marker_id);
}

void GradientMarkerManager::distributeMarkersEvenly()
{
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        moveMarker(marker.id, i / static_cast<float>(std::ssize(m_markers) - 1));
    }
}

void GradientMarkerManager::distributeAlphaMarkersEvenly()
{
    for (const auto& [i, marker] : m_alpha_markers | std::views::enumerate) {
        moveAlphaMarker(marker.id, i / static_cast<float>(std::ssize(m_alpha_markers) - 1));
    }
}

void GradientMarkerManager::distributeMarkersAndMipointsEvenly()
{
    distributeMarkersEvenly();
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        if (i < static_cast<float>(std::ssize(m_markers) - 1)) {
            moveMidpoint(marker.id, (i + 1) / static_cast<float>(std::ssize(m_markers)));
        }
    }
}

void GradientMarkerManager::distributeAlphaMarkersAndAlphaMipointsEvenly()
{
    distributeAlphaMarkersEvenly();
    for (const auto& [i, marker] : m_alpha_markers | std::views::enumerate) {
        if (i < static_cast<float>(std::ssize(m_alpha_markers) - 1)) {
            moveAlphaMidpoint(marker.id, (i + 1) / static_cast<float>(std::ssize(m_alpha_markers)));
        }
    }
}

void GradientMarkerManager::selectNextMarker()
{
    if (std::ssize(m_markers) <= 2) return;

    int32_t idx = getIndexById(m_state.selected_marker_id);
    if (idx == -1) return;

    ++idx;

    if (idx < std::ssize(m_markers)) {
        m_state.selected_marker_id = m_markers[idx].id;
    } else {
        m_state.selected_marker_id = m_markers.front().id;
    }
}

void GradientMarkerManager::selectBackMarker()
{
    if (std::ssize(m_markers) <= 2) return;

    int32_t idx = getIndexById(m_state.selected_marker_id);
    if (idx == -1) return;

    --idx;

    if (idx >= 0) {
        m_state.selected_marker_id = m_markers[idx].id;
    } else {
        m_state.selected_marker_id = m_markers.back().id;
    }
}

void GradientMarkerManager::selectNextAlphaMarker()
{
    if (std::ssize(m_alpha_markers) <= 2) return;

    int32_t idx = getAlphaIndexById(m_state.selected_alpha_marker_id);
    if (idx == -1) return;

    ++idx;

    if (idx < std::ssize(m_alpha_markers)) {
        m_state.selected_alpha_marker_id = m_alpha_markers[idx].id;
    } else {
        m_state.selected_alpha_marker_id = m_alpha_markers.front().id;
    }
}

void GradientMarkerManager::selectBackAlphaMarker()
{
    if (std::ssize(m_alpha_markers) <= 2) return;

    int32_t idx = getAlphaIndexById(m_state.selected_alpha_marker_id);
    if (idx == -1) return;

    --idx;

    if (idx >= 0) {
        m_state.selected_alpha_marker_id = m_alpha_markers[idx].id;
    } else {
        m_state.selected_alpha_marker_id = m_alpha_markers.back().id;
    }
}

void GradientMarkerManager::drawMarker(const char* label, ImVec2 p0, ImVec2 p1, const ImVec4& color, const int32_t id, const bool is_upward) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float triangle_height = static_cast<float>(m_config.triangle_height);

    // 三角形を描画
    if (is_upward) {
        draw_list->AddTriangleFilled(
            ImVec2(p0.x + m_config.marker_width * 0.5f, p0.y),
            ImVec2(p1.x, p0.y + triangle_height),
            ImVec2(p0.x, p0.y + triangle_height),
            ImGui::ColorConvertFloat4ToU32(ImVec4(204.0f / 255.0f, 204.0f / 255.0f, 204.0f / 255.0f, 1.0f)));
        draw_list->AddTriangle(
            ImVec2(p0.x + m_config.marker_width * 0.5f, p0.y),
            ImVec2(p1.x, p0.y + triangle_height),
            ImVec2(p0.x, p0.y + triangle_height),
            IM_COL32(255, 255, 255, 255), 1.0f);

        p0.y += triangle_height;
    } else {
        draw_list->AddTriangleFilled(
            ImVec2(p0.x + m_config.marker_width * 0.5f, p1.y),
            ImVec2(p1.x, p1.y - triangle_height),
            ImVec2(p0.x, p1.y - triangle_height),
            ImGui::ColorConvertFloat4ToU32(ImVec4(204.0f / 255.0f, 204.0f / 255.0f, 204.0f / 255.0f, 1.0f)));
        draw_list->AddTriangle(
            ImVec2(p0.x + m_config.marker_width * 0.5f, p1.y),
            ImVec2(p1.x, p1.y - triangle_height),
            ImVec2(p0.x, p1.y - triangle_height),
            IM_COL32(255, 255, 255, 255), 1.0f);
    }

    // カラーボタンを描画
    ImGui::PushID(id);
    ImVec2 backup = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(p0);
    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoDragDrop;
    ImGui::ColorButton((std::string{label} + "##marker_color").c_str(), color, flags, ImVec2(static_cast<float>(m_config.marker_width), static_cast<float>(m_config.marker_width)));
    ImGui::SetCursorScreenPos(backup);
    ImGui::PopID();

    if (!is_upward) {
        p1.y -= triangle_height;
    }
    // 四角形の枠を描画
    draw_list->AddRect(p0, p1, IM_COL32(0, 0, 0, 255), 0, 0, 3.0f);
    draw_list->AddRect(p0, p1, IM_COL32(255, 255, 255, 255), 0, 0, 1.0f);
}

void GradientMarkerManager::drawMarkers() const
{
    // アルファマーカーはグラデーションの下に描画するため、マーカーは上向き
    bool is_upward = true;
    const char* label = "draw_marker";

    int32_t selected_marker_idx = -1;
    for (const auto& [i, marker] : m_markers | std::views::enumerate) {
        // 選択中のマーカーは最前面に描画する
        if (marker.id == m_state.selected_marker_id) {
            selected_marker_idx = static_cast<int32_t>(i);
            continue;
        }
        float pos = marker.pos;
        ImVec2 p0 = ImVec2(m_regions.marker_p0.x + (m_regions.marker_p1.x - m_regions.marker_p0.x) * pos - (m_config.marker_width * 0.5f), m_regions.marker_p0.y);
        ImVec2 p1 = ImVec2(m_regions.marker_p0.x + (m_regions.marker_p1.x - m_regions.marker_p0.x) * pos + (m_config.marker_width * 0.5f), m_regions.marker_p1.y);
        drawMarker(label, p0, p1, marker.color, marker.id, is_upward);
    }

    if (selected_marker_idx >= 0) {
        float pos = m_markers.at(selected_marker_idx).pos;
        ImVec2 p0 = ImVec2(m_regions.marker_p0.x + (m_regions.marker_p1.x - m_regions.marker_p0.x) * pos - (m_config.marker_width * 0.5f), m_regions.marker_p0.y);
        ImVec2 p1 = ImVec2(m_regions.marker_p0.x + (m_regions.marker_p1.x - m_regions.marker_p0.x) * pos + (m_config.marker_width * 0.5f), m_regions.marker_p1.y);
        drawMarker(label, p0, p1, m_markers.at(selected_marker_idx).color, m_markers.at(selected_marker_idx).id, is_upward);
        highlightMarker(p0, p1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 2.0f, 2.0f, is_upward);
    }
}

void GradientMarkerManager::drawAlphaMarkers() const
{
    // アルファマーカーはグラデーションの上に描画するため、マーカーは下向き
    bool is_upward = false;
    const char* label = "draw_alpha_marker";

    int32_t selected_marker_idx = -1;
    for (const auto& [i, marker] : m_alpha_markers | std::views::enumerate) {
        // 選択中のマーカーは最前面に描画する
        if (marker.id == m_state.selected_alpha_marker_id) {
            selected_marker_idx = static_cast<int32_t>(i);
            continue;
        }
        float pos = marker.pos;
        ImVec2 p0 = ImVec2(m_regions.alpha_marker_p0.x + (m_regions.alpha_marker_p1.x - m_regions.alpha_marker_p0.x) * pos - (m_config.marker_width * 0.5f), m_regions.alpha_marker_p0.y);
        ImVec2 p1 = ImVec2(m_regions.alpha_marker_p0.x + (m_regions.alpha_marker_p1.x - m_regions.alpha_marker_p0.x) * pos + (m_config.marker_width * 0.5f), m_regions.alpha_marker_p1.y);
        drawMarker(label, p0, p1, ImVec4(0.0f, 0.0f, 0.0f, marker.value), marker.id, is_upward);
    }

    if (selected_marker_idx >= 0) {
        float pos = m_alpha_markers.at(selected_marker_idx).pos;
        ImVec2 p0 = ImVec2(m_regions.alpha_marker_p0.x + (m_regions.alpha_marker_p1.x - m_regions.alpha_marker_p0.x) * pos - (m_config.marker_width * 0.5f), m_regions.alpha_marker_p0.y);
        ImVec2 p1 = ImVec2(m_regions.alpha_marker_p0.x + (m_regions.alpha_marker_p1.x - m_regions.alpha_marker_p0.x) * pos + (m_config.marker_width * 0.5f), m_regions.alpha_marker_p1.y);
        drawMarker(label, p0, p1, ImVec4(0.0f, 0.0f, 0.0f, m_alpha_markers.at(selected_marker_idx).value), m_alpha_markers.at(selected_marker_idx).id, is_upward);
        highlightMarker(p0, p1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 2.0f, 2.0f, is_upward);
    }
}

void GradientMarkerManager::drawMidpoint(ImVec2 p0, ImVec2 p1, const ImVec4& color) const
{
    ImVec2 center = ImVec2(p0.x + (p1.x - p0.x) * 0.5f, p0.y + (p1.y - p0.y) * 0.5f);

    ImU32 u32color        = ImGui::ColorConvertFloat4ToU32(color);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddNgon(center, m_config.midpoint_width * 0.5f, u32color, 4, 2.0f);
}

void GradientMarkerManager::drawMidpoints() const
{
    int32_t selected_midpoint_idx = -1;
    for (const auto& [i, marker] : m_markers | std::views::take(std::ssize(m_markers) - 1) | std::views::enumerate) {
        // 選択中の中間点は最前面に描画するため、最後に描画する
        if (marker.id == m_state.selected_midpoint_id) {
            selected_midpoint_idx = static_cast<int32_t>(i);
            continue;
        }
        auto pos = marker.midpoint.pos;
        ImVec2 p0 = ImVec2(m_regions.midpoint_p0.x + (m_regions.midpoint_p1.x - m_regions.midpoint_p0.x) * pos - (m_config.midpoint_width * 0.5f), m_regions.midpoint_p0.y);
        ImVec2 p1 = ImVec2(m_regions.midpoint_p0.x + (m_regions.midpoint_p1.x - m_regions.midpoint_p0.x) * pos + (m_config.midpoint_width * 0.5f), m_regions.midpoint_p1.y);
        drawMidpoint(p0, p1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    if (selected_midpoint_idx >= 0) {
        auto pos = m_markers.at(selected_midpoint_idx).midpoint.pos;
        ImVec2 p0 = ImVec2(m_regions.midpoint_p0.x + (m_regions.midpoint_p1.x - m_regions.midpoint_p0.x) * pos - (m_config.midpoint_width * 0.5f), m_regions.midpoint_p0.y);
        ImVec2 p1 = ImVec2(m_regions.midpoint_p0.x + (m_regions.midpoint_p1.x - m_regions.midpoint_p0.x) * pos + (m_config.midpoint_width * 0.5f), m_regions.midpoint_p1.y);
        drawMidpoint(p0, p1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        highlightMidpoint(p0, p1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
}

void GradientMarkerManager::drawAlphaMidpoints() const
{
    int32_t selected_midpoint_idx = -1;
    for (const auto& [i, marker] : m_alpha_markers | std::views::take(std::ssize(m_alpha_markers) - 1) | std::views::enumerate) {
        if (marker.id == m_state.selected_alpha_midpoint_id) {
            selected_midpoint_idx = static_cast<int32_t>(i);
            continue;
        }

        auto pos = marker.midpoint.pos;
        ImVec2 p0 = ImVec2(m_regions.alpha_midpoint_p0.x + (m_regions.alpha_midpoint_p1.x - m_regions.alpha_midpoint_p0.x) * pos - (m_config.midpoint_width * 0.5f), m_regions.alpha_midpoint_p0.y);
        ImVec2 p1 = ImVec2(m_regions.alpha_midpoint_p0.x + (m_regions.alpha_midpoint_p1.x - m_regions.alpha_midpoint_p0.x) * pos + (m_config.midpoint_width * 0.5f), m_regions.alpha_midpoint_p1.y);
        drawMidpoint(p0, p1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    if (selected_midpoint_idx >= 0) {
        auto pos = m_alpha_markers.at(selected_midpoint_idx).midpoint.pos;
        ImVec2 p0 = ImVec2(m_regions.alpha_midpoint_p0.x + (m_regions.alpha_midpoint_p1.x - m_regions.alpha_midpoint_p0.x) * pos - (m_config.midpoint_width * 0.5f), m_regions.alpha_midpoint_p0.y);
        ImVec2 p1 = ImVec2(m_regions.alpha_midpoint_p0.x + (m_regions.alpha_midpoint_p1.x - m_regions.alpha_midpoint_p0.x) * pos + (m_config.midpoint_width * 0.5f), m_regions.alpha_midpoint_p1.y);
        drawMidpoint(p0, p1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        highlightMidpoint(p0, p1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
}

void GradientMarkerManager::highlightMarker(ImVec2 p0, ImVec2 p1, const ImVec4& highlight_color, const float thickness, const float offset, const bool is_upward) const
{
    float triangle_height = static_cast<float>(m_config.triangle_height);
    if (is_upward) {
        p0.y += triangle_height;
    } else {
        p1.y -= triangle_height;
    }

    p0 = ImVec2(p0.x - offset, p0.y - offset);
    p1 = ImVec2(p1.x + offset, p1.y + offset);

    ImU32 u32_highlight_color = ImGui::ColorConvertFloat4ToU32(highlight_color);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(p0, p1, u32_highlight_color, 0.0f, 0, thickness);
}

void GradientMarkerManager::highlightMidpoint(ImVec2 p0, ImVec2 p1, const ImVec4& highlight_color) const
{
    ImVec2 center = ImVec2(p0.x + (p1.x - p0.x) * 0.5f, p0.y + (p1.y - p0.y) * 0.5f);

    ImU32 u32color        = ImGui::ColorConvertFloat4ToU32(highlight_color);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddNgonFilled(center, m_config.midpoint_width * 0.5f, u32color, 4);
}
