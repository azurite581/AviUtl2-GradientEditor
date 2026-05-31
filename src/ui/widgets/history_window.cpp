#include "history_window.h"

#include <chrono>
#include <format>
#include <string>

#include "IconsMaterialSymbols.h"
#include "gradient_widget.h"
#include "imgui.h"
#include "imgui_utils.h"

void HistoryWindow::pushHistory(const GradientData& gradient_data)
{
    auto now       = std::chrono::system_clock::now();
    std::string tz = std::format("{}", std::chrono::zoned_time{std::chrono::current_zone(), now});

    if (!m_history_data.empty()) {
        auto latest_history = m_history_data.back();
        // グラデーションデータが最後にプッシュされた履歴と同じかどうか比較し、異なれば追加する
        // そのため GradientData クラスで同値比較演算子（==, !=）を定義しておくこと
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

void HistoryWindow::render(GradientConfigManager& manager, History& cfg)
{
    static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    bool history_window_visible = ImGui::Begin((m_config_wrapper->tr(L"履歴") + "###history_window").c_str(), nullptr, window_flags);
    if (history_window_visible) {
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

        // 初回のみ履歴を読み込む
        if (!m_is_initialized) {
            m_is_initialized = true;
            loadHistory();
        }

        imgui_utils::alignForWidth(ImGui::GetFrameHeight(), 1.0f);  // 右揃え
        if (imgui_utils::squareIconButton(ICON_MS_DELETE_HISTORY, "##delete_history")) {
            ImGui::OpenPopup((m_config_wrapper->tr(L"すべての履歴を削除") + "###delete_history").c_str());
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip(m_config_wrapper->tr(L"すべての履歴を削除").c_str(), ImGui::GetStyle().HoverDelayNormal);
        }

        // 履歴削除確認ポップアップ
        static ImGuiWindowFlags modal_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
        ImVec2 center                       = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal((m_config_wrapper->tr(L"すべての履歴を削除") + "###delete_history").c_str(), nullptr, modal_flags)) {
            ImGui::TextUnformatted(m_config_wrapper->tr(L"すべての履歴を削除しますか?").c_str());
            ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * ITEM_SPACING_SCALE_Y));

            float button_width          = ImGui::GetFrameHeight() * 4;
            float combined_button_width = button_width * 2 + ImGui::GetStyle().ItemSpacing.x * 1;
            float avail                 = ImGui::GetContentRegionAvail().x;
            float off                   = (avail - combined_button_width) * 0.5f;
            if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

            if (ImGui::Button((m_config_wrapper->tr(L"キャンセル") + "###cancel").c_str(), ImVec2(button_width, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();

            if (ImGui::Button((m_config_wrapper->tr(L"削除") + "###delete_history").c_str(), ImVec2(button_width, 0))) {
                manager.deleteHistory(cfg);
                loadHistory();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        int32_t history_num = static_cast<int32_t>(std::ssize(m_history_data));

        if (history_num >= 1) {
            ImVec2 button_size   = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * PRESET_GRADIENT_HEIGHT);
            button_size.x        = ImMax(1.0f, button_size.x);
            button_size.y        = ImMax(1.0f, button_size.y);
            m_is_history_clicked = false;

            // 履歴をレンダリング
            for (int32_t i = history_num - 1; i >= 0; --i) {
                auto history_data = m_history_data[i];

                ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FrameBorderSize, ImGui::GetStyle().FrameBorderSize));

                if (custom_ui::drawGradientButton(history_data.name, button_size, history_data.data)) {
                    m_is_history_clicked = true;
                    m_selected_gradient  = history_data.data;
                }

                ImGui::PopStyleVar(2);

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone)) {
                    ImGui::SetTooltip("%s", history_data.name.c_str());
                }
            }
        }
    }
    ImGui::End();
}

void HistoryWindow::writeHistoryToConfig(GradientConfigManager& manager, History& cfg)
{
    cfg.histories.clear();

    // 履歴をコンフィグにセット
    for (auto history : m_history_data) {
        auto gradient_history = manager.gradient2history(history.data);
        gradient_history.name = history.name;
        cfg.histories.push_back(gradient_history);
    }

    // 書き込み
    auto result = manager.writeHistory(cfg);
    if (!result.is_success) {
        m_logger_wrapper->error("{}", result.error);
    }
}
