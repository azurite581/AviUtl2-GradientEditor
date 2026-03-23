#include "preset_controller.h"

#include <cstdint>
#include <format>
#include <iterator>
#include <vector>

#include "color_conv.h"
#include "str_conv.h"

namespace gradient_editor {
GradientData PresetController::preset2gradient(const preset::GradientPreset& preset)
{
    gradient_editor::GradientData gradient{};
    std::vector<GradientMarkerData> markers_data;
    for (uint32_t i = 0; i < static_cast<uint32_t>(std::ssize(preset.colors)); ++i) {
        GradientMarkerData marker_data;
        marker_data.color = color_conv::u32Rgba2Vec4Rgba<ImVec4>(str_conv::charsToInt(preset.colors[i].substr(2, 8), 0xffffffff, 16));
        marker_data.pos   = preset.positions[i];
        if (static_cast<int32_t>(i) < static_cast<int32_t>(std::ssize(preset.colors)) - 1) {
            marker_data.midpoint.ratio = preset.midpoints[i];
        }
        markers_data.push_back(marker_data);
    }
    gradient.m_marker_manager.setDefaultMarkers(markers_data);
    gradient.m_blur_width  = preset.blur_width;
    gradient.m_color_space = preset.color_space;
    gradient.m_interp_dir  = preset.interpolation_path;

    return gradient;
}

preset::GradientPreset PresetController::gradient2preset(gradient_editor::GradientData& gradient)
{
    preset::GradientPreset preset;
    std::vector<std::string> rgba_hex_strs(static_cast<uint32_t>(std::ssize(gradient.m_marker_manager.getMarkerColors())));
    for (const auto& [i, marker_color] : gradient.m_marker_manager.getMarkerColors() | std::views::enumerate) {
        uint32_t rgba    = color_conv::vec4Rgba2u32Rgba<ImVec4>(marker_color);
        rgba_hex_strs[i] = std::format("0x{:08X}", rgba);
    }
    preset.colors             = rgba_hex_strs;
    preset.positions          = gradient.m_marker_manager.getMarkerPos();
    preset.midpoints          = gradient.m_marker_manager.getMidpointRatios();
    preset.blur_width         = gradient.getBlurWidth();
    preset.color_space        = gradient.getColorSpace();
    preset.interpolation_path = gradient.getInterpDir();

    return preset;
}

bool PresetController::deletePreset(PresetManager& manager, GradientConfig& file, const uint32_t index)
{
    // 削除
    file.presets.erase(file.presets.begin() + index);

    // 書き込み
    auto write_result = manager.writePresetFile(file);
    if (!write_result.is_success) {
        return false;
    }
    return true;
}

bool PresetController::swapPreset(PresetManager& manager, GradientConfig& file, const uint32_t index_1, const uint32_t index_2)
{
    // スワップ
    auto tmp              = file.presets[index_1];
    file.presets[index_1] = file.presets[index_2];
    file.presets[index_2] = tmp;

    // 書き込み
    auto write_result = manager.writePresetFile(file);
    if (!write_result.is_success) {
        return false;
    }
    return true;
}

bool PresetController::overwritePreset(PresetManager& manager, GradientConfig& file, preset::GradientPreset preset, const std::string& new_name, const uint32_t index)
{
    preset.name         = new_name;
    file.presets[index] = preset;  // 上書き

    // 書き込み
    auto write_result = manager.writePresetFile(file);
    if (!write_result.is_success) {
        return false;
    }

    // 再読み込み
    auto result = manager.loadPresetFile();
    file        = result.preset_file;
    if (!result.error.empty()) {
        return false;
    }
    return true;
}

bool PresetController::addPreset(PresetManager& manager, GradientConfig& file, preset::GradientPreset preset, const std::string& new_name)
{
    // 追加する前に名前の重複を避ける
    auto has_duplicate_name = [&file](std::string& target_name) {
        for (const auto& p : file.presets) {
            if (p.name == target_name) {
                return true;
            }
        }
        return false;
    };

    std::string original_name  = new_name;
    std::string candidate_name = original_name;
    // 重複がある限りループし続ける
    while (has_duplicate_name(candidate_name)) {
        candidate_name += "_copy";
    }
    preset.name = candidate_name;  // 新しい名前をセット

    file.presets.push_back(preset);  // 追加

    // 書き込み
    auto res = manager.writePresetFile(file);

    // 再読み込み
    auto result = manager.loadPresetFile();
    file        = result.preset_file;
    if (!result.error.empty()) {
        return false;
    }

    return true;
}

bool PresetController::setCategories(PresetManager& manager, GradientConfig& file, const std::vector<std::string>& new_categories)
{
    file.categories = new_categories;

    // 書き込み
    auto write_result = manager.writePresetFile(file);
    if (!write_result.is_success) {
        return false;
    }
    return true;
}

bool PresetController::changeCategory(PresetManager& manager, GradientConfig& file, const uint32_t index, const std::string& new_category)
{
    file.presets[index].category = new_category;

    // 書き込み
    auto write_result = manager.writePresetFile(file);
    if (!write_result.is_success) {
        return false;
    }
    return true;
}

bool PresetController::changeCategories(PresetManager& manager, GradientConfig& file, const std::vector<std::string>& new_categories)
{
    file.categories = new_categories;

    // 書き込み
    auto write_result = manager.writePresetFile(file);
    if (!write_result.is_success) {
        return false;
    }
    return true;
}

}  // namespace gradient_editor
