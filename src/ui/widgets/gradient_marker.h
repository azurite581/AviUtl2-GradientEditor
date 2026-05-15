#ifndef GRADIENT_MARKER_H
#define GRADIENT_MARKER_H

#include <algorithm>
#include <compare>
#include <cstdint>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>
#include <variant>

#include "imgui.h"
#include <cmath>

struct Midpoint {
    float ratio{0.5f};
    float pos{0.0f};

    auto operator<=>(const Midpoint&) const = default;
};

struct MarkerData {
    MarkerData() {}
    MarkerData(const int64_t id_, const float pos_, const ImVec4& value_, const float midpoint_ratio)
        : id{id_},
        pos{pos_},
        value{value_},
        midpoint{midpoint_ratio, 0.0f}
    {
    }

    int64_t id{0};
    float pos{0.0f};
    ImVec4 value{1.0f, 1.0f, 1.0f, 1.0f};
    Midpoint midpoint{0.5f, 0.0f};

    ImVec2 marker_p0{}, marker_p1{};
    ImVec2 midpoint_p0{}, midpoint_p1{};

    ImVec2 marker_arrow_size {20.0f, 10.0f};
    ImVec2 marker_size{20.f, 20.f};
    ImVec2 midpoint_size{20.0f, 20.0f};

    void drawMarker(
        const char* label,
        const int64_t id,
        const ImVec2 rect_p0, const ImVec2 rect_p1,
        const ImVec4& rect_color,
        const bool is_upward = true,
        const ImVec4& inner_border_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
        const ImVec4& outer_border_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
        const ImVec4& arrow_color = ImVec4(204.0f / 255.0f, 204.0f / 255.0f, 204.0f / 255.0f, 1.0f)
    ) const;

    void drawMidpoint(
        const ImVec2 rect_p0,
        const ImVec2 rect_p1,
        const ImVec4& color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
    ) const;

    void highlightMarker(
        const ImVec2 rect_p0,
        const ImVec2 rect_p1,
        const ImVec4& highlight_color,
        const bool is_upward = true,
        const float thickness = 2.0f,
        const float offset = 2.0f
    ) const;

    void highlightMidpoint(
        const ImVec2 rect_p0,
        const ImVec2 rect_p1,
        const ImVec4& highlight_color
    ) const;

    bool operator==(const MarkerData& o) const noexcept
    {
        if (id != o.id) return false;
        if (pos != o.pos) return false;
        if (midpoint != o.midpoint) return false;
        if (value.x != o.value.x) return false;
        if (value.y != o.value.y) return false;
        if (value.z != o.value.z) return false;
        if (value.w != o.value.w) return false;
        return true;
    }
};

class MarkerManager {
private:
    enum class Region : int32_t {
        Marker   = -1,
        Midpoint = -2,
        Gradient = -3,
        OutSide  = -4,
    };

    enum class Clicked : int32_t {
        Marker = -1,
        Midpoint = -2,
        MarkerRegion = -3,
        OutSide = -4
    };

    struct Regions {
        ImVec2 gradient_p0{}, gradient_p1{};
        ImVec2 marker_p0{}, marker_p1{};
    };

    struct State {
        Clicked clicked = Clicked::OutSide;
        int64_t selected_marker_id{0};
        int64_t selected_midpoint_id{0};
        int64_t marker_id_counter{2};
    };

    Regions m_regions{};
    State m_state{};

    bool m_io_enable{true};
    bool m_is_upward{true};

    uint32_t m_marker_max_count{30};

    std::vector<MarkerData> m_markers;

    float m_default_midpoint_ratio = 0.5f;
    ImVec4 m_default_value{1.0f, 1.0f, 1.0f, 1.0f};

    ImVec2 marker_arrow_size {20.0f, 10.0f};
    ImVec2 marker_size{20.f, 20.f};
    ImVec2 midpoint_size{20.0f, 20.0f};

    ImVec4 m_new_marker_value{1.0f, 1.0f, 1.0f, 1.0f};  // 新しく追加するマーカーの色

public:
    MarkerManager(const std::vector<MarkerData>& markers)
    {
        m_markers = markers;
    }

    MarkerManager()
    {
        // 初期値をセット
        MarkerData marker_data_0;
        marker_data_0.id = 0;
        marker_data_0.pos = 0.0f;
        marker_data_0.midpoint.ratio = 0.5f;
        marker_data_0.value = ImVec4(0, 0, 0, 1);
        m_markers.push_back(marker_data_0);

        MarkerData marker_data_1;
        marker_data_1.id = 1;
        marker_data_1.pos = 1.0f;
        marker_data_1.midpoint.ratio = 0.5f;
        marker_data_1.value = ImVec4(1, 1, 1, 1);
        m_markers.push_back(marker_data_1);
    }

    //
    // ゲッター
    //
    [[nodiscard]] ImVec4 getNewMarkerValue() const noexcept { return m_new_marker_value; }

    [[nodiscard]] float getMarkerRegionHeight() const noexcept
    {
        float max_height = 0;
        for (const auto& marker : m_markers) {
            float height = marker.marker_arrow_size.y + marker.marker_size.y;
            if (max_height < height) max_height = height;
        }
        return max_height;
    }

    // 各種描画領域を取得
    [[nodiscard]] std::pair<ImVec2, ImVec2> getMarkerRegion() const noexcept { return {m_regions.marker_p0, m_regions.marker_p1}; }
    [[nodiscard]] std::pair<ImVec2, ImVec2> getGradientRegion() const noexcept { return {m_regions.gradient_p0, m_regions.gradient_p1}; }

    // インデックス⇔IDの相互変換
    [[nodiscard]] int64_t getMarkerIndexById(const int64_t id) const;
    [[nodiscard]] int64_t getMarkerIdByIndex(const int64_t index) const;

    // 各種マーカーの値を取得
    [[nodiscard]] std::vector<MarkerData> getMarkers() const
    {
        return m_markers;
    }

    [[nodiscard]] std::vector<ImVec4> getMarkerValues() const
    {
        std::vector<ImVec4> values(static_cast<int32_t>(std::ssize(m_markers)));
        for (const auto& [i, marker] : m_markers | std::views::enumerate) {
            values[i] = marker.value;
        }

        return values;
    }

    [[nodiscard]] std::vector<float> getMarkerPositions() const
    {
        std::vector<float> positions(static_cast<int32_t>(std::ssize(m_markers)));
        for (const auto& [i, marker] : m_markers | std::views::enumerate) {
            positions[i] = marker.pos;
        }

        return positions;
    }

    [[nodiscard]] std::vector<float> getMidpointRatios() const
    {
        std::vector<float> midpoint_ratios(static_cast<int32_t>(std::ssize(m_markers)));
        for (const auto& [i, marker] : m_markers | std::views::enumerate) {
            midpoint_ratios[i] = marker.midpoint.ratio;
        }

        return midpoint_ratios;
    }

    // IDで指定したマーカーの値を取得
    [[nodiscard]] ImVec4 getMarkerValue(const int64_t id) const
    {
        auto idx = getMarkerIndexById(id);
        if (idx == -1) return {};

        return m_markers[idx].value;
    }

    [[nodiscard]] float getMarkerPosition(const int64_t id) const
    {
        auto idx = getMarkerIndexById(id);
        if (idx == -1) return {};

        return m_markers[idx].pos;
    }

    [[nodiscard]] float getMidpointRatio(const int64_t id) const
    {
        auto idx = getMarkerIndexById(id);
        if (idx == -1) return {};

        return m_markers[idx].midpoint.ratio;
    }

    // 選択中のマーカーの値を取得
    [[nodiscard]] int64_t getSelectedMarkerId() const noexcept
    {
        return m_state.selected_marker_id;
    }

    [[nodiscard]] int64_t getSelectedMidpointId() const noexcept
    {
        return m_state.selected_midpoint_id;
    }

    [[nodiscard]] MarkerData getSelectedMarkerData() const
    {
        auto idx = getMarkerIndexById(m_state.selected_marker_id);
        if (idx == -1) return MarkerData{};

        return m_markers[idx];
    }

    [[nodiscard]] ImVec4 getSelectedMarkerValue() const
    {
        auto idx = getMarkerIndexById(m_state.selected_marker_id);
        if (idx == -1) return m_default_value;

        return m_markers[idx].value;
    }

    [[nodiscard]] float getSelectedMarkerPosition() const
    {
        auto idx = getMarkerIndexById(m_state.selected_marker_id);
        if (idx == -1) return {};

        return m_markers[idx].pos;
    }

    [[nodiscard]] float getSelectedMidpointRatio() const
    {
        auto idx = getMarkerIndexById(m_state.selected_marker_id);
        if (idx == -1) return {};

        return m_markers[idx].midpoint.ratio;
    }

    // マウス下のマーカー/中間点を取得
    [[nodiscard]] std::pair<Clicked, int64_t> getMarkerIdUnderMouse(const ImVec2& mouse_pos) const;
    // マウスのグラデーション上での位置
    [[nodiscard]] ImVec2 getMousePosOnGradient(const ImVec2& mouse_pos) const;

    [[nodiscard]] float getMarkerPosFromMousePos(const ImVec2& mouse_pos) const
    {
        ImVec2 mouse_pos_on_gradient = getMousePosOnGradient(mouse_pos);
        float marker_pos = std::clamp(mouse_pos_on_gradient.x / (m_regions.gradient_p1.x - m_regions.gradient_p0.x), 0.0f, 1.0f);
        return marker_pos;
    }

    //
    // セッター
    //
    void setMarkerSize(const ImVec2& size) noexcept
    {
        marker_size = size;
    }

    void setMidpointSize(const ImVec2& size) noexcept
    {
        midpoint_size = size;
    }

    // 描画領域
    void setGradientRegion(const ImVec2& p0, const ImVec2& p1) noexcept
    {
        m_regions.gradient_p0 = p0;
        m_regions.gradient_p1 = p1;
    }

    void setMarkerRegion(const ImVec2& p0, const ImVec2& p1) noexcept
    {
        m_regions.marker_p0 = p0;
        m_regions.marker_p1 = p1;
    };

    // 各種値
    void setMarkers(const std::vector<MarkerData>& markers)
    {
        m_markers = markers;
    }

    void setMarkerValue(const int64_t id, const ImVec4& value)
    {
        auto idx = getMarkerIndexById(id);
        if (idx == -1) return;
        m_markers[idx].value = value;
    }

    void setMarkerPosition(const int64_t id, const float pos)
    {
        auto idx = getMarkerIndexById(id);
        if (idx == -1) return;
        m_markers[idx].pos = pos;
    }

    void setMidpointRatio(const int64_t id, const float ratio)
    {
        auto idx = getMarkerIndexById(id);
        if (idx == -1) return;
        m_markers[idx].midpoint.ratio = ratio;
    }

    // 選択中の値
    void setSelectedMarkerId(const int64_t id) noexcept
    {
        m_state.selected_marker_id = id;
    }

    void setSelectedMidpointId(const int64_t id) noexcept
    {
        m_state.selected_midpoint_id = id;
    }

    void setSelectedMarkerValue(const ImVec4& value)
    {
        auto idx = getMarkerIndexById(m_state.selected_marker_id);
        if (idx == -1) return;
        m_markers[idx].value = value;
    }

    void setSelectedMarkerPosition(const float pos)
    {
        auto idx = getMarkerIndexById(m_state.selected_marker_id);
        if (idx == -1) return;
        m_markers[idx].pos = std::clamp(pos, 0.0f, 1.0f);
    }

    void setSelectedMidpointRatio(const float ratio)
    {
        auto idx = getMarkerIndexById(m_state.selected_marker_id);
        if (idx == -1) return;
        m_markers[idx].midpoint.ratio = std::clamp(ratio, 0.0f, 1.0f);
    }

    void setIOEnable(const bool enable) noexcept { m_io_enable = enable; }
    void setMarkerUpward(const bool upward) noexcept { m_is_upward = upward; }
    void setMarkerMaxCount(const int64_t max_count) noexcept { m_marker_max_count = max_count; }
    void setNewMarkerValue(const ImVec4& new_value) noexcept { m_new_marker_value = new_value; }

    //
    // 操作
    //
    void resetMarkers(const std::vector<MarkerData>& markers)
    {
        auto it = std::ranges::max_element(markers, {}, &MarkerData::id);
        if (it != markers.end()) {
            int64_t max_id = it->id;

            m_markers = markers;
            m_state.selected_marker_id = markers.front().id;
            m_state.selected_midpoint_id = markers.front().id;
            m_state.marker_id_counter = max_id + 1;
        }
    }

    void reverseMarkers();
    void resetMidpoints();

    void sortMarkersByPos();
    void sortMarkersById();

    void moveMarker(const int64_t id, const float new_pos);
    void moveMidpoint(const int64_t id, const float new_pos);
    void addMarker(const int64_t id, const float marker_pos, const ImVec4& value, const float midpoint_ratio);

    void deleteMarker(const int64_t id);
    void deleteSelectedMarker();
    void distributeMarkersEvenly();
    void distributeMarkersAndMipointsEvenly();

    void selectNextMarker();
    void selectBackMarker();

    bool isDoubleClickedMarker(const ImVec2& mouse_pos);
    void changeMarkerCount(const uint32_t marker_count);

    //
    // 更新
    //
    void updateMarkerAndMidpointPosition(const ImVec2& mouse_pos);
    void updateMidpointsPos();
    void reassignMarkerID();

    //
    // 描画
    //
    std::pair<ImVec2, ImVec2> calcDrawPos(const float marker_pos, const float marker_width) const;
    void calcMarkersDrawPos();
    void calcMidpointsDrawPos();
    void drawMarkers();
    void drawMidpoints();
};

#endif  // GRADIENT_MARKER_H
