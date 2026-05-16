#include "script_bridge.h"

#include <algorithm>
#include <format>
#include <ranges>

#include "alias_parser.h"
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

    const std::wstring effect_name_with_idx = std::format(L"{}:{}", effect_name, effect_index);

    // マーカー数
    uint32_t marker_count = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"マーカー数", 2u, target_move_index);
    data.getColorMarkers()->changeMarkerCount(marker_count);

    auto markers = data.getColorMarkers()->getMarkers();

    // 位置
    std::string pos_array_str = edit->get_object_item_value(object_handle, effect_name_with_idx.c_str(), L"位置");
    auto pos_result = alias_parser::splitStr<float>(pos_array_str, ",");
    if (!pos_result) {
        m_logger_wrapper->error("Failed to parse 位置: {}", pos_result.error());
    } else {
        for (uint32_t i = 0; i < markers.size(); ++i) {
            data.getColorMarkers()->setMarkerPosition(markers[i].id, pos_result.value()[i]);
        }
    }

    // 中間点
    std::string mid_array_str = edit->get_object_item_value(object_handle, effect_name_with_idx.c_str(), L"中間点");
    auto mid_result = alias_parser::splitStr<float>(mid_array_str, ",");
    if (!mid_result) {
        m_logger_wrapper->error("Failed to parse 中間点: {}", mid_result.error());
    } else {
        for (uint32_t i = 0; i < markers.size() - 1; ++i) {
            data.getColorMarkers()->setMidpointRatio(markers[i].id, mid_result.value()[i]);
        }
    }

    // 色と透明度
    std::string color_array_str = edit->get_object_item_value(object_handle, effect_name_with_idx.c_str(), L"色");
    auto color_result = alias_parser::splitStr<uint32_t>(color_array_str, ",");
    std::string alpha_array_str = edit->get_object_item_value(object_handle, effect_name_with_idx.c_str(), L"色の透明度");
    auto alpha_result = alias_parser::splitStr<float>(alpha_array_str, ",");
    if (!color_result) {
        m_logger_wrapper->error("Failed to parse 色: {}", color_result.error());
    } else if (!alpha_result) {
        m_logger_wrapper->error("Failed to parse 色の透明度: {}", alpha_result.error());
    } else {
        for (uint32_t i = 0; i < markers.size(); ++i) {
            uint32_t hex_rgb = color_result.value()[i];
            float alpha      = alpha_result.value()[i];

            ImVec4 marker_color;
            marker_color.x = ((hex_rgb >> 16) & 0xFF) / 255.0f;
            marker_color.y = ((hex_rgb >> 8) & 0xFF) / 255.0f;
            marker_color.z = ((hex_rgb >> 0) & 0xFF) / 255.0f;
            marker_color.w = 1.0f - alpha;
            data.getColorMarkers()->setMarkerValue(markers[i].id, marker_color);
        }
    }

    // ぼかし幅
    float blur_width = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"ぼかし幅", 100.0f);
    data.setColorBlurWidth(blur_width / 100.0f);

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

    // アルファマーカー数
    uint32_t alpha_marker_count = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"アルファマーカー数", 2u, target_move_index);
    data.getAlphaMarkers()->changeMarkerCount(alpha_marker_count);

    // 位置
    std::string alpha_pos_array_str = edit->get_object_item_value(object_handle, effect_name_with_idx.c_str(), L"アルファ位置");
    auto alpha_pos_result = alias_parser::splitStr<float>(alpha_pos_array_str, ",");
    if (!alpha_pos_result) {
        m_logger_wrapper->error("Failed to parse アルファ位置: {}", alpha_pos_result.error());
    } else {
        for (uint32_t i = 0; i < markers.size(); ++i) {
            data.getAlphaMarkers()->setMarkerPosition(markers[i].id, alpha_pos_result.value()[i]);
        }
    }

    // アルファ値
    std::string alpha_value_array_str = edit->get_object_item_value(object_handle, effect_name_with_idx.c_str(), L"アルファ値");
    auto alpha_value_result = alias_parser::splitStr<float>(alpha_value_array_str, ",");
    if (!alpha_value_result) {
        m_logger_wrapper->error("Failed to parse アルファ値: {}", alpha_value_result.error());
    } else {
        for (uint32_t i = 0; i < markers.size(); ++i) {
            data.getAlphaMarkers()->setMarkerPosition(markers[i].id, alpha_value_result.value()[i]);
        }
    }

    // 中間点
    std::string alpha_mid_array_str = edit->get_object_item_value(object_handle, effect_name_with_idx.c_str(), L"アルファ中間点");
    auto alpha_mid_result = alias_parser::splitStr<float>(alpha_mid_array_str, ",");
    if (!alpha_mid_result) {
        m_logger_wrapper->error("Failed to parse アルファ中間点: {}", alpha_mid_result.error());
    } else {
        for (uint32_t i = 0; i < markers.size(); ++i) {
            data.getAlphaMarkers()->setMidpointRatio(markers[i].id, alpha_mid_result.value()[i]);
        }
    }

    // ぼかし幅
    float alpha_blur_width = plugin2_utils::getObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"アルファぼかし幅", 100.0f);
    data.setAlphaBlurWidth(alpha_blur_width / 100.0f);

    // 比較用の変数に現在の値を保存
    m_curr_values.marker_count            = static_cast<uint32_t>(std::ssize(data.getColorMarkers()->getMarkers()));
    m_curr_values.selected_color          = data.getColorMarkers()->getSelectedMarkerValue();
    m_curr_values.selected_marker_pos     = data.getColorMarkers()->getSelectedMarkerPosition();
    m_curr_values.selected_midpoint_ratio = data.getColorMarkers()->getSelectedMidpointRatio();
    m_curr_values.blur_width              = data.getBlurWidth();
    m_curr_values.color_space_index       = data.getColorSpace();
    m_curr_values.interp_dir_index        = data.getInterpDir();

    m_curr_values.alpha_marker_count      = static_cast<uint32_t>(std::ssize(data.getAlphaMarkers()->getMarkers()));
    m_curr_values.selected_alpha_marker_pos = data.getAlphaMarkers()->getSelectedMarkerPosition();
    m_curr_values.selected_alpha_marker_value = data.getAlphaMarkers()->getSelectedMarkerPosition();
    m_curr_values.selected_alpha_midpoint_ratio = data.getAlphaMarkers()->getSelectedMidpointRatio();
}

void ScriptBridge::applyGradientToScript(EDIT_SECTION* edit,
                                         GradientData& data,
                                         const std::wstring& effect_name,
                                         int32_t effect_index,
                                         int32_t target_move_index)
{
    OBJECT_HANDLE object_handle = edit->get_focus_object();
    if (!object_handle) return;

    const std::wstring effect_name_with_idx = std::format(L"{}:{}", effect_name, effect_index);

    // マーカー数
    auto markers          = data.getColorMarkers()->getMarkers();
    uint32_t marker_count = static_cast<uint32_t>(markers.size());
    plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"マーカー数", marker_count, 2u, target_move_index);

    // 色、透明度、位置、中間点
    std::string col_array_str, alpha_array_str, pos_array_str, mid_array_str;
    for (uint32_t i = 0; i < marker_count; ++i) {
        uint32_t r          = static_cast<uint32_t>(markers[i].value.x * 255.0f + 0.5f);
        uint32_t g          = static_cast<uint32_t>(markers[i].value.y * 255.0f + 0.5f);
        uint32_t b          = static_cast<uint32_t>(markers[i].value.z * 255.0f + 0.5f);
        uint32_t marker_rgb = (r << 16) | (g << 8) | b;

        col_array_str += std::format("0x{:06x}", marker_rgb);
        alpha_array_str += std::format("{:.2f}", 1.0f - (markers[i].value.w));
        pos_array_str += std::format("{:.2f}", markers[i].pos);

        if (i != marker_count - 1) {
            mid_array_str += std::format("{:.2f}", markers[i].midpoint.ratio);

            col_array_str += ",";
            alpha_array_str += ",";
            pos_array_str += ",";
            if (i != marker_count - 2) {
                mid_array_str += ",";
            }
        }
    }
    edit->set_object_item_value(object_handle, effect_name_with_idx.c_str(), L"色", col_array_str.c_str());
    edit->set_object_item_value(object_handle, effect_name_with_idx.c_str(), L"色の透明度", alpha_array_str.c_str());
    edit->set_object_item_value(object_handle, effect_name_with_idx.c_str(), L"位置", pos_array_str.c_str());
    edit->set_object_item_value(object_handle, effect_name_with_idx.c_str(), L"中間点", mid_array_str.c_str());

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

    // アルファマーカー数
    auto alpha_markers = data.getAlphaMarkers()->getMarkers();
    uint32_t alpha_marker_count = static_cast<uint32_t>(alpha_markers.size());
    edit->set_object_item_value(object_handle, effect_name_with_idx.c_str(), L"アルファマーカー数", std::to_string(alpha_marker_count).c_str());

    // アルファ、位置、中間点
    std::string alpha_pos_array_str, alpha_value_array_str, alpha_mid_array_str;
    for (uint32_t i = 0; i < alpha_marker_count; ++i) {
        alpha_value_array_str += std::format("{:.2f}", alpha_markers[i].value.w);
        alpha_pos_array_str += std::format("{:.2f}", alpha_markers[i].pos);
        if (i != alpha_marker_count - 1) {
            alpha_mid_array_str += std::format("{:.2f}", alpha_markers[i].midpoint.ratio);

            alpha_value_array_str += ",";
            alpha_pos_array_str += ",";
            if (i != alpha_marker_count - 2) {
                alpha_mid_array_str += ",";
            }
        }
    }
    edit->set_object_item_value(object_handle, effect_name_with_idx.c_str(), L"アルファ位置", alpha_pos_array_str.c_str());
    edit->set_object_item_value(object_handle, effect_name_with_idx.c_str(), L"アルファ値", alpha_value_array_str.c_str());
    edit->set_object_item_value(object_handle, effect_name_with_idx.c_str(), L"アルファ中間点", alpha_mid_array_str.c_str());

    // ぼかし幅
    plugin2_utils::setObjectItemValue(edit, object_handle, effect_name.c_str(), effect_index, L"アルファぼかし幅", data.getAlphaBlurWidth() * 100.0f, 100.0f);
}
