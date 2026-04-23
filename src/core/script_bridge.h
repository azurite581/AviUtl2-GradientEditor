#ifndef SCRIPT_BRIDGE_H
#define SCRIPT_BRIDGE_H

#include "logger_wrapper_interface.h"

#include <cstdint>
#include <string>
#include <vector>

#include "gradient_data.h"
#include "imgui.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "aviutl2_sdk/plugin2.h"

class ScriptBridge {
public:
    void setLoggerWrapper(LoggerWrapperInterface* logger_wrapper) noexcept { m_logger_wrapper = logger_wrapper; }

    // スクリプトからグラデーションデータを読み込む
    void loadGradientFromScript(EDIT_SECTION* edit,
                                GradientData& data,
                                const std::wstring& effect_name,
                                int32_t effect_index,
                                int32_t target_move_index);

    // スクリプトへグラデーションデータを反映する
    void applyGradientToScript(EDIT_SECTION* edit,
                               GradientData& data,
                               const std::wstring& effect_name,
                               int32_t effect_index,
                               int32_t target_move_index);

    // 特定の範囲のスクリプトデータをリセットする
    void resetScriptData(EDIT_SECTION* edit,
                         uint32_t start_id,
                         uint32_t end_id,
                         const std::wstring& effect_name,
                         int32_t effect_index,
                         int32_t target_move_index,
                         uint32_t max_marker_count);

    bool isChangedValues(GradientData& data)
    {
        setValues(data);
        if (m_prev_values != m_curr_values) {
            m_prev_values = m_curr_values;
            return true;
        } else {
            return false;
        }
    }

    void update(GradientData& data)
    {
        setValues(data);
        if (m_prev_values != m_curr_values) {
            m_prev_values       = m_curr_values;
            m_is_changed_values = true;
        } else {
            m_is_changed_values = false;
        }
    }

    bool getIsChangedValues() const noexcept { return m_is_changed_values; }

    // 値が変化したかどうか調べるための構造体
    struct Values {
        uint32_t marker_count         = 2;
        ImVec4 selected_color         = {0.0f, 0.0f, 0.0f, 0.0f};
        float selected_marker_pos     = 0.0f;
        float selected_midpoint_ratio = 0.5f;
        float blur_width              = 1.0f;
        uint32_t color_space_index    = 0;
        uint32_t interp_dir_index     = 0;

        static bool equal(const ImVec4& a, const ImVec4& b)
        {
            return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
        }

        bool operator==(const Values& rhs) const
        {
            return marker_count == rhs.marker_count && equal(selected_color, rhs.selected_color) && selected_marker_pos == rhs.selected_marker_pos && selected_midpoint_ratio == rhs.selected_midpoint_ratio && blur_width == rhs.blur_width && color_space_index == rhs.color_space_index && interp_dir_index == rhs.interp_dir_index;
        }

        bool operator!=(const Values& rhs) const
        {
            return !(*this == rhs);
        }
    };

    void setValues(GradientData& data)
    {
        m_curr_values.marker_count            = static_cast<uint32_t>(std::ssize(data.getMarkerManager()->getMarkers()));
        m_curr_values.selected_color          = data.getMarkerManager()->getSelectedMarkerColor();
        m_curr_values.selected_marker_pos     = data.getMarkerManager()->getSelectedMarkerPos();
        m_curr_values.selected_midpoint_ratio = data.getMarkerManager()->getSelectedMidpointRatio();
        m_curr_values.blur_width              = data.getBlurWidth();
        m_curr_values.color_space_index       = data.getColorSpace();
        m_curr_values.interp_dir_index        = data.getInterpDir();
    }
    Values getValues() const noexcept { return m_curr_values; }

private:
    LoggerWrapperInterface* m_logger_wrapper;

    Values m_prev_values;
    Values m_curr_values;

    bool m_is_changed_values = false;
};

#endif
