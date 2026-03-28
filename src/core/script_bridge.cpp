#include "script_bridge.h"

#include <algorithm>
#include <ranges>

#include "constants.h"
#include "plugin2_utils.h"
#include "str_conv.h"

void ScriptBridge::loadGradientFromScript(EDIT_SECTION* edit,
                                          GradientData& data,
                                          const std::wstring& effect_name,
                                          int32_t effect_index,
                                          int32_t target_move_index)
{
    OBJECT_HANDLE object_handle = edit->get_focus_object();
    if (!object_handle) return;

    // マーカー数
    uint32_t marker_count = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"マーカー数", 2u, target_move_index);
    data.getMarkerManager()->changeMarkerCount(marker_count);

    auto markers = data.getMarkerManager()->getMarkers();

    // 位置
    for (const auto& marker : markers) {
        float marker_pos = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"位置" + str_conv::intToWchars(marker.id + 1, "1")).c_str(), 0.0f, target_move_index);
        data.getMarkerManager()->setMarkerPos(marker.id, marker_pos / 100.0f);
    }

    // 色と透明度
    for (const auto& marker : markers) {
        uint32_t hex_rgb = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"色" + str_conv::intToWchars(marker.id + 1, "1")).c_str(), 0xffffff, 0, 16);
        float alpha      = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"透明度" + str_conv::intToWchars(marker.id + 1, "1")).c_str(), 0.0f, target_move_index);

        ImVec4 marker_color;
        marker_color.x = ((hex_rgb >> 16) & 0xFF) / 255.0f;
        marker_color.y = ((hex_rgb >> 8) & 0xFF) / 255.0f;
        marker_color.z = ((hex_rgb >> 0) & 0xFF) / 255.0f;
        marker_color.w = (100.0f - alpha) / 100.0f;
        data.getMarkerManager()->setMarkerColor(marker.id, marker_color);
    }

    // 中間点
    for (size_t i = 0; i < markers.size(); ++i) {
        if (i != markers.size() - 1) {
            float marker_midpoint_ratio = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"中間点" + str_conv::intToWchars(markers[i].id + 1, "1")).c_str(), 0.0f);
            data.getMarkerManager()->setMidpointRatio(markers[i].id, marker_midpoint_ratio / 100.0f);
        }
    }

    // ぼかし幅
    float blur_width = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"ぼかし幅", 100.0f);
    data.setBlurWidth(blur_width / 100.0f);

    // 色空間
    std::string color_space_str = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"色空間", std::string{COLOR_SPACE_NAMES[0]});
    for (uint32_t i = 0; i < 8; ++i) {
        if (color_space_str == COLOR_SPACE_NAMES[i]) {
            data.setColorSpace(i);
            break;
        }
    }

    // 補間経路
    std::string interp_dir_str = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"補間経路", std::string("短経路"));
    for (uint32_t i = 0; i < 2; ++i) {
        if (interp_dir_str == INTERP_DIR_NAMES[i]) {
            data.setInterpDir(i);
            break;
        }
    }

    // 値をセット
    m_curr_values.marker_count            = static_cast<uint32_t>(std::ssize(data.getMarkerManager()->getMarkers()));
    m_curr_values.selected_color          = data.getMarkerManager()->getSelectedMarkerColor();
    m_curr_values.selected_marker_pos     = data.getMarkerManager()->getSelectedMarkerPos();
    m_curr_values.selected_midpoint_ratio = data.getMarkerManager()->getSelectedMidpointRatio();
    m_curr_values.blur_width              = data.getBlurWidth();
    m_curr_values.color_space_index       = data.getColorSpace();
    m_curr_values.interp_dir_index        = data.getInterpDir();
}

void ScriptBridge::applyGradientToScript(EDIT_SECTION* edit,
                                         GradientData& data,
                                         const std::wstring& effect_name,
                                         int32_t effect_index,
                                         int32_t target_move_index)
{
    OBJECT_HANDLE object_handle = edit->get_focus_object();
    if (!object_handle) return;

    auto markers          = data.getMarkerManager()->getMarkers();
    uint32_t marker_count = static_cast<uint32_t>(markers.size());

    // マーカー数
    plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"マーカー数", marker_count, 2u, target_move_index);

    // 各マーカーのデータ
    for (const auto& marker : markers) {
        // 色
        uint32_t r          = static_cast<uint32_t>(marker.color.x * 255.0f + 0.5f);
        uint32_t g          = static_cast<uint32_t>(marker.color.y * 255.0f + 0.5f);
        uint32_t b          = static_cast<uint32_t>(marker.color.z * 255.0f + 0.5f);
        uint32_t marker_rgb = (r << 16) | (g << 8) | b;

        char hex_rgb_buf[8];
        std::snprintf(hex_rgb_buf, sizeof(hex_rgb_buf), "%06x", marker_rgb);

        // 透明度
        float alpha = (1.0f - marker.color.w) * 100.0f;

        std::wstring id_wstr = str_conv::intToWchars(marker.id + 1, "1");
        plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"色" + id_wstr).c_str(), std::string(hex_rgb_buf), std::string("ffffff"));
        plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"透明度" + id_wstr).c_str(), alpha, 0.0f, target_move_index);

        // 位置
        plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"位置" + id_wstr).c_str(), marker.pos * 100.0f, 0.0f, target_move_index);

        // 中間点 (最後のマーカー以外)
        auto it = std::find_if(markers.begin(), markers.end(), [&](const auto& m) { return m.id == marker.id; });
        if (it != markers.end() && std::next(it) != markers.end()) {
            plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"中間点" + id_wstr).c_str(), marker.midpoint.ratio * 100.0f, 50.0f);
        }
    }

    // ぼかし幅
    plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"ぼかし幅", data.getBlurWidth() * 100.0f, 100.0f);

    // 色空間
    int32_t cs_idx = data.getColorSpace();
    if (cs_idx >= 0 && cs_idx < 8) {
        plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"色空間", std::string(COLOR_SPACE_NAMES[cs_idx]), std::string(COLOR_SPACE_NAMES[0]));
    }

    // 補間経路
    int32_t id_idx = data.getInterpDir();
    if (id_idx >= 0 && id_idx < 2) {
        plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"補間経路", std::string(INTERP_DIR_NAMES[id_idx]), std::string(INTERP_DIR_NAMES[0]));
    }
}

void ScriptBridge::resetScriptData(EDIT_SECTION* edit,
                                   uint32_t start_id, uint32_t end_id,
                                   const std::wstring& effect_name,
                                   int32_t effect_index,
                                   int32_t target_move_index,
                                   uint32_t max_marker_count)
{
    if (start_id >= end_id) return;

    OBJECT_HANDLE object_handle = edit->get_focus_object();
    if (!object_handle) return;

    const uint32_t DEFAULT_COLOR = 0xffffff;
    const float DEFAULT_ALPHA    = 0.0f;
    const float DEFAULT_POS      = 0.0f;
    const float DEFAULT_MIDPOINT = 50.0f;

    // start_id ~ end_id までの範囲を初期値にリセットする
    for (uint32_t i = start_id; i < end_id; ++i) {
        std::wstring id_wstr = str_conv::intToWchars(i + 1, "1");
        plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"位置" + id_wstr).c_str(), DEFAULT_POS, DEFAULT_POS, target_move_index);
        plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"色" + id_wstr).c_str(), DEFAULT_COLOR, DEFAULT_COLOR, 0, 16);
        plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"透明度" + id_wstr).c_str(), DEFAULT_ALPHA, DEFAULT_ALPHA, target_move_index);
        if (i < max_marker_count) {
            plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, (L"中間点" + id_wstr).c_str(), DEFAULT_MIDPOINT, DEFAULT_MIDPOINT, target_move_index);
        }
    }
}

