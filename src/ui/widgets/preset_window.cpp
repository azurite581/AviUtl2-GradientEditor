#include "preset_window.h"

#include <cstdio>
#include <algorithm>
#include <iostream>
#include <ranges>

#include "IconsMaterialSymbols.h"
#include "gradient_widget.h"
#include "imgui.h"
#include "imgui_utils.h"

#include "misc/cpp/imgui_stdlib.h"

namespace gradient_editor {
void PresetWindow::render(bool* is_open, PresetManager& manager, GradientConfig& file)
{
    static std::string selected_category_name = "uncategorized";

    auto loadCategories = [&]() {
        m_categories.clear();

        std::unordered_set<std::string> tmp;
        if (!file.categories.empty()) {
            for (const auto& [i, category] : file.categories | std::views::enumerate) {
                tmp.insert(category);
            }
        } else {
            tmp.insert("uncategorized");
        }

        for (const auto& e : tmp) m_categories.push_back(e);
    };

    // カテゴリを読み込む
    static bool is_init = true;
    if (is_init) {
        is_init = false;
        loadCategories();
    }

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("PresetWindow", is_open, window_flags);

    float item_inner_x = ImGui::GetStyle().ItemSpacing.x * 0.5f;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(item_inner_x, 0));
    bool is_called_popup = false;

    if (ImGui::BeginTable("##align_table1", 3)) {
        float text_width = ImGui::CalcTextSize(m_config_wrapper->tr(L"カテゴリ").c_str()).x;
        ImGui::TableSetupColumn("##text", ImGuiTableColumnFlags_WidthFixed, text_width);
        ImGui::TableSetupColumn("##combo", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##button", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());

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

        static int32_t category_selected_index = 0;
        const char* combo_preview_value = categories[category_selected_index].c_str();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo("##category_combo", combo_preview_value, combo_flags)) {
            for (int n = 0; n < std::ssize(categories); n++) {
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

        ImGui::TableNextColumn();
        is_called_popup = imgui_utils::squareIconButton(ICON_MS_MENU, "##category_menu");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
            ImGui::SetTooltip(m_config_wrapper->tr(L"カテゴリを編集").c_str(), ImGui::GetStyle().HoverDelayNormal);
        }

        ImGui::EndTable();
    }
    ImGui::PopStyleVar();

    if (is_called_popup) {
        ImGui::OpenPopup("preset_category_editor");
    }

    //
    // カテゴリ編集ポップアップ
    //
    std::string input_placeholder = (ICON_MS_SEARCH + std::string{" "} + m_config_wrapper->tr(L"カテゴリーを検索または作成"));
    float input_placeholder_width = ImGui::CalcTextSize(input_placeholder.c_str()).x;
    ImGui::SetNextWindowSize({input_placeholder_width + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetStyle().WindowPadding.x * 2.0f, 0});

    ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, ImGui::GetFrameHeight() * 0.25f);
    if (ImGui::BeginPopup("preset_category_editor")) {
        ImGui::Text(m_config_wrapper->tr(L"カテゴリーを編集").c_str());
        ImGui::SameLine();
        imgui_utils::helpMarker(ICON_MS_HELP, m_config_wrapper->tr(L"カテゴリーをドラッグで並び替え、右クリックからメニューを表示して編集できます。").c_str());
        ImGui::Separator();

        static ImGuiTextFilter filter;
        static int selected_index = -1;

        // ポップアップが開いたらフィルターをクリア
        if (ImGui::IsWindowAppearing()) {
            filter.Clear();
            selected_index = -1;
        }

        // 並び替え中に同じアイテムが二重に送信されることがあるため、1フレームのみのID競合が発生するという問題がある
        // そのため一時的に検出を無効にする
        ImGui::PushItemFlag(ImGuiItemFlags_AllowDuplicateId, true);

        // 検索欄
        ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputTextWithHint("##filter", input_placeholder.c_str(), filter.InputBuf, IM_COUNTOF(filter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll)) {
            filter.Build();
        }
        ImGui::PopItemFlag();
        std::string input_text = filter.InputBuf;

        std::vector<std::string> category_names = m_categories;

        // 既存カテゴリをループし、フィルターを通過したものだけを表示
        int32_t swap_a = -1, swap_b = -1;
        for (int32_t j = 0; j < static_cast<int32_t>(std::ssize(m_categories)); ++j) {
            std::string category_name_with_icon = ICON_MS_FOLDER + std::string{" "} + m_categories[j];
            if (filter.PassFilter(category_name_with_icon.c_str())) {
                bool selected = (selected_index == j);

                ImGui::Selectable(category_name_with_icon.c_str(), selected, ImGuiSelectableFlags_DontClosePopups);
                static std::string category_name{};

                //
                // 右クリックメニュー
                //
                static bool contains_same_category = false;
                if (ImGui::BeginPopupContextItem()) {
                    selected_index = j;

                    if (ImGui::IsWindowAppearing()) {
                        category_name = m_categories[j];
                        contains_same_category = false;
                    }

                    // 名前変更
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text(m_config_wrapper->tr(L"名前を変更").c_str());
                    ImGui::SameLine();
                    static ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_None;
                    input_flags |= ImGuiInputTextFlags_CharsNoBlank;      // スペース、タブなし
                    input_flags |= ImGuiInputTextFlags_AutoSelectAll;     // 最初にマウスフォーカスが当たったときにテキスト全体を選択

                    if (ImGui::InputText("##edit", &category_name, input_flags)) {
                        contains_same_category = PresetManager::containsCategory(m_categories, category_name);
                    }
                    ImGui::SameLine();

                    if (contains_same_category) ImGui::BeginDisabled();
                    bool save = ImGui::Button(m_config_wrapper->tr(L"保存").c_str());
                    if (contains_same_category) ImGui::EndDisabled();

                    if (contains_same_category) {
                        ImGui::SetTooltip(m_config_wrapper->tr(L"すでに同じ名前のカテゴリが存在します").c_str());
                    }

                    // 置き換え
                    if ((!contains_same_category && ImGui::IsItemDeactivatedAfterEdit() && ImGui::IsKeyPressed(ImGuiKey_Enter)) || save) {
                        if (!PresetManager::containsCategory(m_categories, category_name)) {
                            m_categories[j] = category_name;
                            auto result = manager.changeCategories(file, m_categories);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            }
                            contains_same_category = false;
                        }
                    }

                    if (ImGui::BeginMenu(m_config_wrapper->tr(L"プリセットをまとめて移動").c_str())) {
                        ImGui::Text(m_config_wrapper->tr(L"移動先").c_str());
                        ImGui::Separator();
                        for (const auto& c : m_categories) {
                            if (ImGui::MenuItem(c.c_str())) {
                                auto result = manager.changeCategory(file, m_categories[j], c);
                                if (!result.is_success) {
                                    m_logger_wrapper->error("{}", result.error);
                                }
                            }
                        }
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu(m_config_wrapper->tr(L"削除").c_str())) {
                        if (ImGui::MenuItem(m_config_wrapper->tr(L"カテゴリのみ").c_str())) {
                            auto result = manager.deleteOnlyCategory(file, m_categories[j]);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            } else {
                                loadCategories();
                            }
                        }
                        if (ImGui::MenuItem(m_config_wrapper->tr(L"プリセットごと").c_str())) {
                            auto result = manager.deleteCategoryAndPresets(file, m_categories[j]);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            } else {
                                loadCategories();
                            }
                        }
                        ImGui::EndMenu();
                    }

                    if (ImGui::Button(m_config_wrapper->tr(L"閉じる").c_str())) {
                        contains_same_category = false;
                        selected_index = -1;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }
            }

            // スワップ
            if (ImGui::IsItemActive() && !ImGui::IsItemHovered()) {
                int n_next = j + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
                if (n_next >= 0 && n_next < std::ssize(m_categories)) {
                    swap_a = j;
                    swap_b = n_next;
                    ImGui::ResetMouseDragDelta();
                }
            }
        }

        if (swap_a != -1 && swap_b != -1) {
            std::swap(m_categories[swap_a], m_categories[swap_b]);
        }

        // カテゴリを作成
        bool exist_same_name = (std::find(category_names.begin(), category_names.end(), input_text) != category_names.end());
        if (!exist_same_name && input_text != "") {
            std::string new_category_label = ICON_MS_ADD + std::string{" "} + input_text;
            if (ImGui::Selectable(new_category_label.c_str())) {
                auto result = manager.addCategory(file, input_text);
                if (!result.is_success) {
                    m_logger_wrapper->error("{}", result.error);
                } else {
                    loadCategories();
                }
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::PopItemFlag();
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    // プリセット名入力欄
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(item_inner_x, 0));
    if (ImGui::BeginTable("##align_table2", 4)) {
        std::string input_text = m_config_wrapper->tr(L"名前");
        float text_width = ImGui::CalcTextSize(input_text.c_str()).x;

        ImGui::TableSetupColumn("##text", ImGuiTableColumnFlags_WidthFixed, text_width);
        ImGui::TableSetupColumn("##combo", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##button1", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
        ImGui::TableSetupColumn("##button2", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // ラベル
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(input_text.c_str());

        // 名前入力欄
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText("##preset_name", &m_preset_name, ImGuiInputTextFlags_CharsNoBlank);

        // 入力された文字列のチェック
        bool exist_same_preset_name = false;
        for (const auto& p : file.presets) {
            if (p.name == m_preset_name) {
                exist_same_preset_name = true;
                break;
            }
        }
        bool is_empty = m_preset_name == "";

        // 上書きボタン
        ImGui::TableNextColumn();
        if (is_empty) ImGui::BeginDisabled(true);
        if (imgui_utils::squareIconButton(ICON_MS_SAVE, "##overwrite")) {
            ImGui::OpenPopup((m_config_wrapper->tr(L"上書き保存") + "###Overwrite confirmation").c_str());
        }
        if (is_empty) ImGui::EndDisabled();

        // 上書き確認ダイアログ
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGuiWindowFlags modal_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
        if (ImGui::BeginPopupModal((m_config_wrapper->tr(L"上書き保存") + "###Overwrite confirmation").c_str(), nullptr, modal_flags)) {
            std::string replace_preset_name = file.presets[m_selected_preset_index].name;
            ImGui::Text(m_config_wrapper->tr(L"プリセット \"%s\" を現在のグラデーションで \"%s\" として上書きしますか?").c_str(), replace_preset_name.c_str(), m_preset_name.c_str());
            ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * 0.25f));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * 0.25f));

            // 中央に配置
            float btn_width = 120 * 2 + ImGui::GetStyle().ItemSpacing.x * 1;
            float avail     = ImGui::GetContentRegionAvail().x;
            float off       = (avail - btn_width) * 0.5f;
            if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

            if (ImGui::Button((m_config_wrapper->tr(L"はい") + "###Yes").c_str(), ImVec2(120, 0))) {
                auto preset = manager.gradient2preset(m_target_gradient_data);
                auto result = manager.overwritePreset(file, preset, m_selected_preset_index, m_preset_name, selected_category_name);
                if (!result.is_success) {
                    m_logger_wrapper->error("{}", result.error);
                } else {
                    loadCategories();
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button((m_config_wrapper->tr(L"いいえ") + "###No").c_str(), ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
            ImGui::SetTooltip(m_config_wrapper->tr(L"上書き保存").c_str(), ImGui::GetStyle().HoverDelayNormal);
        }

        // 新規保存ボタン
        ImGui::TableNextColumn();
        if (exist_same_preset_name || is_empty) ImGui::BeginDisabled(true);
        if (imgui_utils::squareIconButton(ICON_MS_SAVE_AS, "##add")) {
            auto preset = manager.gradient2preset(m_target_gradient_data);
            auto result = manager.addPreset(file, preset, m_preset_name, selected_category_name);
            if (!result.is_success) {
                m_logger_wrapper->error("{}", result.error);
            } else {
                loadCategories();
            }
        }
        if (exist_same_preset_name || is_empty) ImGui::EndDisabled();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (exist_same_preset_name) {
                ImGui::SetTooltip(m_config_wrapper->tr(L"すでに同じ名前のプリセットが存在します。新規保存するには名前を変更してください").c_str(), ImGui::GetStyle().HoverDelayNormal);
            } else {
                ImGui::SetTooltip(m_config_wrapper->tr(L"新規保存").c_str(), ImGui::GetStyle().HoverDelayNormal);
            }
        }

        m_is_clicked_preset = false;

        ImGui::EndTable();
    }
    ImGui::PopStyleVar();

    // プリセット一覧を描画
    ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * 0.25f));
    renderPresetList(manager, file, selected_category_name);

    ImGui::End();
}

void PresetWindow::renderPresetList(PresetManager& manager, GradientConfig& file, std::string_view category)
{
    bool is_delete        = false;
    uint32_t delete_index = 0;

    auto loadCategories = [&]() {
        m_categories.clear();

        std::unordered_set<std::string> tmp;
        if (!file.categories.empty()) {
            for (const auto& [i, category] : file.categories | std::views::enumerate) {
                tmp.insert(category);
            }
        } else {
            tmp.insert("uncategorized");
        }

        for (const auto& e : tmp) m_categories.push_back(e);
    };

    // プリセットのデータを1つずつ取り出して描画
    ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, ImGui::GetFrameHeight() * 0.25f);
    for (const auto& [i, preset] : file.presets | std::views::enumerate) {
        ImGui::PushID(static_cast<int>(i));

        // プリセットからグラデーションのデータを得る
        gradient_editor::GradientData gradient = manager.preset2gradient(preset);

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
                m_preset_name = preset.name;                                                     // プリセット名を更新
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
                if (ImGui::Selectable(m_config_wrapper->tr(L"カテゴリを変更").c_str())) {
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

            //
            // カテゴリーを変更
            //
            std::string input_placeholder = (ICON_MS_SEARCH + std::string{" "} + m_config_wrapper->tr(L"カテゴリーを検索または作成"));
            float input_placeholder_width = ImGui::CalcTextSize(input_placeholder.c_str()).x;
            ImGui::SetNextWindowSize({input_placeholder_width + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetStyle().WindowPadding.x * 2.0f, 0});
            if (ImGui::BeginPopup("preset_category_selector")) {
                ImGui::Text(m_config_wrapper->tr(L"カテゴリーを変更").c_str());
                ImGui::SameLine();
                imgui_utils::helpMarker(ICON_MS_HELP, m_config_wrapper->tr(L"カテゴリーをクリックしてプリセットを移動します").c_str());
                ImGui::Separator();

                static ImGuiTextFilter filter;
                static int selected_index = -1;

                // ポップアップが開いたらフィルターをクリア
                if (ImGui::IsWindowAppearing()) {
                    filter.Clear();
                    selected_index = -1;
                }

                // 並び替え中に同じアイテムが二重に送信されることがあるため、1フレームのみのID競合が発生するという問題がある
                // そのため一時的に検出を無効にする
                ImGui::PushItemFlag(ImGuiItemFlags_AllowDuplicateId, true);

                // 検索欄
                ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputTextWithHint("##filter", input_placeholder.c_str(), filter.InputBuf, IM_COUNTOF(filter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll)) {
                    filter.Build();
                }
                ImGui::PopItemFlag();
                std::string input_text = filter.InputBuf;

                std::vector<std::string> category_names = m_categories;

                // 既存カテゴリをループし、フィルターを通過したものだけを表示
                for (int32_t j = 0; j < static_cast<int32_t>(std::ssize(m_categories)); ++j) {
                    std::string category_name_with_icon = ICON_MS_FOLDER + std::string{" "} + m_categories[j];
                    if (filter.PassFilter(category_name_with_icon.c_str())) {
                        bool selected = (selected_index == j);

                        // クリックしたらそのカテゴリに移動させる
                        if (ImGui::Selectable(category_name_with_icon.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
                            auto result = manager.changeCategory(file, i, m_categories[j]);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            } else {
                                loadCategories();
                            }
                        }
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(
                            ImGui::GetCursorPosX() +
                            ImMax(0.0f, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(m_config_wrapper->tr(L"移動").c_str()).x - ImGui::GetStyle().WindowPadding.x)
                        );
                        ImGui::Text(m_config_wrapper->tr(L"移動").c_str());
                        static std::string category_name{};
                    }
                }


                // カテゴリを作成
                bool exist_same_name = (std::find(category_names.begin(), category_names.end(), input_text) != category_names.end());
                if (!exist_same_name && input_text != "") {
                    std::string new_category_label = ICON_MS_ADD + std::string{" "} + input_text;

                    ImGui::SetNextItemAllowOverlap();
                    ImGui::Selectable(new_category_label.c_str());
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(
                        ImGui::GetCursorPosX() +
                        ImMax(0.0f, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(m_config_wrapper->tr(L"移動").c_str()).x - ImGui::GetStyle().WindowPadding.x)
                    );
                    if (ImGui::SmallButton(m_config_wrapper->tr(L"移動").c_str())) {
                        auto result = manager.addCategory(file, input_text);
                        if (!result.is_success) {
                            m_logger_wrapper->error("{}", result.error);
                        } else {
                            loadCategories();
                        }

                        result = manager.changeCategory(file, i, input_text);
                        if (!result.is_success) {
                            m_logger_wrapper->error("{}", result.error);
                        } else {
                            loadCategories();
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::PopItemFlag();
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

                    // スワップ
                    auto result = manager.swapPreset(file, static_cast<uint32_t>(i), payload_index);
                    if (!result.is_success) {
                        m_logger_wrapper->error("{}", result.error);
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }

        ImGui::PopID();
    }
    ImGui::PopStyleVar();

    if (is_delete) {
        auto result = manager.deletePreset(file, delete_index);
        if (!result.is_success) {
            m_logger_wrapper->error("{}", result.error);
        }
    }
}

}  // namespace gradient_editor
