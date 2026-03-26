#ifndef HISTORY_WINDOW_H
#define HISTORY_WINDOW_H

#include "gradient_data.h"
#include "gradient_preset.h"
#include "logger_wrapper_interface.h"
#include "config2_wrapper_interface.h"

#include <deque>
#include <iterator>

class HistoryWindow {
public:
    HistoryWindow() {}

    void setLoggerWrapper(LoggerWrapperInterface* logger_wrapper) noexcept { m_logger_wrapper = logger_wrapper; }
    void setConfigWrapper(ConfigWrapperInterface* config_wrapper) noexcept { m_config_wrapper = config_wrapper; }

    void render(PresetManager& manager, GradientConfig& cfg);

    struct HistoryData {
        std::string name;
        GradientData data;
    };

    std::deque<HistoryData> m_history_data;

    bool isHistoryClicked() const noexcept { return m_is_history_clicked; }
    void setHistory(const GradientData& gradient_data);

    HistoryData getHistory(const uint32_t idx) const noexcept {
        return m_history_data[idx];
    }

    GradientData getSelectedGradient() {
        return m_selected_gradient;
    }

    void writeHistoryToConfig(PresetManager& manager, GradientConfig& cfg) {

        cfg.histories.clear();
        for (auto h : m_history_data) {
            auto gh = manager.gradient2history(h.data);
            gh.name = h.name;
            cfg.histories.push_back(gh);
        }
    }

private:
    LoggerWrapperInterface* m_logger_wrapper;
    ConfigWrapperInterface* m_config_wrapper;

    static constexpr uint32_t HISTORY_MAX_NUM = 30;

    bool m_is_history_clicked = false;
    GradientData m_selected_gradient;

    bool m_is_initialized = false;




};

#endif
