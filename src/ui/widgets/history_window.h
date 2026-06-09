#ifndef HISTORY_WINDOW_H
#define HISTORY_WINDOW_H

#include <deque>
#include <iterator>

#include "config2_wrapper_interface.h"
#include "gradient_config.h"
#include "gradient_data.h"
#include "logger_wrapper_interface.h"

class HistoryWindow {
public:
    struct HistoryData {
        std::string name;
        GradientData data;
    };

    HistoryWindow() {}

    [[nodiscard]] HistoryData getHistory(const uint32_t idx) const noexcept { return m_history_data[idx]; }
    [[nodiscard]] GradientData getSelectedGradient() { return m_selected_gradient; }

    void init(LoggerWrapperInterface* logger_wrapper, ConfigWrapperInterface* config_wrapper, GradientConfigManager& manager, History& cfg);
    void render(GradientConfigManager& manager, History& cfg);
    void loadHistories(GradientConfigManager& manager, History& cfg);
    bool isHistoryClicked() const noexcept { return m_is_history_clicked; }
    void pushHistory(const GradientData& gradient_data);
    void writeHistoryToConfig(GradientConfigManager& manager, History& cfg);

    std::deque<HistoryData> m_history_data;

private:
    static constexpr float MODAL_WINDOW_WIDTH  = 120.0f;
    static constexpr int32_t HISTORY_MAX_COUNT = 50;

    // ImGui::GetFrameHeight() を基準としたときの相対スケール
    static constexpr float PRESET_GRADIENT_HEIGHT = 1.25f;
    static constexpr float ITEM_SPACING_SCALE_Y   = 0.25f;

    LoggerWrapperInterface* m_logger_wrapper;
    ConfigWrapperInterface* m_config_wrapper;

    bool m_is_history_clicked = false;
    bool m_is_initialized     = false;
    GradientData m_selected_gradient;
};

#endif
