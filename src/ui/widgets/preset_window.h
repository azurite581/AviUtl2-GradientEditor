#ifndef PRESET_WINDOW_H
#define PRESET_WINDOW_H

#include "config2_wrapper_interface.h"
#include "gradient_config.h"
#include "gradient_data.h"
#include "logger_wrapper_interface.h"

class PresetWindow {
public:
    PresetWindow() {}

    bool isPresetClicked() const noexcept { return m_is_clicked_preset; }
    void setCategories(std::vector<std::string>& categories) noexcept { m_categories = categories; }
    void setSelectedCategoryIndex(const int32_t index) noexcept { m_selected_category_index = index; }
    void setTargetGradientData(const GradientData& data) noexcept { m_target_gradient_data = data; }
    [[nodiscard]] GradientData getSelectedGradientData() const noexcept { return m_selected_gradient; }
    [[nodiscard]] GradientData getTargetGradientData() const noexcept { return m_selected_gradient; }

    void init(LoggerWrapperInterface* logger_wrapper, ConfigWrapperInterface* config_wrapper, GradientConfigManager& manager, Preset& cfg);
    void render(GradientConfigManager& manager, Preset& file);
    void loadCategories(Preset& cfg);
    void writeSelectedCategoryToConfig(GradientConfigManager& manager, Preset& cfg);

private:
    static constexpr float MODAL_WINDOW_WIDTH = 120.0f;

    // ImGui::GetFrameHeight() を基準としたときの相対スケール
    static constexpr float PRESET_GRADIENT_HEIGHT = 1.25f;
    static constexpr float ITEM_SPACING_SCALE_Y   = 0.25f;

    LoggerWrapperInterface* m_logger_wrapper;
    ConfigWrapperInterface* m_config_wrapper;

    std::string m_preset_name{};
    std::string m_selected_category_name{};
    std::string m_old_category_name{};
    std::string m_old_preset_name{};
    bool m_is_clicked_preset{false};
    GradientData m_selected_gradient;
    int32_t m_selected_preset_index{-1};  // -1 == 未選択
    int32_t m_selected_category_index{0};
    uint32_t m_delete_index{0};
    std::vector<std::string> m_categories{};

    GradientData m_target_gradient_data;

    void renderPresetList(GradientConfigManager& manager, Preset& file, std::string_view category);
};

#endif
