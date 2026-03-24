#ifndef PRESET_CONTROLLER_H
#define PRESET_CONTROLLER_H

#include "gradient_data.h"
#include "gradient_preset.h"

namespace gradient_editor {

class PresetController {
public:
    static gradient_editor::GradientData preset2gradient(const preset::GradientPreset& preset);
    static preset::GradientPreset gradient2preset(gradient_editor::GradientData& gradient);
    static bool deletePreset(PresetManager& manager, GradientConfig& file, const uint32_t index);
    static bool swapPreset(PresetManager& manager, GradientConfig& file, const uint32_t index_1, const uint32_t index_2);
    static bool overwritePreset(PresetManager& manager, GradientConfig& file, preset::GradientPreset preset, const std::string& new_name, const std::string& category, const uint32_t index);
    static bool addPreset(PresetManager& manager, GradientConfig& file, preset::GradientPreset preset, const std::string& new_name, const std::string& category);
    static bool setCategories(PresetManager& manager, GradientConfig& file, const std::vector<std::string>& new_categories);
    static bool deleteCategory(PresetManager& manager, GradientConfig& file, const std::string& category);
    static bool changeCategory(PresetManager& manager, GradientConfig& file, const uint32_t index, const std::string& new_category);
    static bool changeCategories(PresetManager& manager, GradientConfig& file, const std::vector<std::string>& new_categories);
};

}  // namespace gradient_editor

#endif
