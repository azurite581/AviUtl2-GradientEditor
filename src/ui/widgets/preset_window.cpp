#include "preset_window.h"

#include <algorithm>
#include <iostream>
#include <ranges>

#include "IconsMaterialSymbols.h"
#include "gradient_widget.h"
#include "imgui.h"
#include "preset_controller.h"
#include "imgui_utils.h"

namespace gradient_editor {
void PresetWindow::render(bool* is_open, PresetManager& manager, GradientConfig& file)
{
    // カテゴリを読み込む
    static bool is_init = true;
    if (is_init) {
        is_init = false;
        for (const auto& [i, category] : file.categories | std::views::enumerate) {
            m_categories.insert(category);
        }
    }

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("PresetWindow", is_open, window_flags);

    static std::string selected_category_name = "default";
    float item_inner_x = ImGui::GetStyle().ItemSpacing.x * 0.5f;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(item_inner_x, 0));
    if (ImGui::BeginTable("##align_table", 2)) {
        float text_width = ImGui::CalcTextSize(m_config_wrapper->tr(L"カテゴリ").c_str()).x;
        ImGui::TableSetupColumn("##text", ImGuiTableColumnFlags_WidthFixed, text_width);
        ImGui::TableSetupColumn("##combo", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(m_config_wrapper->tr(L"カテゴリ").c_str());

        ImGui::TableNextColumn();
        static ImGuiComboFlags combo_flags = 0;
        std::vector<std::string> categories;
        for (const auto& c : m_categories) {
            categories.push_back(c);
        }

        static int category_selected_index = 0;
        const char* combo_preview_value = categories[category_selected_index].c_str();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo("##category", combo_preview_value, combo_flags))
        {
            for (int n = 0; n < std::ssize(categories); n++)
            {
                const bool is_selected = (category_selected_index == n);
                if (ImGui::Selectable(categories[n].c_str(), is_selected))
                    category_selected_index = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        selected_category_name = categories[category_selected_index];

        ImGui::EndTable();
    }
    ImGui::PopStyleVar();

    float input_width{ImGui::GetContentRegionAvail().x};
    ImGuiStyle& style = ImGui::GetStyle();
    input_width -= ImGui::GetFrameHeight();
    input_width -= ImGui::GetFrameHeight();
    input_width -= style.ItemSpacing.x;
    input_width = (input_width < 0.0f) ? 0.0f : input_width;

    ImGui::SetNextItemWidth(input_width);

    // プリセット名入力欄
    ImGui::InputText("##preset_name", m_preset_name, IM_ARRAYSIZE(m_preset_name), ImGuiInputTextFlags_CharsNoBlank);
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
        ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * 0.25f));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * 0.25f));

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
    ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * 0.25f));
    renderPresetList(manager, file, selected_category_name);




    ImGui::End();
}

void PresetWindow::categorySelectorPopup()
{

}


void PresetWindow::renderPresetList(PresetManager& manager, GradientConfig& file, std::string_view category)
{
    bool is_delete        = false;
    uint32_t delete_index = 0;

    // プリセットのデータを1つずつ取り出して描画
    for (const auto& [i, preset] : file.presets | std::views::enumerate) {
        ImGui::PushID(static_cast<int>(i));

        // プリセットからグラデーションのデータを得る
        gradient_editor::GradientData gradient = PresetController::preset2gradient(preset);

        if (preset.category == category) {
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
            bool open_category_popup = false;
            if (ImGui::BeginPopupContextItem()) {
                // カテゴリを変更
                if (ImGui::Button(m_config_wrapper->tr(L"カテゴリを変更").c_str())) {
                    open_category_popup = true;
                    ImGui::CloseCurrentPopup();
                }

                // プリセットから削除
                if (ImGui::Selectable(m_config_wrapper->tr(L"削除").c_str())) {
                    is_delete    = true;
                    delete_index = static_cast<uint32_t>(i);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            static ImGuiTextFilter filter;
            static float input_text_width = ImGui::CalcTextSize((ICON_MS_SEARCH + std::string{" "} + m_config_wrapper->tr(L"カテゴリを検索または作成")).c_str()).x;
            ImGui::SetNextWindowSize({input_text_width, 0});
            if (ImGui::BeginPopup("preset_category_selector")) {
                ImGui::Text(m_config_wrapper->tr(L"カテゴリを変更").c_str());
                ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * 0.25f));

                ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputTextWithHint("##filter",(ICON_MS_SEARCH + std::string{" "} + m_config_wrapper->tr(L"カテゴリを検索または作成")).c_str(), filter.InputBuf, IM_COUNTOF(filter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll)) {
                    filter.Build();
                }
                ImGui::PopItemFlag();
                ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * 0.25f));

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
                static int32_t current = -1;
                std::string input_text = filter.InputBuf;

                // すでに同じ名前のカテゴリがないかチェック
                std::vector<std::string> category_labels(static_cast<uint32_t>(std::ssize(m_categories)));
                std::vector<std::string> category_(static_cast<uint32_t>(std::ssize(m_categories)));
                bool exist_same_name = false;
                for (uint32_t j = 0; const auto& item : m_categories) {
                    if (input_text == item) {
                        exist_same_name = true;
                    }

                    category_[j] = item;
                    // アイコンを追加
                    category_labels[j] = ICON_MS_FOLDER + std::string{" "} + item;
                    ++j;
                }

                if (!exist_same_name && input_text != "") {
                    category_.push_back(input_text);
                    category_labels.push_back(ICON_MS_ADD + std::string{" "} + input_text);
                }

                if (ImGui::BeginListBox("##category_list", ImVec2(-FLT_MIN, 0))) {
                    for (int32_t j = 0; const auto& item : category_labels) {
                        if (filter.PassFilter(item.c_str())) {
                            bool is_selected = (current == j);
                            if (ImGui::Selectable(category_labels[j].c_str(), is_selected)) {
                                current = j;
                            }

                            if (is_selected) {
                                PresetController::setCategories(manager, file, category_);
                                PresetController::changeCategory(manager, file, i, category_[j]);

                                filter.Clear();
                                current = -1;

                                // コンボボックスの要素を再読み込み
                                for (const auto& c : file.categories) {
                                    m_categories.insert(c);
                                }

                                ImGui::CloseCurrentPopup();
                            }
                        }
                        ++j;
                    }
                    ImGui::EndListBox();
                }
                ImGui::PopStyleColor();

                ImGui::EndPopup();
            }

            if (open_category_popup) {
                ImGui::OpenPopup("preset_category_selector");
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
        }

        ImGui::PopID();
    }

    if (is_delete) {
        PresetController::deletePreset(manager, file, delete_index);
    }
}

}  // namespace gradient_editor
