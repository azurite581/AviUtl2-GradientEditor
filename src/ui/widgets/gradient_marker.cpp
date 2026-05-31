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
    const ImVec4& arrow_color) const
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
    ImGui::PushID(std::format("{}", id).c_str());  // int64_t 型に対応していないため、文字列に変換して渡す
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
    draw_list->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(inner_border_color), 0, 3.0f, 0);
    draw_list->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(outer_border_color), 0, 1.0f, 0);
}

void MarkerData::drawMidpoint(const ImVec2 rect_p0, const ImVec2 rect_p1, const ImVec4& color, const float thickness) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 center = ImVec2(rect_p0.x + (rect_p1.x - rect_p0.x) * 0.5f, rect_p0.y + (rect_p1.y - rect_p0.y) * 0.5f);
    draw_list->AddNgon(center, midpoint_size.x * 0.5f, ImGui::ColorConvertFloat4ToU32(color), 4, thickness);
}

void MarkerData::highlightMarker(
    const ImVec2 rect_p0,
    const ImVec2 rect_p1,
    const ImVec4& highlight_color,
    const bool is_upward,
    const float thickness,
    const float offset) const
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
    draw_list->AddRect(p0, p1, ImGui::ColorConvertFloat4ToU32(highlight_color), 0.0f, thickness, 0);
}

void MarkerData::highlightMidpoint(
    const ImVec2 rect_p0,
    const ImVec2 rect_p1,
    const ImVec4& highlight_color) const
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
    if ((m_regions.marker_p0.x - m_size.marker_size.x * 0.5 <= mouse_pos.x && mouse_pos.x < m_regions.marker_p1.x + m_size.marker_size.x * 0.5) &&
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
    setMarkerPosition(id, new_pos);
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

    float ratio                   = (new_pos - left_pos) / range;
    m_markers[idx].midpoint.ratio = std::clamp(ratio, 0.0f, 1.0f);
    m_markers[idx].midpoint.pos   = new_pos;

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

    // 中間点も変更
    m_state.selected_midpoint_id = m_state.selected_marker_id;
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

    // 中間点も変更
    m_state.selected_midpoint_id = m_state.selected_marker_id;
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
        if (clicked == Clicked::Marker) {
            return true;
        }
    }
    return false;
}

void MarkerManager::changeMarkerCount(const uint32_t marker_count)
{
    if (marker_count < 2) return;

    uint32_t cur_marker_count = static_cast<uint32_t>(std::ssize(m_markers));
    if (marker_count == cur_marker_count) {
        return;
    } else if (marker_count > cur_marker_count) {
        // 増やす場合
        for (uint32_t i = 0; i < marker_count - cur_marker_count; ++i) {
            addMarker(m_state.marker_id_counter, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}, 0.5f);
            ++m_state.marker_id_counter;
        }
    } else {
        // 減らす場合
        for (uint32_t i = 0; i < cur_marker_count - marker_count; ++i) {
            if (!m_markers.empty()) {
                deleteMarker(getMarkerIdByIndex(static_cast<uint32_t>(std::ssize(m_markers)) - 1));
            }
        }
    }

    sortMarkersByPos();
    updateMidpointsPos();
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
        m_state.clicked    = clicked;
        if (m_state.clicked == Clicked::Marker) {
            m_state.selected_marker_id = id;
            m_state.selected_midpoint_id = id;
        } else if (m_state.clicked == Clicked::Midpoint) {
            m_state.selected_marker_id   = id;  // 中間点はマーカーに紐づいているため、選択マーカーも変更
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
               m_state.clicked == Clicked::MarkerRegion &&      // クリックされた位置がマーカー領域
               std::ssize(m_markers) < m_marker_max_count       // 現在のマーカー数がマーカーの最大数未満
    ) {
        // クリック位置にマーカーを作成
        float marker_pos = getMarkerPosFromMousePos(mouse_pos);
        addMarker(m_state.marker_id_counter, marker_pos, m_new_marker_value, m_default_midpoint_ratio);

        m_state.selected_marker_id = m_state.marker_id_counter;  // 追加したマーカーを選択状態にする
        m_state.clicked            = Clicked::Marker;

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
        auto [p0, p1]    = calcDrawPos(marker.pos, m_size.marker_size.x);
        marker.marker_p0 = p0;
        marker.marker_p1 = p1;
    }
}

// 各中間点の描画位置を計算してセットする
void MarkerManager::calcMidpointsDrawPos(const float thickness)
{
    // 中心位置をずらして領域の端に揃えるための係数
    const float marker_region_height = m_regions.marker_p1.y - m_regions.marker_p0.y;
    const float midpoint_height      = m_size.midpoint_size.y + thickness;
    const float diff                 = marker_region_height - midpoint_height;

    for (auto& marker : m_markers) {
        auto [p0, p1]      = calcDrawPos(marker.midpoint.pos, m_size.midpoint_size.x);
        p0.y               = m_is_upward ? p0.y - diff : p0.y + diff;
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
        // マーカーのサイズをセット（各マーカーのサイズではなく、MarkerManager が保持するサイズを使用する）
        marker.marker_size       = m_size.marker_size;
        marker.marker_arrow_size = m_size.marker_arrow_size;

        // 選択中のマーカーは最前面に描画するため、最後に描画する
        if (marker.id == m_state.selected_marker_id) {
            selected_marker_idx = static_cast<int32_t>(i);
            continue;
        }
        marker.drawMarker(
            label,
            marker.id,
            marker.marker_p0,
            marker.marker_p1,
            marker.value,
            m_is_upward,
            m_color.marker_inner_border_color,
            m_color.marker_outer_border_color,
            m_color.marker_arrow_color);
    }

    if (selected_marker_idx >= 0) {
        auto& selected_marker             = m_markers.at(selected_marker_idx);
        selected_marker.marker_size       = m_size.marker_size;
        selected_marker.marker_arrow_size = m_size.marker_arrow_size;

        selected_marker.drawMarker(
            label,
            selected_marker.id,
            selected_marker.marker_p0,
            selected_marker.marker_p1,
            selected_marker.value,
            m_is_upward,
            m_color.marker_inner_border_color,
            m_color.marker_outer_border_color,
            m_color.marker_arrow_color);
        selected_marker.highlightMarker(
            selected_marker.marker_p0,
            selected_marker.marker_p1,
            m_color.marker_outer_border_color,
            m_is_upward);
    }

    ImGui::PopID();
}

void MarkerManager::drawMidpoints()
{
    updateMidpointsPos();                             // midpoint.ratio から midpoint.pos を計算して格納
    calcMidpointsDrawPos(m_size.midpoint_thickness);  // 描画座標を計算して midpoint_p0, midopint_p1 に格納

    int32_t selected_midpoint_idx = -1;
    for (const auto& [i, marker] : m_markers | std::views::take(std::ssize(m_markers) - 1) | std::views::enumerate) {
        marker.midpoint_size = m_size.midpoint_size;

        // 選択中のマーカーは最前面に描画するため、最後に描画する
        if (marker.id == m_state.selected_midpoint_id) {
            selected_midpoint_idx = static_cast<int32_t>(i);
            continue;
        }
        marker.drawMidpoint(marker.midpoint_p0, marker.midpoint_p1, m_color.midpoint_color, m_size.midpoint_thickness);
    }

    if (selected_midpoint_idx >= 0) {
        auto& selected_marker         = m_markers.at(selected_midpoint_idx);
        selected_marker.midpoint_size = m_size.midpoint_size;

        selected_marker.drawMidpoint(selected_marker.midpoint_p0, selected_marker.midpoint_p1, m_color.midpoint_color, m_size.midpoint_thickness);
        selected_marker.highlightMidpoint(selected_marker.midpoint_p0, selected_marker.midpoint_p1, m_color.midpoint_color);
    }
}
