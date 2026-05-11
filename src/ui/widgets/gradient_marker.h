#ifndef GRADIENT_MARKER_H
#define GRADIENT_MARKER_H

#include <algorithm>
#include <compare>
#include <cstdint>
#include <functional>
#include <iterator>
#include <ranges>
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
    int32_t id{0};
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
    int32_t id{0};
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

using MarkerData = std::variant<GradientMarkerData, AlphaMarkerData>;

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
