#ifndef PRESET_WINDOW_H
#define PRESET_WINDOW_H

#include <unordered_set>

#include "config2_wrapper_interface.h"
#include "gradient_config.h"
#include "gradient_data.h"
#include "logger_wrapper_interface.h"

class PresetWindow {
public:
    PresetWindow() {}

    void setLoggerWrapper(LoggerWrapperInterface* logger_wrapper) noexcept { m_logger_wrapper = logger_wrapper; }
    void setConfigWrapper(ConfigWrapperInterface* config_wrapper) noexcept { m_config_wrapper = config_wrapper; }

    void render(GradientConfigManager& manager, PresetConfig& file);

    bool isPresetClicked() const noexcept { return m_is_clicked_preset; }
    [[nodiscard]] GradientData getSelectedGradientData() const noexcept { return m_selected_gradient; }
    [[nodiscard]] GradientData getTargetGradientData() const noexcept { return m_selected_gradient; }
    void setTargetGradientData(const GradientData& data) noexcept { m_target_gradient_data = data; }
    void overwriteCatogories(GradientConfigManager& manager, PresetConfig& cfg);

private:
    static constexpr float MODAL_WINDOW_WIDTH   = 120.0f;
    static constexpr float ITEM_SPACING_SCALE_Y = 0.25f;

    LoggerWrapperInterface* m_logger_wrapper;
    ConfigWrapperInterface* m_config_wrapper;

    std::string m_preset_name{};
    std::string m_old_category_name{};
    bool m_is_initialized{false};
    bool m_is_clicked_preset{false};
    GradientData m_selected_gradient;
    int32_t m_selected_preset_index{-1};  // -1 == 未選択
    int32_t m_selected_category_index{0};
    std::vector<std::string> m_categories{};

    GradientData m_target_gradient_data;

    void renderPresetList(GradientConfigManager& manager, PresetConfig& file, std::string_view category);
};

#endif
