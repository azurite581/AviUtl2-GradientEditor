#include "history_window.h"

#include <chrono>
#include <format>
#include <string>

#include "IconsMaterialSymbols.h"
#include "gradient_widget.h"
#include "imgui_utils.h"

#include "imgui.h"

void HistoryWindow::pushHistory(const GradientData& gradient_data)
{
    auto now       = std::chrono::system_clock::now();
    std::string tz = std::format("{}", std::chrono::zoned_time{std::chrono::current_zone(), now});

    if (!m_history_data.empty()) {
        auto latest_history = m_history_data.back();
        if (latest_history.data != gradient_data) {
            m_history_data.push_back({tz, gradient_data});
            if (static_cast<int32_t>(std::ssize(m_history_data)) > HISTORY_MAX_COUNT) {
                m_history_data.pop_front();
            }
        }
    } else {
        m_history_data.push_back({tz, gradient_data});
    }
}

void HistoryWindow::render(GradientConfigManager& manager, HistoryConfig& cfg)
{
    static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin((m_config_wrapper->tr(L"履歴") + "###history_window").c_str()), nullptr, window_flags) {
        // 履歴を読み込む
        auto loadHistory = [&]() {
            m_history_data.clear();

            for (const auto& history : cfg.histories) {
                m_history_data.push_back({history.name, manager.history2gradient(history)});
                if (static_cast<int32_t>(std::ssize(m_history_data)) > HISTORY_MAX_COUNT) {
                    m_history_data.pop_front();
                }
            }
        };

        // 初回のみ履歴を取得
        if (!m_is_initialized) {
            m_is_initialized = true;
            loadHistory();
        }

        imgui_utils::alignForWidth(ImGui::GetFrameHeight(), 1.0f);  // 右揃え
        if (imgui_utils::squareIconButton(ICON_MS_DELETE_HISTORY, "##delete_history")) {
            manager.deleteHistory(cfg);
            loadHistory();
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip(m_config_wrapper->tr(L"すべての履歴を削除").c_str(), ImGui::GetStyle().HoverDelayNormal);
        }


        int32_t history_num = static_cast<int32_t>(std::ssize(m_history_data));
        if (history_num >= 1) {
            ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FrameBorderSize, ImGui::GetStyle().FrameBorderSize));

            ImVec2 button_size   = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * 1.5f);
            bool is_clicked      = false;
            m_is_history_clicked = false;
            for (int32_t i = history_num - 1; i >= 0; --i) {
                auto history_data = m_history_data[i];

                is_clicked = CustomUI::drawGradientButton(history_data.name, button_size, history_data.data);

                if (is_clicked) {
                    m_is_history_clicked = true;
                    m_selected_gradient  = history_data.data;
                }

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone)) {
                    ImGui::SetTooltip(history_data.name.c_str());
                }
            }
            ImGui::PopStyleVar(2);
        }
    }
    ImGui::End();
}

void HistoryWindow::writeHistoryToConfig(GradientConfigManager& manager, HistoryConfig& cfg)
{
    cfg.histories.clear();

    for (auto history : m_history_data) {
        GradientHistory gradient_history = manager.gradient2history(history.data);
        gradient_history.name = history.name;
        cfg.histories.push_back(gradient_history);
    }

    // 書き込み
    auto result = manager.writeHistory(cfg);
    if (result.is_success) {
        m_logger_wrapper->error("{}", result.error);
    }
}
