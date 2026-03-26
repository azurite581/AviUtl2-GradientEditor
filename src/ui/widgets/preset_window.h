#ifndef PRESET_WINDOW_H
#define PRESET_WINDOW_H

#include <unordered_set>

#include "gradient_data.h"
#include "gradient_preset.h"
#include "logger_wrapper_interface.h"
#include "config2_wrapper_interface.h"

namespace gradient_editor {

class PresetWindow {
public:
    PresetWindow() {}

    void setLoggerWrapper(LoggerWrapperInterface* logger_wrapper) noexcept { m_logger_wrapper = logger_wrapper; }
    void setConfigWrapper(ConfigWrapperInterface* config_wrapper) noexcept { m_config_wrapper = config_wrapper; }

    void render(PresetManager& manager, GradientConfig& file);

    bool isClickedPreset() const noexcept { return m_is_clicked_preset; }
    [[nodiscard]] GradientData getSelectedGradientData() const noexcept { return m_selected_gradient; }
    [[nodiscard]] GradientData getTargetGradientData() const noexcept { return m_selected_gradient; }
    void setTargetGradientData(const GradientData& data) noexcept { m_target_gradient_data = data; }

private:
    LoggerWrapperInterface* m_logger_wrapper;
    ConfigWrapperInterface* m_config_wrapper;

    bool m_is_init           = false;
    std::string m_preset_name{};
    bool m_is_clicked_preset = false;
    GradientData m_selected_gradient;
    uint32_t m_selected_preset_index = 0;
    std::vector<std::string> m_categories{};

    GradientData m_target_gradient_data;

    void renderPresetList(PresetManager& manager, GradientConfig& file, std::string_view category);
};

}  // namespace gradient_editor

#endif
