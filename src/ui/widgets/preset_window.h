#ifndef PRESET_WINDOW_H
#define PRESET_WINDOW_H

#include <unordered_set>

#include "gradient_data.h"
#include "gradient_preset.h"
#include "config2_wrapper_interface.h"

namespace gradient_editor {

class PresetWindow {
public:
    PresetWindow() {}

    void setConfigWrapper(ConfigWrapperInterface* config_wrapper)
    {
        m_config_wrapper = config_wrapper;
    }

    void render(bool* is_open, PresetManager& manager, GradientConfig& file);

    bool isClickedPreset() const noexcept { return m_is_clicked_preset; }
    [[nodiscard]] gradient_editor::GradientData getSelectedGradientData() const noexcept { return m_selected_gradient; }
    [[nodiscard]] gradient_editor::GradientData getTargetGradientData() const noexcept { return m_selected_gradient; }
    void setTargetGradientData(const gradient_editor::GradientData& data) noexcept { m_target_gradient_data = data; }

private:
    ConfigWrapperInterface* m_config_wrapper;

    bool m_is_init           = false;
    std::string m_preset_name{};
    bool m_is_clicked_preset = false;
    gradient_editor::GradientData m_selected_gradient;
    uint32_t m_selected_preset_index = 0;
    std::vector<std::string> m_categories{};

    gradient_editor::GradientData m_target_gradient_data;

    void renderPresetList(PresetManager& manager, GradientConfig& file, std::string_view category);
};

}  // namespace gradient_editor

#endif
