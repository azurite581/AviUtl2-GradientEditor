#include "preset_window.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <ranges>

#include "IconsMaterialSymbols.h"
#include "gradient_widget.h"
#include "imgui.h"
#include "imgui_utils.h"
#include "misc/cpp/imgui_stdlib.h"

void PresetWindow::render(GradientConfigManager& manager, PresetConfig& cfg)
{
    static std::vector<std::string> categories;
    static int32_t category_selected_index    = m_selected_category_index = cfg.selected_category;
    static std::string selected_category_name = GradientConfigManager::DEFAULT_CATEGORY;

    // 重複なしでカテゴリーを読み込む
    auto loadCategories = [&]() {
        m_categories.clear();
        std::unordered_set<std::string> seen;
        if (!cfg.categories.empty()) {
            for (const auto& [i, category] : cfg.categories | std::views::enumerate) {
                if (seen.insert(category).second) {  // 新規なら true を返す
                    m_categories.push_back(category);
                }
            }
        } else {
            m_categories.push_back(GradientConfigManager::DEFAULT_CATEGORY);
        }
    };

    if (!m_is_initialized) {
        m_is_initialized = true;
        loadCategories();
        m_old_category_name = m_categories[category_selected_index];  // 起動時に読み込まれるカテゴリー
    }

    //
    // プリセットウィンドウ
    //
    static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin((m_config_wrapper->tr(L"プリセット") + "###preset_window").c_str(), nullptr, window_flags);

    float item_spacing_x = ImGui::GetStyle().ItemSpacing.x * 0.5f;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(item_spacing_x, 0));
    bool is_called_popup = false;

    if (ImGui::BeginTable("##align_table1", 3)) {
        std::string text = m_config_wrapper->tr(L"カテゴリー");
        float text_width = ImGui::CalcTextSize(text.c_str()).x;
        ImGui::TableSetupColumn("##text", ImGuiTableColumnFlags_WidthFixed, text_width);
        ImGui::TableSetupColumn("##combo", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##button", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(text.c_str());

        ImGui::TableNextColumn();
        static ImGuiComboFlags combo_flags = 0;

        // カテゴリーコンボボックス
        const char* combo_preview_value = m_categories[category_selected_index].c_str();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo("##category_combo", combo_preview_value, combo_flags)) {
            for (int i = 0; i < std::ssize(m_categories); ++i) {
                const bool is_selected = (category_selected_index == i);
                if (ImGui::Selectable(m_categories[i].c_str(), is_selected))
                    category_selected_index = m_selected_category_index = i;

                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        selected_category_name = m_categories[category_selected_index];

        // カテゴリー編集ボタン
        ImGui::TableNextColumn();
        is_called_popup = imgui_utils::squareIconButton(ICON_MS_MENU, "##category_menu");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay)) {
            ImGui::SetTooltip(m_config_wrapper->tr(L"カテゴリーを編集").c_str(), ImGui::GetStyle().HoverDelayNormal);
        }

        ImGui::EndTable();
    }
    ImGui::PopStyleVar();

    if (is_called_popup) {
        ImGui::OpenPopup("preset_category_editor");
    }

    //
    // カテゴリー編集ポップアップ
    //
    std::string input_placeholder = (ICON_MS_SEARCH + std::string{" "} + m_config_wrapper->tr(L"カテゴリーを検索または作成"));
    float input_placeholder_width = ImGui::CalcTextSize(input_placeholder.c_str()).x;
    ImGui::SetNextWindowSize({input_placeholder_width + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetStyle().WindowPadding.x * 2.0f, 0});
    ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, ImGui::GetFrameHeight() * ITEM_SPACING_SCALE_Y);

    if (ImGui::BeginPopup("preset_category_editor")) {
        ImGui::Text(m_config_wrapper->tr(L"カテゴリーを編集").c_str());
        ImGui::SameLine();
        imgui_utils::helpMarker(ICON_MS_HELP, m_config_wrapper->tr(L"カテゴリーをドラッグで並び替え、右クリックからメニューを表示して編集できます").c_str());
        ImGui::Separator();

        static ImGuiTextFilter filter;
        static int selected_index = -1;

        if (ImGui::IsWindowAppearing()) {
            filter.Clear();
            selected_index = -1;
        }

        // 検索欄
        ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputTextWithHint("##filter", input_placeholder.c_str(), filter.InputBuf, IM_COUNTOF(filter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll)) {
            filter.Build();
        }
        ImGui::PopItemFlag();

        std::string input_text                  = filter.InputBuf;
        std::vector<std::string> category_names = m_categories;

        // 並び替え中に同じアイテムが二重に送信されることがあるため、1フレームのみのID競合が発生するという問題がある
        // そのため一時的に検出を無効にする
        ImGui::PushItemFlag(ImGuiItemFlags_AllowDuplicateId, true);

        // 既存カテゴリーをループし、フィルターを通過したものだけを表示
        int32_t swap_a = -1, swap_b = -1;
        for (int32_t j = 0; j < static_cast<int32_t>(std::ssize(m_categories)); ++j) {
            std::string category_name_with_icon = ICON_MS_DRAG_INDICATOR + std::string{" "} + ICON_MS_FOLDER + std::string{" "} + m_categories[j];

            if (filter.PassFilter(category_name_with_icon.c_str())) {
                bool selected = (selected_index == j);

                ImGui::Selectable(category_name_with_icon.c_str(), selected, ImGuiSelectableFlags_DontClosePopups);
                static std::string category_name{};

                // スワップ
                if (ImGui::IsItemActive() && !ImGui::IsItemHovered()) {
                    int32_t j_next = j + (ImGui::GetMouseDragDelta(0).y < 0.0f ? -1 : 1);
                    if (j_next >= 0 && j_next < std::ssize(m_categories)) {
                        swap_a = j;
                        swap_b = j_next;
                        ImGui::ResetMouseDragDelta();
                    }
                }

                // 右クリックメニュー
                static bool contains_same_category = false;
                if (ImGui::BeginPopupContextItem()) {
                    selected_index = j;

                    if (ImGui::IsWindowAppearing()) {
                        category_name          = m_categories[j];
                        m_old_category_name    = m_categories[j];
                        contains_same_category = false;
                    }

                    // 名前変更
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text(m_config_wrapper->tr(L"名前を変更").c_str());
                    ImGui::SameLine();
                    static ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_None;
                    input_flags |= ImGuiInputTextFlags_CharsNoBlank;   // スペース、タブなし
                    input_flags |= ImGuiInputTextFlags_AutoSelectAll;  // 最初にマウスフォーカスが当たったときにテキスト全体を選択

                    // カテゴリー名入力欄
                    if (ImGui::InputText("##edit", &category_name, input_flags)) {
                        contains_same_category = GradientConfigManager::containsCategory(m_categories, category_name);
                    }
                    ImGui::SameLine();

                    if (contains_same_category || category_name.empty()) ImGui::BeginDisabled();
                    bool save = ImGui::Button(m_config_wrapper->tr(L"保存").c_str());
                    if (contains_same_category || category_name.empty()) ImGui::EndDisabled();

                    if (contains_same_category) {
                        ImGui::SetTooltip(m_config_wrapper->tr(L"すでに同じ名前のカテゴリが存在します").c_str());
                    }

                    // 名前に重複がなければカテゴリー名を置き換える
                    if ((!contains_same_category && ImGui::IsItemDeactivatedAfterEdit() && ImGui::IsKeyPressed(ImGuiKey_Enter)) || save) {
                        if (!GradientConfigManager::containsCategory(m_categories, category_name)) {
                            // カテゴリー名の配列を置き換える
                            m_categories[j] = category_name;
                            auto result     = manager.changeCategories(cfg, m_categories);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            }

                            // 各プリセットのカテゴリーを置き換える
                            result = manager.changeCategory(cfg, m_old_category_name, category_name);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            } else {
                                loadCategories();
                            }

                            m_old_category_name    = category_name;
                            contains_same_category = false;
                        }
                    }

                    if (ImGui::BeginMenu(m_config_wrapper->tr(L"プリセットをまとめて移動").c_str())) {
                        ImGui::Text(m_config_wrapper->tr(L"移動先").c_str());
                        ImGui::Separator();
                        for (const auto& c : m_categories) {
                            if (c == m_old_category_name) continue;
                            if (ImGui::MenuItem(c.c_str())) {
                                auto result = manager.changeCategory(cfg, m_old_category_name, c);
                                if (!result.is_success) {
                                    m_logger_wrapper->error("{}", result.error);
                                }
                            }
                        }
                        ImGui::EndMenu();
                    }

                    bool has_categories = static_cast<int32_t>(std::ssize(m_categories)) <= 1;
                    if (has_categories) ImGui::BeginDisabled();
                    if (ImGui::BeginMenu(m_config_wrapper->tr(L"削除").c_str())) {
                        if (ImGui::MenuItem(m_config_wrapper->tr(L"カテゴリのみ").c_str())) {
                            int32_t delete_category_index = 0;
                            for (int32_t del_idx = 0; const auto& c : m_categories) {
                                if (c == m_old_category_name) {
                                    delete_category_index = del_idx;
                                    break;
                                }
                                ++del_idx;
                            }

                            // 削除前にコンボボックスのインデックスを変更する
                            if (delete_category_index != 0 && category_selected_index >= delete_category_index) {
                                category_selected_index--;
                            }

                            // デフォルトカテゴリーの作成
                            auto result = manager.addCategory(cfg, GradientConfigManager::DEFAULT_CATEGORY);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            }

                            // 削除対象のカテゴリーに属していたプリセットのカテゴリーをデフォルトカテゴリーに変更
                            result = manager.changeCategory(cfg, m_old_category_name, GradientConfigManager::DEFAULT_CATEGORY);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            }

                            // カテゴリー名のみを削除
                            result = manager.deleteOnlyCategory(cfg, m_old_category_name);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            } else {
                                loadCategories();
                            }
                        }

                        if (ImGui::MenuItem(m_config_wrapper->tr(L"プリセットごと").c_str())) {
                            int32_t delete_category_index = 0;
                            for (int32_t del_idx = 0; const auto& c : m_categories) {
                                if (c == m_old_category_name) {
                                    delete_category_index = del_idx;
                                    break;
                                }
                                ++del_idx;
                            }

                            // 削除前にコンボボックスのインデックスを変更する
                            if (delete_category_index != 0 && category_selected_index >= delete_category_index) {
                                category_selected_index--;
                            }

                            auto result = manager.deleteCategoryAndPresets(cfg, m_old_category_name);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            } else {
                                loadCategories();
                            }
                        }
                        ImGui::EndMenu();
                    }
                    if (has_categories) ImGui::EndDisabled();

                    if (ImGui::Button(m_config_wrapper->tr(L"閉じる").c_str())) {
                        contains_same_category = false;
                        selected_index         = -1;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
        }

        // スワップ
        if (swap_a != -1 && swap_b != -1) {
            std::swap(m_categories[swap_a], m_categories[swap_b]);
            manager.swapCategory(cfg, swap_a, swap_b);
        }

        // カテゴリーを作成
        bool exist_same_name = (std::find(category_names.begin(), category_names.end(), input_text) != category_names.end());
        if (!exist_same_name && input_text != "") {
            std::string new_category_label = ICON_MS_ADD + std::string{" "} + input_text;
            if (ImGui::Selectable(new_category_label.c_str())) {
                if (!manager.containsCategory(cfg.categories, input_text)) {
                    auto result = manager.addCategory(cfg, input_text);
                    if (!result.is_success) {
                        m_logger_wrapper->error("{}", result.error);
                    } else {
                        loadCategories();
                    }
                }
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::PopItemFlag();
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    // プリセット名入力欄
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(item_spacing_x, 0));
    if (ImGui::BeginTable("##align_table2", 4)) {
        std::string input_text = (m_config_wrapper->tr(L"プリセット名") + ":");
        float text_width       = ImGui::CalcTextSize(input_text.c_str()).x;

        ImGui::TableSetupColumn("##text1", ImGuiTableColumnFlags_WidthFixed, text_width);
        ImGui::TableSetupColumn("##text2", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##button1", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
        ImGui::TableSetupColumn("##button2", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // ラベル
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(input_text.c_str());

        // 最後に選択されたプリセット名を表示
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(m_preset_name.c_str());

        // 上書きボタン
        ImGui::TableNextColumn();
        if (m_selected_preset_index == -1) ImGui::BeginDisabled();
        if (imgui_utils::squareIconButton(ICON_MS_SAVE, "##overwrite")) {
            ImGui::OpenPopup((m_config_wrapper->tr(L"上書き保存") + "###overwrite_confirmation").c_str());
        }
        if (m_selected_preset_index == -1) ImGui::EndDisabled();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (m_selected_preset_index == -1) {
                ImGui::SetTooltip(m_config_wrapper->tr(L"プリセットが未選択です").c_str(), ImGui::GetStyle().HoverDelayNormal);
            } else {
                ImGui::SetTooltip(m_config_wrapper->tr(L"上書き保存").c_str(), ImGui::GetStyle().HoverDelayNormal);
            }
        }

        static ImGuiWindowFlags modal_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal((m_config_wrapper->tr(L"上書き保存") + "###overwrite_confirmation").c_str(), nullptr, modal_flags)) {

            static std::string original_preset_name = cfg.presets[m_selected_preset_index].name;
            static std::string input_preset_name    = original_preset_name;
            if (ImGui::IsWindowAppearing()) {
                original_preset_name = cfg.presets[m_selected_preset_index].name;
                input_preset_name    = original_preset_name;
            }

            ImGui::Text(m_config_wrapper->tr(L"元の名前").c_str());
            ImGui::SameLine();
            ImGui::Text(": \"%s\"", original_preset_name.c_str());

            ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * ITEM_SPACING_SCALE_Y));

            ImGui::InputTextWithHint("##preset_name", original_preset_name.c_str(), &input_preset_name, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll);
            bool is_empty = input_preset_name == "";

            ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * ITEM_SPACING_SCALE_Y));

            // 2つのボタンを中央に配置
            float btn_width = MODAL_WINDOW_WIDTH * 2 + ImGui::GetStyle().ItemSpacing.x * 1;
            float avail     = ImGui::GetContentRegionAvail().x;
            float off       = (avail - btn_width) * 0.5f;
            if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

            if (ImGui::Button((m_config_wrapper->tr(L"キャンセル") + "###cancel").c_str(), ImVec2(MODAL_WINDOW_WIDTH, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();

            if (is_empty) ImGui::BeginDisabled();
            if (ImGui::Button((m_config_wrapper->tr(L"上書き保存") + "###overwrite").c_str(), ImVec2(MODAL_WINDOW_WIDTH, 0))) {
                auto preset = manager.gradient2preset(m_target_gradient_data);
                auto result = manager.overwritePreset(cfg, preset, m_selected_preset_index, input_preset_name, selected_category_name);
                if (!result.is_success) {
                    m_logger_wrapper->error("{}", result.error);
                } else {
                    loadCategories();
                }
                m_preset_name = input_preset_name;

                ImGui::CloseCurrentPopup();
            }
            if (is_empty) ImGui::EndDisabled();

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_AllowWhenDisabled)) {
                if (is_empty) {
                    ImGui::SetTooltip(m_config_wrapper->tr(L"プリセット名が空です").c_str(), ImGui::GetStyle().HoverDelayNormal);
                }
            }

            ImGui::EndPopup();
        }

        // 新規保存ボタン
        ImGui::TableNextColumn();
        if (imgui_utils::squareIconButton(ICON_MS_SAVE_AS, "##save_as")) {
            ImGui::OpenPopup((m_config_wrapper->tr(L"新規保存") + "###save_as_confirmation").c_str());
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip(m_config_wrapper->tr(L"新規保存").c_str(), ImGui::GetStyle().HoverDelayNormal);
        }

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal((m_config_wrapper->tr(L"新規保存") + "###save_as_confirmation").c_str(), nullptr, modal_flags)) {

            static std::string input_preset_name = m_preset_name;
            if (ImGui::IsWindowAppearing()) {
                input_preset_name = m_preset_name;
            }

            ImGui::InputTextWithHint("##preset_name", m_config_wrapper->tr(L"プリセット名").c_str(), &input_preset_name, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll);
            bool is_empty = input_preset_name == "";
            bool exist_same_preset_name = false;
            for (const auto& p : cfg.presets) {
                if (p.name == input_preset_name) {
                    exist_same_preset_name = true;
                    break;
                }
            }

            ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * ITEM_SPACING_SCALE_Y));

            float btn_width = MODAL_WINDOW_WIDTH * 2 + ImGui::GetStyle().ItemSpacing.x * 1;
            float avail     = ImGui::GetContentRegionAvail().x;
            float off       = (avail - btn_width) * 0.5f;
            if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

            if (ImGui::Button((m_config_wrapper->tr(L"キャンセル") + "###cancel").c_str(), ImVec2(MODAL_WINDOW_WIDTH, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            if (is_empty || exist_same_preset_name) ImGui::BeginDisabled(true);
            if (ImGui::Button((m_config_wrapper->tr(L"保存") + "###save").c_str(), ImVec2(MODAL_WINDOW_WIDTH, 0))) {
                auto preset = manager.gradient2preset(m_target_gradient_data);
                auto result = manager.addPreset(cfg, preset, input_preset_name, selected_category_name);
                if (!result.is_success) {
                    m_logger_wrapper->error("{}", result.error);
                } else {
                    loadCategories();
                    // 追加されたプリセットを選択状態にする
                    m_selected_preset_index = static_cast<int32_t>(cfg.presets.size()) - 1;
                    m_selected_gradient     = manager.preset2gradient(cfg.presets.back());
                }
                m_preset_name = input_preset_name;

                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            if (is_empty || exist_same_preset_name) ImGui::EndDisabled();

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay | ImGuiHoveredFlags_AllowWhenDisabled)) {
                if (is_empty) {
                    ImGui::SetTooltip(m_config_wrapper->tr(L"プリセット名が空です").c_str(), ImGui::GetStyle().HoverDelayNormal);
                } else if (exist_same_preset_name) {
                    ImGui::SetTooltip(m_config_wrapper->tr(L"すでに同じ名前のプリセットが存在します").c_str(), ImGui::GetStyle().HoverDelayNormal);
                }
            }

            ImGui::EndPopup();
        }

        m_is_clicked_preset = false;

        ImGui::EndTable();
    }
    ImGui::PopStyleVar();

    ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * ITEM_SPACING_SCALE_Y));

    // プリセット一覧を描画
    renderPresetList(manager, cfg, selected_category_name);

    ImGui::End();
}

void PresetWindow::renderPresetList(GradientConfigManager& manager, PresetConfig& cfg, std::string_view category)
{
    bool is_delete               = false;
    static uint32_t delete_index = 0;

    auto loadCategories = [&]() {
        m_categories.clear();
        std::unordered_set<std::string> seen;
        if (!cfg.categories.empty()) {
            for (const auto& [i, category] : cfg.categories | std::views::enumerate) {
                if (seen.insert(category).second) {  // 新規なら true を返す
                    m_categories.push_back(category);
                }
            }
        } else {
            m_categories.push_back(GradientConfigManager::DEFAULT_CATEGORY);
        }
    };

    ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, ImGui::GetFrameHeight() * ITEM_SPACING_SCALE_Y);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FrameBorderSize, ImGui::GetStyle().FrameBorderSize));
    ImVec2 gradient_size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * 1.5f);
    gradient_size.x = ImMax(1.0f, gradient_size.x);
    gradient_size.y = ImMax(1.0f, gradient_size.y);

    // プリセットのデータを1つずつ取り出して描画
    m_is_clicked_preset = false;
    bool is_any_clicked = false;
    for (const auto& [i, preset] : cfg.presets | std::views::enumerate) {
        ImGui::PushID(static_cast<int32_t>(i));

        // プリセットからグラデーションデータを取得
        GradientData gradient = manager.preset2gradient(preset);

        if (preset.category == category) {
            // プリセットを描画
            ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.0f);

            is_any_clicked = CustomUI::drawGradientButton(preset.name, gradient_size, gradient);
            if (is_any_clicked) {
                m_is_clicked_preset     = true;
                m_selected_preset_index = static_cast<int32_t>(i);  // 選択中のインデックスを更新
                m_selected_gradient     = gradient;                 // 選択中のグラデーションを更新
                m_preset_name           = preset.name;              // プリセット名を更新
            }
            ImGui::PopStyleVar();

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone)) {
                ImGui::SetTooltip(preset.name.c_str());
            }

            // 右クリックメニュー
            bool open_category_popup = false;
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::Selectable(m_config_wrapper->tr(L"カテゴリーを変更").c_str())) {
                    open_category_popup = true;
                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::Selectable(m_config_wrapper->tr(L"削除").c_str())) {
                    is_delete    = true;
                    delete_index = static_cast<uint32_t>(i);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            // カテゴリーを変更
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

                ImGui::PushItemFlag(ImGuiItemFlags_AllowDuplicateId, true);
                // 検索欄
                ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputTextWithHint("##filter", input_placeholder.c_str(), filter.InputBuf, IM_COUNTOF(filter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll)) {
                    filter.Build();
                }
                ImGui::PopItemFlag();

                std::string input_text                  = filter.InputBuf;
                std::vector<std::string> category_names = m_categories;

                // 既存カテゴリーをループし、フィルターを通過したものだけを表示
                for (int32_t j = 0; j < static_cast<int32_t>(std::ssize(m_categories)); ++j) {
                    std::string category_name_with_icon = ICON_MS_FOLDER + std::string{" "} + m_categories[j];
                    if (filter.PassFilter(category_name_with_icon.c_str()) && m_categories[j] != m_old_category_name) {
                        bool selected = (selected_index == j);

                        // クリックしたらプリセットをそのカテゴリに移動させる
                        if (ImGui::Selectable(category_name_with_icon.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
                            auto result = manager.changeCategory(cfg, static_cast<uint32_t>(i), m_categories[j]);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            } else {
                                loadCategories();
                            }
                        }
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(
                            ImGui::GetCursorPosX() +
                            ImMax(0.0f, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(m_config_wrapper->tr(L"移動").c_str()).x - ImGui::GetStyle().WindowPadding.x));
                        ImGui::Text(m_config_wrapper->tr(L"移動").c_str());
                        static std::string category_name{};
                    }
                }

                // カテゴリーを作成
                bool exist_same_name = (std::find(category_names.begin(), category_names.end(), input_text) != category_names.end());
                if (!exist_same_name && input_text != "") {
                    std::string new_category_label = ICON_MS_ADD + std::string{" "} + input_text;

                    ImGui::SetNextItemAllowOverlap();
                    if (ImGui::Selectable(new_category_label.c_str())) {
                        // 入力された名前でカテゴリーを作成
                        if (!manager.containsCategory(cfg.categories, input_text)) {
                            auto result = manager.addCategory(cfg, input_text);
                            if (!result.is_success) {
                                m_logger_wrapper->error("{}", result.error);
                            } else {
                                loadCategories();
                            }
                        }

                        // プリセットを作成したカテゴリーに変更
                        auto result = manager.changeCategory(cfg, static_cast<uint32_t>(i), input_text);
                        if (!result.is_success) {
                            m_logger_wrapper->error("{}", result.error);
                        } else {
                            loadCategories();
                        }

                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(
                        ImGui::GetCursorPosX() +
                        ImMax(0.0f, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(m_config_wrapper->tr(L"移動").c_str()).x - ImGui::GetStyle().WindowPadding.x));
                    ImGui::Text(m_config_wrapper->tr(L"移動").c_str());
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
                    auto result = manager.swapPreset(cfg, static_cast<uint32_t>(i), payload_index);
                    if (!result.is_success) {
                        m_logger_wrapper->error("{}", result.error);
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }

        ImGui::PopID();
    }
    ImGui::PopStyleVar(2);

    // 削除確認ダイアログ
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGuiWindowFlags modal_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;

    if (ImGui::BeginPopupModal((m_config_wrapper->tr(L"削除") + "###delete_confirmation").c_str(), nullptr, modal_flags)) {
        std::string replace_preset_name = cfg.presets[delete_index].name;
        ImGui::Text(m_config_wrapper->tr(L"プリセット \"%s\" を削除しますか?").c_str(), replace_preset_name.c_str(), m_preset_name.c_str());

        ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * ITEM_SPACING_SCALE_Y));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * ITEM_SPACING_SCALE_Y));

        // 中央に配置
        float btn_width = MODAL_WINDOW_WIDTH * 2 + ImGui::GetStyle().ItemSpacing.x * 1;
        float avail     = ImGui::GetContentRegionAvail().x;
        float off       = (avail - btn_width) * 0.5f;
        if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

        if (ImGui::Button((m_config_wrapper->tr(L"はい") + "###yes").c_str(), ImVec2(MODAL_WINDOW_WIDTH, 0))) {
            auto result = manager.deletePreset(cfg, delete_index);
            if (!result.is_success) {
                m_logger_wrapper->error("{}", result.error);
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button((m_config_wrapper->tr(L"いいえ") + "###no").c_str(), ImVec2(MODAL_WINDOW_WIDTH, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (is_delete) {
        ImGui::OpenPopup((m_config_wrapper->tr(L"削除") + "###delete_confirmation").c_str());
    }
}

void PresetWindow::overwriteCatogories(GradientConfigManager& manager, PresetConfig& cfg)
{
    cfg.selected_category = m_selected_category_index;

    // 書き込み
    auto result = manager.writePreset(cfg);
    if (result.is_success) {
        m_logger_wrapper->error("{}", result.error);
    }
}
