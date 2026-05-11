#ifndef HISTORY_WINDOW_H
#define HISTORY_WINDOW_H

#include "gradient_data.h"
#include "gradient_config.h"
#include "logger_wrapper_interface.h"
#include "config2_wrapper_interface.h"

#include <deque>
#include <iterator>

class HistoryWindow {
public:
    struct HistoryData {
        std::string name;
        GradientData data;
    };

    HistoryWindow() {}

    void setLoggerWrapper(LoggerWrapperInterface* logger_wrapper) noexcept { m_logger_wrapper = logger_wrapper; }
    void setConfigWrapper(ConfigWrapperInterface* config_wrapper) noexcept { m_config_wrapper = config_wrapper; }

    void render(GradientConfigManager& manager, History& cfg);
    bool isHistoryClicked() const noexcept { return m_is_history_clicked; }
    void pushHistory(const GradientData& gradient_data);
    [[nodiscard]] HistoryData getHistory(const uint32_t idx) const noexcept { return m_history_data[idx]; }
    [[nodiscard]] GradientData getSelectedGradient() { return m_selected_gradient; }
    void writeHistoryToConfig(GradientConfigManager& manager, History& cfg);

    std::deque<HistoryData> m_history_data;

private:
    static constexpr float MODAL_WINDOW_WIDTH   = 120.0f;
    static constexpr int32_t HISTORY_MAX_COUNT = 50;

    // ImGui::GetFrameHeight() を基準としたときの相対スケール
    static constexpr float PRESET_GRADIENT_HEIGHT = 1.25f;
    static constexpr float ITEM_SPACING_SCALE_Y = 0.25f;

    LoggerWrapperInterface* m_logger_wrapper;
    ConfigWrapperInterface* m_config_wrapper;

    bool m_is_history_clicked = false;
    bool m_is_initialized = false;
    GradientData m_selected_gradient;
};

#endif
