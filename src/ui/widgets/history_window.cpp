#include "history_window.h"
#include "gradient_widget.h"

#include "imgui.h"

#include <format>
#include <string>
#include <chrono>

void HistoryWindow::setHistory(const GradientData& gradient_data)
{
    auto now = std::chrono::system_clock::now();
    std::string tz = std::format("{}", std::chrono::zoned_time{std::chrono::current_zone(), now});

    if (!m_history_data.empty()) {
        auto last_history = m_history_data.back();
        if (last_history.data != gradient_data) {
            m_history_data.push_back({tz, gradient_data});
            if (static_cast<uint32_t>(std::ssize(m_history_data)) > HISTORY_MAX_NUM) {
                m_history_data.pop_front();
            }
        }
    } else {
        m_history_data.push_back({tz, gradient_data});
    }
}

void HistoryWindow::render(PresetManager& manager, GradientConfig& cfg)
{
    ImGui::Begin((m_config_wrapper->tr(L"履歴") + "###history_window").c_str());

    // 履歴を取得
    if (!m_is_initialized) {
        m_is_initialized = true;

        for (const auto& history : cfg.histories) {
            m_history_data.push_back({history.name, manager.history2gradient(history)});
            if (static_cast<uint32_t>(std::ssize(m_history_data)) > HISTORY_MAX_NUM) {
                m_history_data.pop_front();
            }
        }
    }

    int32_t history_num = static_cast<int32_t>(std::ssize(m_history_data));
    if (history_num >= 1) {
        ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FrameBorderSize, ImGui::GetStyle().FrameBorderSize));

        ImVec2 button_size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * 1.5f);
        bool is_clicked = false;
        m_is_history_clicked = false;
        for (int32_t i = history_num - 1; i >= 0; --i) {
            auto history_data = m_history_data[i];

            is_clicked = CustomUI::drawGradientButton(history_data.name, button_size, history_data.data);

            if (is_clicked) {
                m_is_history_clicked = true;
                m_selected_gradient = history_data.data;
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone)) {
                ImGui::SetTooltip(history_data.name.c_str());
            }
        }

        ImGui::PopStyleVar(2);
    }
    ImGui::End();
}
