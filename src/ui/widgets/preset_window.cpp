#include "preset_window.h"

#include <algorithm>
#include <iostream>
#include <ranges>

#include "IconsMaterialSymbols.h"
#include "gradient_widget.h"
#include "imgui.h"
#include "preset_controller.h"

namespace gradient_editor {
void PresetWindow::render(bool* is_open, PresetManager& manager, preset_file::GradientPresetFile& file)
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("PresetWindow", is_open, window_flags);

    float input_width{ImGui::GetContentRegionAvail().x};
    ImGuiStyle& style = ImGui::GetStyle();
    input_width -= ImGui::GetFrameHeight();
    input_width -= ImGui::GetFrameHeight();
    input_width -= style.ItemSpacing.x;
    input_width = (input_width < 0.0f) ? 0.0f : input_width;

    ImGui::SetNextItemWidth(input_width);

    // プリセット名入力欄
    ImGui::InputText("##PresetName", m_preset_name, IM_ARRAYSIZE(m_preset_name), ImGuiInputTextFlags_CharsNoBlank);
    ImGui::SameLine();

    // 上書きボタン
    ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, (ImGui::GetFrameHeight() - ImGui::CalcTextSize(ICON_MS_SAVE).x) * 0.5f);
    bool exist_same_preset_name = false;
    for (const auto& p : file.presets) {
        if (p.name == m_preset_name) {
            exist_same_preset_name = true;
            break;
        }
    }

    if (ImGui::Button(ICON_MS_SAVE "##overwrite")) {
        ImGui::OpenPopup((m_config_wrapper->tr(L"上書き保存") + "###Overwrite confirmation").c_str());
    }

    // 上書き確認ダイアログ
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGuiWindowFlags modal_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
    if (ImGui::BeginPopupModal((m_config_wrapper->tr(L"上書き保存") + "###Overwrite confirmation").c_str(), nullptr, modal_flags)) {
        std::string replace_preset_name = file.presets[m_selected_preset_index].name;
        ImGui::Text(m_config_wrapper->tr(L"プリセット \"%s\" を現在のグラデーションで \"%s\" として上書きしますか?").c_str(), replace_preset_name.c_str(), m_preset_name);
        ImGui::Separator();

        // 中央に配置
        float btn_width = 120 * 2 + ImGui::GetStyle().ItemSpacing.x * 1;
        float avail     = ImGui::GetContentRegionAvail().x;
        float off       = (avail - btn_width) * 0.5f;
        if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

        if (ImGui::Button((m_config_wrapper->tr(L"はい") + "###Yes").c_str(), ImVec2(120, 0))) {
            auto preset = PresetController::gradient2preset(m_target_gradient_data);
            PresetController::overwritePreset(manager, file, preset, m_preset_name, m_selected_preset_index);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button((m_config_wrapper->tr(L"いいえ") + "###No").c_str(), ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
        ImGui::SetTooltip(m_config_wrapper->tr(L"上書き保存").c_str(), ImGui::GetStyle().HoverDelayNormal);
    }
    ImGui::SameLine(0.0f, 0.0f);

    // 新規保存ボタン
    ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, (ImGui::GetFrameHeight() - ImGui::CalcTextSize(ICON_MS_LIBRARY_ADD).x) * 0.5f);
    // if (exist_same_preset_name) ImGui::BeginDisabled(true);
    if (ImGui::Button(ICON_MS_LIBRARY_ADD "##add")) {
        auto preset = PresetController::gradient2preset(m_target_gradient_data);
        PresetController::addPreset(manager, file, preset, m_preset_name);
    }
    // if (exist_same_preset_name) ImGui::EndDisabled();
    ImGui::PopStyleVar();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
        ImGui::SetTooltip(m_config_wrapper->tr(L"新規保存").c_str(), ImGui::GetStyle().HoverDelayNormal);
    }

    m_is_clicked_preset = false;
    // プリセット一覧を描画
    renderPresetList(manager, file);

    ImGui::End();
}

void PresetWindow::renderPresetList(PresetManager& manager, preset_file::GradientPresetFile& file)
{
    bool is_delete        = false;
    uint32_t delete_index = 0;
    for (const auto& [i, preset] : file.presets | std::views::enumerate) {
        ImGui::PushID(static_cast<int>(i));

        // プリセットからグラデーションのデータを得る
        gradient_editor::GradientData gradient = PresetController::preset2gradient(preset);

        // プリセットを描画
        ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FrameBorderSize, ImGui::GetStyle().FrameBorderSize));
        ImVec2 gradient_size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * 1.5f);

        // プリセットが押されたとき または初回のみ
        if (CustomUI::drawGradientButton(preset.name, gradient_size, gradient) || (!m_is_init)) {
            m_is_clicked_preset     = true;
            m_selected_preset_index = static_cast<uint32_t>(i);                              // 選択中のインデックスを更新
            m_selected_gradient     = gradient;                                              // 選択中のグラデーションを更新
            std::snprintf(m_preset_name, sizeof(m_preset_name), "%s", preset.name.c_str());  // プリセット名を更新
            if (!m_is_init) m_is_init = true;
        }

        if (m_is_clicked_preset) {
        }

        ImGui::PopStyleVar(2);

        // 名前表示
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone)) {
            ImGui::SetTooltip(preset.name.c_str());
        }

        // 右クリックメニュー
        if (ImGui::BeginPopupContextItem()) {
            // 削除メニュー
            if (ImGui::Selectable(m_config_wrapper->tr(L"削除").c_str())) {
                is_delete    = true;
                delete_index = static_cast<uint32_t>(i);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // この要素がドラッグ開始された場合
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip)) {
            ImGui::SetDragDropPayload("GRADIENT_PRESET", &i, sizeof(uint32_t));
            ImGui::EndDragDropSource();
        }

        // この要素の上にドラッグ中のカーソルがあるか
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GRADIENT_PRESET")) {
                // データの整合性チェック
                IM_ASSERT(payload->DataSize == sizeof(uint32_t));

                // ドラッグ元のインデックスを取り出す
                uint32_t payload_index = *(const uint32_t*)payload->Data;

                PresetController::swapPreset(manager, file, static_cast<uint32_t>(i), payload_index);
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();
    }

    if (is_delete) {
        PresetController::deletePreset(manager, file, delete_index);
    }
}

}  // namespace gradient_editor
