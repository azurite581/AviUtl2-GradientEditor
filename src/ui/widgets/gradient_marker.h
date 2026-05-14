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

struct Midpoint {
    float ratio{0.5f};
    float pos{0.0f};

    std::partial_ordering operator<=>(const Midpoint& other) const
    {
        if (auto cmp = ratio <=> other.ratio; cmp != 0) return cmp;
        return pos <=> other.pos;
    }

    bool operator==(const Midpoint& other) const
    {
        return ratio == other.ratio && pos == other.pos;
    }
};

struct GradientMarkerData {
    int64_t id{0};
    float pos{0.0f};
    ImVec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    Midpoint midpoint{};

    std::partial_ordering operator<=>(const GradientMarkerData& other) const
    {
        if (auto cmp = id <=> other.id; cmp != 0) return cmp;
        if (auto cmp = pos <=> other.pos; cmp != 0) return cmp;
        if (auto cmp = color.x <=> other.color.x; cmp != 0) return cmp;
        if (auto cmp = color.y <=> other.color.y; cmp != 0) return cmp;
        if (auto cmp = color.z <=> other.color.z; cmp != 0) return cmp;
        if (auto cmp = color.w <=> other.color.w; cmp != 0) return cmp;

        return midpoint <=> other.midpoint;
    }

    bool operator==(const GradientMarkerData& other) const
    {
        return id == other.id &&
               pos == other.pos &&
               color.x == other.color.x && color.y == other.color.y && color.z == other.color.z && color.w == other.color.w &&
               midpoint == other.midpoint;
    }
};

struct AlphaMarkerData {
    int64_t id{0};
    float pos{0.0f};
    Midpoint midpoint{};
    float value{1.0f};

    std::partial_ordering operator<=>(const AlphaMarkerData& other) const
    {
        if (auto cmp = id <=> other.id; cmp != 0) return cmp;
        if (auto cmp = pos <=> other.pos; cmp != 0) return cmp;
        if (auto cmp = value <=> other.value; cmp != 0) return cmp;
        return midpoint <=> other.midpoint;
    }

    bool operator==(const AlphaMarkerData& other) const
    {
        return pos == other.pos && value == other.value && id == other.id && midpoint == other.midpoint;
    }
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
    ImVec4 value{};
    Midpoint midpoint{};

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

public:
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
        if (idx == -1) {};
        return m_markers[idx];
    }

    [[nodiscard]] ImVec4 getSelectedMarkerValue() const
    {
        auto idx = getMarkerIndexById(m_state.selected_marker_id);
        if (idx == -1) {};
        return m_markers[idx].value;
    }

    [[nodiscard]] float getSelectedMarkerPosition() const
    {
        auto idx = getMarkerIndexById(m_state.selected_marker_id);
        if (idx == -1) {};
        return m_markers[idx].pos;
    }

    [[nodiscard]] float getSelectedMidpointRatio() const
    {
        auto idx = getMarkerIndexById(m_state.selected_marker_id);
        if (idx == -1) {};
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

    void setMarkerPosision(const int64_t id, const float pos)
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

    //
    // 操作
    //
    void changeMarkerNum(const int64_t marker_num);
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


class GradientMarkerManager {
private:
    uint32_t m_marker_max_count = 30;

    struct Config {
        uint32_t marker_width{20};
        uint32_t marker_height{20};
        uint32_t triangle_height{10};
        uint32_t midpoint_width{20};
    } m_config;

    struct Regions {
        ImVec2 midpoint_p0{}, midpoint_p1{};
        ImVec2 alpha_midpoint_p0{}, alpha_midpoint_p1{};
        ImVec2 gradient_p0{}, gradient_p1{};
        ImVec2 marker_p0{}, marker_p1{};
        ImVec2 alpha_marker_p0{}, alpha_marker_p1{};
    } m_regions;

    struct State {
        int32_t selected_marker_id{0};
        int32_t selected_alpha_marker_id{0};

        int32_t selected_midpoint_id{0};
        int32_t selected_alpha_midpoint_id{0};

        int32_t clicked_marker_id{-4};  // Region::OutSide
        int32_t clicked_alpha_marker_id{-4};

        int32_t clicked_midpoint_id{-4};
        int32_t clicked_alpha_midpoint_id{-4};

        int32_t marker_id_counter{2};
        int32_t alpha_marker_id_counter{2};
        bool is_marker_added{false};
        bool is_alpha_marker_added{false};
        ImVec4 picker_cur_color{1.0f, 1.0f, 1.0f, 1.0f};
        ImVec4 picker_backup_color{1.0f, 1.0f, 1.0f, 1.0f};
        float cur_alpha_value{1.0f};
        bool is_open_popup = false;
    } m_state;

    std::vector<GradientMarkerData> m_markers = {
        {.id = 0, .pos = 0.0f, .color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f), .midpoint = {.ratio = 0.5f, .pos = 0.5}},
        {.id = 1, .pos = 1.0f, .color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), .midpoint = {.ratio = 0.5f, .pos = FLT_MIN}}};


    std::vector<AlphaMarkerData> m_alpha_markers = {
        {.id = 0, .pos = 0.0f, .midpoint = {.ratio = 0.5f, .pos = 0.5}, .value = 1.0f},
        {.id = 1, .pos = 1.0f, .midpoint = {.ratio = 0.5f, .pos = 0.5}, .value = 1.0f}
    };

    enum class Region : int32_t {
        Marker   = -1,
        Midpoint = -2,
        Gradient = -3,
        OutSide  = -4,
        AlphaMarker = -5,
        AlphaMidpoint = -6,
    };

    bool m_io_enable = true;

public:
    GradientMarkerManager()
    {
    }

    std::partial_ordering operator<=>(const GradientMarkerManager& other) const
    {
        return m_markers <=> other.m_markers;
    }

    bool operator==(const GradientMarkerManager& other) const
    {
        return m_markers == other.m_markers;
    }

    //
    // ゲッター
    //
    [[nodiscard]] uint32_t getMarkerWidth() const noexcept { return m_config.marker_width; }
    [[nodiscard]] uint32_t getMarkerRegionHeight() const noexcept { return m_config.marker_height + m_config.triangle_height; }
    [[nodiscard]] uint32_t getMidpointHeight() const noexcept { return m_config.midpoint_width; }
    [[nodiscard]] ImVec2 getGradientRegionP0() const noexcept { return m_regions.gradient_p0; };
    [[nodiscard]] int32_t getIndexById(const int32_t id) const;
    [[nodiscard]] int32_t getAlphaIndexById(const int32_t id) const;
    [[nodiscard]] int32_t getIdByIndex(const uint32_t index) const;
    [[nodiscard]] int32_t getAlphaIdByIndex(const uint32_t index) const;
    [[nodiscard]] float getMarkerPosFromMousePos(const ImVec2& mouse_pos) const;

    [[nodiscard]] const std::vector<GradientMarkerData>& getMarkers() const noexcept { return m_markers; }
    [[nodiscard]] const std::vector<AlphaMarkerData>& getAlphaMarkers() const noexcept { return m_alpha_markers;}
    [[nodiscard]] std::vector<float> getMarkerPos() const;
    [[nodiscard]] std::vector<ImVec4> getMarkerColors() const;
    [[nodiscard]] std::vector<float> getAlphaMarkerValues() const;
    [[nodiscard]] std::vector<float> getAlphaMarkerPos() const;
    [[nodiscard]] std::vector<float> getMidpointRatios() const;
    [[nodiscard]] std::vector<float> getAlphaMidpointRatios() const;

    [[nodiscard]] float getMarkerPos(const int32_t id) const;
    [[nodiscard]] ImVec4 getMarkerColor(const int32_t id) const;
    [[nodiscard]] float getMidpointRatio(const int32_t id) const;
    [[nodiscard]] float getAlphaMarkerValue(const int32_t id) const;
    [[nodiscard]] float getAlphaMidpointRatio(const int32_t id) const;

    [[nodiscard]] float getSelectedMarkerPos() const;
    [[nodiscard]] ImVec4 getSelectedMarkerColor() const;
    [[nodiscard]] float getSelectedMidpointRatio() const;
    [[nodiscard]] float getSelectedAlphaMidpointRatio() const;
    [[nodiscard]] float getSelectedAlphaMarkerValue() const;
    [[nodiscard]] float getSelectedAlphaMarkerPos() const;
    [[nodiscard]] int32_t getSelectedMarkerId() const noexcept { return m_state.selected_marker_id; }
    [[nodiscard]] int32_t getSelectedAlphaMarkerId() const noexcept { return m_state.selected_alpha_marker_id; }
    [[nodiscard]] int32_t getSelectedMidpointId() const noexcept { return m_state.selected_midpoint_id; }

    [[nodiscard]] int32_t getMarkerIdUnderMouse(const ImVec2& mouse_pos) const;
    [[nodiscard]] int32_t getAlphaMarkerIdUnderMouse(const ImVec2& mouse_pos) const;
    [[nodiscard]] int32_t getMidpointIdUnderMouse(const ImVec2& mouse_pos) const;
    [[nodiscard]] int32_t getAlphaMidpointIdUnderMouse(const ImVec2& mouse_pos) const;
    [[nodiscard]] ImVec2 getMousePosOnGradient(const ImVec2& mouse_pos) const;

    [[nodiscard]] ImVec4 getColorPickerColor() const noexcept { return m_state.picker_cur_color; }
    bool isMarkerAdded() const noexcept { return m_state.is_marker_added; }
    bool isOpenPopup() const noexcept { return m_state.is_open_popup; }

    //
    // セッター
    //
    void setMarkerMaxCount(const uint32_t max_count) noexcept { m_marker_max_count = max_count; }
    void setMarkerWidth(const uint32_t width) noexcept
    {
        m_config.marker_width    = width;
        m_config.marker_height   = width;
        m_config.midpoint_width  = width;
        m_config.triangle_height = static_cast<uint32_t>(width * 0.5f);
    }

    void setMarkerPos(const int32_t id, const float pos);
    void setAlphaMarkerPos(const int32_t id, const float pos);
    void setMarkerColor(const int32_t id, const ImVec4& color);
    void setAlphaMarkerValue(const int32_t id, const float value);
    void setMidpointRatio(const int32_t id, const float ratio);
    void setAlphaMidpointRatio(const int32_t id, const float ratio);

    void setSelectedMarkerPos(const float pos);
    void setSelectedAlphaMarkerPos(const float pos);
    void setSelectedMarkerColor(const ImVec4& color);
    void setSelectedAlphaMarkerValue(const float value);
    void setSelectedMidpointRatio(const float ratio);
    void setSelectedAlphaMidpointRatio(const float ratio);

    void setMidpointRegion(const ImVec2& p0, const ImVec2& p1) noexcept;
    void setAlphaMidpointRegion(const ImVec2& p0, const ImVec2& p1) noexcept;
    void setGradientRegion(const ImVec2& p0, const ImVec2& p1) noexcept;
    void setMarkerRegion(const ImVec2& p0, const ImVec2& p1) noexcept;
    void setAlphaMarkerRegion(const ImVec2& p0, const ImVec2& p1) noexcept;

    void setMarkerColorPickerColor(const ImVec4& color) noexcept { m_state.picker_cur_color = color; }
    void setBackupPickerColor(const ImVec4& color) noexcept { m_state.picker_backup_color = color; }
    void setIOEnable(const bool enable) noexcept { m_io_enable = enable; }

    //
    // 操作
    //
    void changeMarkerCount(const uint32_t marker_count);
    void changeAlphaMarkerCount(const uint32_t alpha_marker_count);
    void setDefaultMarkers(const std::vector<GradientMarkerData>& marker_data = {
                               {.id = 0, .pos = 0.0f, .color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f), .midpoint = {.ratio = 0.5f, .pos = 0.5}},
                               {.id = 1, .pos = 1.0f, .color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), .midpoint = {.ratio = 0.5f, .pos = FLT_MIN}}});
    void setDefaultAlphaMarkers(const std::vector<AlphaMarkerData>& alpha_marker_data = {
                               {.id = 0, .pos = 0.0f, .midpoint = {.ratio = 0.5f, .pos = 0.5}, .value = 1.0f},
                               {.id = 1, .pos = 1.0f, .midpoint = {.ratio = 0.5f, .pos = FLT_MIN}, .value = 1.0f}});

    void moveMarker(const int32_t id, const float new_pos);
    void moveAlphaMarker(const int32_t id, const float new_pos);

    void moveMidpoint(const int32_t id, const float new_pos);
    void moveMidpointRatio(const int32_t id, const float new_ratio);
    void moveAlphaMidpoint(const int32_t id, const float new_pos);
    void moveAlphaMidpointRatio(const int32_t id, const float new_ratio);

    void reverseMarkers();
    void reverseAlphaMarkers();
    void resetMidpoints();
    void resetAlphaMidpoints();
    void sortMarkers();
    void sortAlphaMarkers();
    void sortMarkersById();
    void addMarker(const int32_t id, const float marker_pos, const ImVec4& color, const float midpoint_ratio = 0.5f);
    void addAlphaMarker(const int32_t id, const float marker_pos, const float value, const float midpoint_ratio = 0.5f);
    void changeColor(const int32_t id, const ImVec4& new_color);
    void showColorPickerPopup();
    void showAlphaSliderPopup();
    void deleteMarker(const int32_t id);
    void deleteAlphaMarker(const int32_t id);
    void deleteSelectedMarker();
    void deleteSelectedAlphaMarker();
    void distributeMarkersEvenly();
    void distributeAlphaMarkersEvenly();
    void distributeMarkersAndMipointsEvenly();
    void distributeAlphaMarkersAndAlphaMipointsEvenly();
    void selectNextMarker();
    void selectBackMarker();
    void selectNextAlphaMarker();
    void selectBackAlphaMarker();

    //
    // イベント
    //
    void onClickedMarker(const ImVec2& mouse_pos, bool use_default_action, std::move_only_function<void(void*)> func, void* param);
    void onDoubleClickedMarker(const ImVec2& mouse_pos, bool use_default_action = true, std::move_only_function<void(void*)> func = nullptr, void* param = nullptr);
    void onDoubleClickedAlphaMarker(const ImVec2& mouse_pos, bool use_default_action = true, std::move_only_function<void(void*)> func = nullptr, void* param = nullptr);

    //
    // 更新
    //
    void updateMidpointsPos();
    void updateAlphaMidpointsPos();
    void updateMarkerId();
    void updateMarker(const ImVec2& mouse_pos, const ImVec4& new_marker_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    void updateAlphaMarker(const ImVec2& mouse_pos, const float new_value = 1.0f);
    void updateMidpoint(const ImVec2& mouse_pos);
    void updateAlphaMidpoint(const ImVec2& mouse_pos);

    //
    // 描画
    //
    void drawMarker(const char* label, ImVec2 p0, ImVec2 p1, const ImVec4& color, const int32_t id, const bool is_upward = true) const;
    void drawMarkers() const;
    void drawAlphaMarkers() const;
    void drawMidpoint(ImVec2 p0, ImVec2 p1, const ImVec4& color) const;
    void drawMidpoints() const;
    void drawAlphaMidpoints() const;
    void highlightMarker(ImVec2 p0, ImVec2 p1, const ImVec4& highlight_color, const float thickness = 2.0f, const float offset = 2.0f, const bool is_upward = true) const;
    void highlightMidpoint(ImVec2 p0, ImVec2 p1, const ImVec4& highlight_color) const;
};

#endif  // GRADIENT_MARKER_H
