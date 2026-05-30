#include "main_view.h"

#include <algorithm>
#include <expected>
#include <filesystem>
#include <format>
#include <iostream>
#include <string_view>

#include "IconsMaterialSymbols.h"
#include "alias_parser.h"
#include "app_state.h"
#include "color_conv.h"
#include "constants.h"
#include "file_dialog.h"
#include "gradient_widget.h"
#include "grd_codec.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_utils.h"
#include "plugin2_utils.h"
#include "str_conv.h"

MainView::MainView(LoggerWrapperInterface* logger_wrapper, ConfigWrapperInterface* config_wrapper)
    : m_logger_wrapper{logger_wrapper}, m_config_wrapper{config_wrapper}
{
    // std::cout の出力先をファイルにリダイレクトする（デバッグ用）
    // std::ofstream file("log.txt");
    // std::cout.rdbuf(file.rdbuf());

    m_preset_window.setLoggerWrapper(m_logger_wrapper);
    m_preset_window.setConfigWrapper(m_config_wrapper);
    m_history_window.setLoggerWrapper(m_logger_wrapper);
    m_history_window.setConfigWrapper(m_config_wrapper);
    m_script_bridge.setLoggerWrapper(m_logger_wrapper);

    std::filesystem::path data_path          = gradient_editor::g_app_state.config_handle->app_data_path;
    std::filesystem::path config_folder_path = data_path / L"Plugin" / CONFIG_FOLDER_NAME;

    // プリセットフォルダがなければ作成
    if (!std::filesystem::exists(config_folder_path) || !std::filesystem::is_directory(config_folder_path)) {
        std::filesystem::create_directories(config_folder_path);
    }

    // プリセットファイルがなければ作成
    std::filesystem::path preset_path = config_folder_path / PRESET_FILE_NAME;
    if (!std::filesystem::exists(preset_path)) {
        std::ofstream ofs(preset_path, std::ios::binary);
        if (ofs.is_open()) {
            ofs << GradientConfigManager::DEFAULT_PRESET_FILE_JSON;
        } else {
            m_logger_wrapper->error("Failed to create preset file at: {}", preset_path.string());
        }
    }

    // プリセットを読み込む
    m_config_manager.setPresetFilePath(preset_path);
    auto preset_load_result = m_config_manager.loadPresetConfig();
    if (!preset_load_result.is_success()) {
        auto error_msg = preset_load_result.error + "\n";
        m_logger_wrapper->error("{}", error_msg);  // エラーメッセージに '{' または '}' があると std::format_error になるため "{}" で受け取る
        OutputDebugStringA(error_msg.c_str());
    }
    m_preset_config = preset_load_result.config;

    // 履歴ファイルがなければ作成
    std::filesystem::path history_path = config_folder_path / HISTORY_FILE_NAME;
    if (!std::filesystem::exists(history_path)) {
        std::ofstream history_file(history_path);
        if (!history_file) {
            m_logger_wrapper->error("{}", "Failed to create history file.");
        }
    }

    // 履歴を読み込む
    m_config_manager.setHistoryFilePath(history_path);
    auto history_load_result = m_config_manager.loadHistoryConfig();
    if (!history_load_result.is_success()) {
        m_logger_wrapper->error("{}", history_load_result.error.c_str());
    }
    m_history_config = history_load_result.config;

    m_object_video_color_start = color_conv::u32Rgba2u32Abgr(color_conv::u32Rgb2u32Rgba(gradient_editor::g_app_state.config_handle->get_color_code_index(gradient_editor::g_app_state.config_handle, "ObjectVideo", 0), 0xFF));
    m_object_video_color_stop  = color_conv::u32Rgba2u32Abgr(color_conv::u32Rgb2u32Rgba(gradient_editor::g_app_state.config_handle->get_color_code_index(gradient_editor::g_app_state.config_handle, "ObjectVideo", 1), 0xFF));
    m_frame_cursor_color       = color_conv::u32Rgba2u32Abgr(color_conv::u32Rgb2u32Rgba(gradient_editor::g_app_state.config_handle->get_color_code(gradient_editor::g_app_state.config_handle, "FrameCursor"), 0xFF));
}

bool MainView::colorPickerPopup(const char* label, ImVec4& current_color, ImVec4& previous_color)
{
    bool changed = false;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 2));
    ImGui::PushStyleVarX(ImGuiStyleVar_ItemInnerSpacing, 2);

    if (ImGui::BeginPopup(label)) {
        changed |= ImGui::ColorPicker4("##marker_color_picker", (float*)&current_color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_AlphaBar);
        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::TextUnformatted(m_config_wrapper->tr(L"現在の色").c_str());
            ImGuiColorEditFlags color_button_flags = ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf;
            ImVec2 color_button_size               = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x * 0.6f);
            changed |= ImGui::ColorButton("##current_color", current_color, color_button_flags, color_button_size);

            ImGui::TextUnformatted(m_config_wrapper->tr(L"以前の色").c_str());
            if (ImGui::ColorButton("##previous_color", previous_color, color_button_flags, color_button_size)) {
                changed |= true;
                current_color = previous_color;
            }

            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::Dummy(ImVec2(avail.x, avail.y - ImGui::GetFrameHeightWithSpacing()));
            if (ImGui::Button(m_config_wrapper->tr(L"閉じる").c_str(), ImVec2(avail.x, ImGui::GetFrameHeight()))) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndGroup();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2);

    return changed;
}

void MainView::render()
{
    // ドッキングスペースの設定
    ImGuiID dockspace_id    = ImGui::GetID("dockspace");
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
        ImGuiID dock_id_right = 0;
        ImGuiID dock_id_main  = dockspace_id;
        ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Right, PRESET_WINDOW_RATIO, &dock_id_right, &dock_id_main);
        ImGui::DockBuilderDockWindow("###gradient_editor_window", dock_id_main);
        ImGui::DockBuilderDockWindow("###preset_window", dock_id_right);
        ImGui::DockBuilderDockWindow("###history_window", dock_id_right);
        ImGui::DockBuilderFinish(dockspace_id);
    }
    ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

    static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_MenuBar;
    static ImGuiWindowClass window_class;
    window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
    ImGui::SetNextWindowClass(&window_class);
    ImGui::Begin("###gradient_editor_window", nullptr, window_flags);

    // メニューバーの描画
    std::string settings_popup_name = m_config_wrapper->tr(L"UIの設定") + "###style_settings";
    bool open_style_settings_popup  = false;

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu(m_config_wrapper->tr(L"ファイル").c_str())) {
            if (ImGui::MenuItem(m_config_wrapper->tr(L"プリセットを読み込む").c_str(), nullptr)) {
                auto open_file_dialog_result = openFiles(gradient_editor::g_app_state.host_app_hwnd);
                switch (open_file_dialog_result.result) {
                    case FileDialogResult::FD_OKAY: {
                        auto paths = std::get<static_cast<int32_t>(FileDialogResult::FD_OKAY)>(open_file_dialog_result.value);
                        for (const auto& path : paths) {
                            auto grd = parseGRD(path);
                            if (!grd) {
                                m_logger_wrapper->error("{}", grd.error());
                                continue;
                            }

                            // GRDファイル → プリセット形式変換時のログをまとめて表示
                            auto [presets, message] = GradientConfigManager::grd2preset(grd.value());
                            for (const auto& msg : message) {
                                m_logger_wrapper->log("{}", msg);
                            }

                            // プリセットをプリセットウィンドウに読み込む
                            for (const auto& preset : presets) {
                                // ファイル名をカテゴリー名にする
                                std::string file_name = str_conv::wideCharToMultiByte(path.stem().wstring());
                                m_config_manager.addCategory(m_preset_config, file_name);
                                auto categories = m_config_manager.loadCategories(m_preset_config);
                                m_preset_window.setCategories(categories);
                                m_config_manager.addPreset(m_preset_config, preset, preset.name, file_name);
                            }
                        }
                        break;
                    }
                    case FileDialogResult::FD_CANCEL: {
                        break;
                    }
                    case FileDialogResult::FD_ERROR: {
                        auto error_msg = std::get<static_cast<int32_t>(FileDialogResult::FD_ERROR)>(open_file_dialog_result.value);
                        m_logger_wrapper->error(L"{}", error_msg);
                        break;
                    }
                }
            }

            if (ImGui::BeginMenu(m_config_wrapper->tr(L"プリセットを出力").c_str())) {
                // 現在のグラデーションのみを GRD ファイルとして出力
                if (ImGui::MenuItem(m_config_wrapper->tr(L"現在のグラデーション").c_str(), nullptr)) {
                    auto open_file_dialog_result = writeFile(gradient_editor::g_app_state.host_app_hwnd);
                    switch (open_file_dialog_result.result) {
                        case FileDialogResult::FD_OKAY: {
                            auto path             = std::get<static_cast<int32_t>(FileDialogResult::FD_OKAY)>(open_file_dialog_result.value);
                            std::string file_name = str_conv::wideCharToMultiByte(path.stem().wstring());

                            if (m_data == nullptr) break;
                            // グラデーションデータ → グラデーションプリセット形式
                            auto gradient_preset = GradientConfigManager::gradient2preset(*m_data);
                            gradient_preset.name = file_name;  // ファイル名をプリセット名にする

                            // グラデーションプリセット → GRD 形式
                            const auto grd = GradientConfigManager::presets2grd({gradient_preset});
                            if (!grd) {
                                m_logger_wrapper->error("{}", grd.error());
                                break;
                            }

                            const auto write_grd_result = writeGRD(grd.value(), path);
                            if (!write_grd_result) {
                                m_logger_wrapper->error("{}", write_grd_result.error());
                            }

                            m_logger_wrapper->log("Successfully exported GRD file: {}", path.string());
                            break;
                        }
                        case FileDialogResult::FD_CANCEL: {
                            break;
                        }
                        case FileDialogResult::FD_ERROR: {
                            auto error_msg = std::get<static_cast<int32_t>(FileDialogResult::FD_ERROR)>(open_file_dialog_result.value);
                            m_logger_wrapper->error(L"{}", error_msg);
                            break;
                        }
                    }
                }

                // カテゴリー内のプリセットをまとめて GRD ファイルとして出力
                if (ImGui::BeginMenu(m_config_wrapper->tr(L"カテゴリー").c_str())) {
                    auto categories = m_config_manager.loadCategories(m_preset_config);
                    for (const auto& category : categories) {
                        if (ImGui::MenuItem(category.c_str(), nullptr)) {
                            auto open_file_dialog_result = writeFile(gradient_editor::g_app_state.host_app_hwnd);
                            switch (open_file_dialog_result.result) {
                                case FileDialogResult::FD_OKAY: {
                                    auto path = std::get<static_cast<int32_t>(FileDialogResult::FD_OKAY)>(open_file_dialog_result.value);

                                    // 選択したカテゴリーのプリセットを取得
                                    std::vector<GradientPreset> presets;
                                    for (const auto& [i, preset] : m_preset_config.presets | std::views::enumerate) {
                                        if (preset.category == category) {
                                            presets.push_back(preset);
                                        }
                                    }

                                    // グラデーションプリセット → GRD 形式
                                    const auto grd = GradientConfigManager::presets2grd(presets);
                                    if (!grd) {
                                        m_logger_wrapper->error("{}", grd.error());
                                        break;
                                    }

                                    // GRD ファイルとして出力
                                    const auto write_grd_result = writeGRD(grd.value(), path);
                                    if (!write_grd_result) {
                                        m_logger_wrapper->error("{}", write_grd_result.error());
                                    }

                                    m_logger_wrapper->log("Successfully exported GRD file: {}", path.string());
                                    break;
                                }
                                case FileDialogResult::FD_CANCEL: {
                                    break;
                                }
                                case FileDialogResult::FD_ERROR: {
                                    auto error_msg = std::get<static_cast<int32_t>(FileDialogResult::FD_ERROR)>(open_file_dialog_result.value);
                                    m_logger_wrapper->error(L"{}", error_msg);
                                    break;
                                }
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(m_config_wrapper->tr(L"表示").c_str())) {
            ImGui::MenuItem(m_config_wrapper->tr(L"プリセット").c_str(), nullptr, &m_window_visible.preset_window);
            ImGui::MenuItem(m_config_wrapper->tr(L"履歴").c_str(), nullptr, &m_window_visible.history_window);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(m_config_wrapper->tr(L"設定").c_str())) {
            if (ImGui::MenuItem(m_config_wrapper->tr(L"UIの設定").c_str(), nullptr)) {
                open_style_settings_popup = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    if (open_style_settings_popup) {
        ImGui::OpenPopup(settings_popup_name.c_str());
    }

    static ImGuiWindowFlags modal_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
    ImVec2 center                       = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    static float old_font_scale_main = ImGui::GetStyle().FontScaleMain;

    if (ImGui::BeginPopupModal(settings_popup_name.c_str(), nullptr, modal_flags)) {
        if (ImGui::IsWindowAppearing()) {
            old_font_scale_main = ImGui::GetStyle().FontScaleMain;
        }

        static constexpr float ITEM_SPACING_SCALE_Y = 0.25f;
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(m_config_wrapper->tr(L"UIのサイズ").c_str());
        ImGui::SameLine();
        static float font_scale_main = old_font_scale_main;
        if (ImGui::DragFloat("##ui_size", &font_scale_main, 0.02f, 0.5f, 4.0f, "%.2f")) {
            font_scale_main = std::clamp(font_scale_main, 0.5f, 4.0f);
            ;
            ImGui::GetStyle().FontScaleMain = font_scale_main;
        }

        ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight() * ITEM_SPACING_SCALE_Y));

        float button_width          = ImGui::GetFrameHeight() * 4;
        float combined_button_width = button_width * 2 + ImGui::GetStyle().ItemSpacing.x * 1;
        float avail                 = ImGui::GetContentRegionAvail().x;
        float off                   = (avail - combined_button_width) * 0.5f;
        if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

        if (ImGui::Button(m_config_wrapper->tr(L"キャンセル").c_str(), ImVec2(button_width, 0))) {
            font_scale_main                 = old_font_scale_main;
            ImGui::GetStyle().FontScaleMain = old_font_scale_main;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button(m_config_wrapper->tr(L"OK").c_str(), ImVec2(button_width, 0))) {
            gradient_editor::g_app_state.settings.ui_scale = static_cast<uint32_t>(font_scale_main * 100);
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    static ImGuiWindowClass side_window_class;
    side_window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoUndocking;

    // プリセットウィンドウを描画
    if (m_window_visible.preset_window) {
        ImGui::SetNextWindowClass(&side_window_class);
        m_preset_window.render(m_config_manager, m_preset_config);
    }

    // 履歴ウィンドウを描画
    if (m_window_visible.history_window) {
        ImGui::SetNextWindowClass(&side_window_class);
        m_history_window.render(m_config_manager, m_history_config);
    }

    // 初回はプリセットウィンドウにフォーカスを合わせる
    if (!m_is_init) {
        ImGui::SetWindowFocus("###preset_window");
    }

    // グラデーションエディターを描画
    renderGradientEditor();

    ImGui::End();

    m_is_init = true;
}

void MainView::renderGradientEditor()
{
    ImGui::Begin("###gradient_editor_window");

    float frame_height = ImGui::GetFrameHeight();

    static GradientData replace_data;
    if (!m_is_init && !m_history_window.m_history_data.empty()) {
        replace_data = m_history_window.m_history_data.back().data;
    }

    //
    // スクリプト選択
    //
    static std::vector<std::string> effect_names_vec = []() {
        std::vector<std::string> res;
        for (auto name : EFFECT_NAMES) res.push_back(str_conv::wideCharToMultiByte(name));
        return res;
    }();

    bool is_changed_section_effect = false;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(m_config_wrapper->tr(L"対象").c_str());
    ImGui::SameLine();
    if (ImGui::BeginCombo("##スクリプト", effect_names_vec[m_effect_name_index].c_str(), ImGuiComboFlags_WidthFitPreview)) {
        for (uint32_t i = 0; i < effect_names_vec.size(); ++i) {
            if (ImGui::Selectable(effect_names_vec[i].c_str(), m_effect_name_index == i)) {
                is_changed_section_effect = true;
                m_effect_name_index       = i;
            }
        }
        ImGui::EndCombo();
    }
    std::wstring effect_full_name = std::wstring(EFFECT_NAMES[m_effect_name_index]) + EFFECT_GROUP_NAME;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"編集対象のスクリプト名").c_str());

    bool is_changed_effect_index = false;
    ImGui::SameLine();
    ImGui::PushItemWidth(frame_height * scale::relative::EFFECT_INDEX_SPIN_WIDTH);
    if (imgui_utils::spinInt("##effect_index", &m_effect_index)) {
        is_changed_effect_index = true;
        int32_t count           = 0;
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            if (auto obj = edit->get_focus_object()) count = edit->count_object_effect(obj, effect_full_name.c_str());
        });
        m_effect_index = std::clamp(m_effect_index, 0, count == 0 ? 0 : count - 1);
    }
    ImGui::PopItemWidth();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"編集対象のスクリプトのインデックス").c_str());

    //
    // 各種データ操作ボタン
    //
    bool create_new_object = false;
    if (ImGui::Button(m_config_wrapper->tr(L"新規").c_str())) {
        const char* alias = reinterpret_cast<const char*>(NEW_OBJECT_ALIAS_TAMPLATES[m_effect_name_index]);
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            auto obj = edit->create_object_from_alias(alias, edit->info->layer, edit->info->frame, NEW_OBJECT_LENGTH);
            if (!obj) {
                return;
            }

            edit->set_focus_object(obj);
            create_new_object = true;
        });
    }

    // スクリプトへ反映
    ImGui::SameLine();
    bool off_to_on          = false;
    bool create_new_object_ = create_new_object;
    if (imgui_utils::pushToggleButton(m_config_wrapper->tr(L"反映").c_str(), &m_apply, &create_new_object_)) {
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            auto focus_obj = edit->get_focus_object();
            if (!focus_obj) return;
            m_layer_frame = edit->get_object_layer_frame(focus_obj);
        });
        if (m_apply) off_to_on = true;
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"スクリプトへ値を反映").c_str());
    if (m_apply) m_load = false;

    // スクリプトから読み込む
    ImGui::SameLine(0, 0);
    m_load = ImGui::Button(m_config_wrapper->tr(L"読込").c_str());
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"スクリプトから値を読み込む").c_str());

    // 再読み込み
    ImGui::SameLine();
    bool is_refresh = imgui_utils::squareIconButton(ICON_MS_REFRESH, "##refresh");
    if (is_refresh) {
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            if (auto obj = edit->get_focus_object()) m_layer_frame = edit->get_object_layer_frame(obj);
        });
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"選択オブジェクトの再読み込み").c_str());

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text((m_config_wrapper->tr(L"レイヤー") + "=%d, " + m_config_wrapper->tr(L"フレーム") + "=[%d - %d]").c_str(), m_layer_frame.layer + 1, m_layer_frame.start + 1, m_layer_frame.end + 1);

    bool is_reset_alpha_marker = imgui_utils::squareIconButton(ICON_MS_SYNC, "##alpha_marker_reset");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"マーカーをリセット").c_str());
    ImGui::SameLine();

    float alpha_tool_buttons_width = frame_height * 7 + ImGui::GetStyle().ItemSpacing.x * 5;
    imgui_utils::alignForWidth(alpha_tool_buttons_width, 1.0f);  // 右揃えにする

    bool select_back_alpha_marker = imgui_utils::squareIconButton(ICON_MS_ARROW_BACK_2, "##select_back_alpha_marker");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"前のマーカーを選択").c_str());
    ImGui::SameLine(0, 0);
    bool select_next_alpha_marker = imgui_utils::squareIconButton(ICON_MS_PLAY_ARROW, "##select_next_alpha_marker");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"次のマーカーを選択").c_str());
    ImGui::SameLine();
    bool is_distribute_alpha_marker = imgui_utils::squareIconButton(ICON_MS_ARROW_RANGE, "##alpha_marker_distribute");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"マーカーを等間隔に配置").c_str());
    ImGui::SameLine();
    bool is_distribute_alpha_marker_and_alpha_midpoint = imgui_utils::squareIconButton(ICON_MS_FORMAT_LETTER_SPACING, "##alpha_distribut_bothe");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"マーカーと中間点を等間隔に配置").c_str());
    ImGui::SameLine();
    bool is_reset_alpha_midpoint = imgui_utils::squareIconButton(ICON_MS_STAT_0, "##alpha_reset_midpoints");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"すべての中間点を中央に再配置").c_str());
    ImGui::SameLine();
    bool is_reverse_alpha_marker = imgui_utils::squareIconButton(ICON_MS_SWITCH_LEFT, "##alpha_marker_reverse");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"マーカーを反転").c_str());
    ImGui::SameLine();
    bool is_delete_alpha_marker = imgui_utils::squareIconButton(ICON_MS_DELETE, "##alpha_marker_delete");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"選択中のマーカーを削除").c_str());

    if (off_to_on || (m_apply && is_refresh)) {
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            auto focus_obj = edit->get_focus_object();
            if (!focus_obj) return;

            m_layer_frame = edit->get_object_layer_frame(focus_obj);

            // 反映は ON だが対象のエフェクトが付いていない場合
            auto effect_count = edit->count_object_effect(focus_obj, effect_full_name.c_str());
            if (effect_count == 0 && m_apply) {
                auto alias   = edit->get_object_alias(focus_obj);
                auto obj_idx = alias_parser::getLastObjectIndex(alias);
                if (!obj_idx) return;

                auto object0_effect_name = alias_parser::getEffectName(alias);
                if (!object0_effect_name) return;

                static constexpr const char* EXCLUDE_EFFECTS[6] = {
                    "オーディオバッファ",
                    "カメラ制御",
                    "グループ制御(音声)",
                    "フィルタ効果",
                    "音声ファイル",
                    "時間制御(オブジェクト)",
                };
                for (const auto e : EXCLUDE_EFFECTS) {
                    if (object0_effect_name == e) return;
                }

                uint32_t next_index = obj_idx.value() + 1;
                std::string new_alias{};
                try {
                    new_alias = std::vformat(SCRIPT_TAMPLATES[m_effect_name_index], std::make_format_args(next_index));
                } catch (const std::format_error e) {
                    m_logger_wrapper->error("{}", e.what());
                }

                new_alias = alias + new_alias;

                edit->delete_object(focus_obj);
                auto new_obj = edit->create_object_from_alias(
                    new_alias.c_str(),
                    m_layer_frame.layer,
                    m_layer_frame.start,
                    m_layer_frame.end - m_layer_frame.start);
                if (!new_obj) return;

                edit->set_focus_object(new_obj);
            }
        });
    }

    //
    // グラデーションエディターの描画
    //
    ImGui::Dummy(ImVec2(0, frame_height * scale::relative::GRADIENT_MARGIN_Y));

    // 描画設定
    static custom_ui::GradientEditorConfig config;
    config.max_marker_count = MAX_MARKER_COUNT;
    config.marker_size      = {frame_height * scale::relative::GRADIENT_MARKER_WIDTH, frame_height * scale::relative::GRADIENT_MARKER_HEIGHT};
    config.midpoint_size    = {frame_height * scale::relative::GRADIENT_MIDPOINT_WIDTH, frame_height * scale::relative::GRADIENT_MIDPOINT_WIDTH};

    // プリセット、履歴がクリックされたときはそのグラデーションを使用する
    if (m_preset_window.isPresetClicked()) {
        if (m_data != nullptr) {
            m_history_window.pushHistory(*m_data);
        }
        replace_data = m_preset_window.getSelectedGradientData();
    } else if (m_history_window.isHistoryClicked()) {
        replace_data = m_history_window.getSelectedGradient();
    }

    bool should_replace = m_preset_window.isPresetClicked() || m_history_window.isHistoryClicked() || (!m_is_init && !m_history_window.m_history_data.empty());
    m_data              = custom_ui::drawGradientEditor(
        "gradient",
        ImVec2(std::clamp(ImGui::GetContentRegionAvail().x, 1.0f, 4096.0f), frame_height * scale::relative::GRADIENT_HEIGHT),
        replace_data,
        custom_ui::GradientEditorFlags_None |
            custom_ui::GradientEditorFlags_AlphaMarker,
        should_replace,
        config);

    ImGui::Dummy(ImVec2(0, frame_height * scale::relative::GRADIENT_MARGIN_Y));
    m_preset_window.setTargetGradientData(*m_data);

    // 更新
    m_script_bridge.update(*m_data);

    // カラーピッカーポップアップ
    if (m_data->getColorMarkers()->isDoubleClickedMarker(ImGui::GetIO().MousePos)) {
        m_popup_previous_color = m_popup_current_color;
        m_popup_current_color  = m_data->getColorMarkers()->getSelectedMarkerValue();
        ImGui::OpenPopup(COLOR_PICKER_POPUP_ID);
    }
    if (colorPickerPopup(COLOR_PICKER_POPUP_ID, m_popup_current_color, m_popup_previous_color)) {
        m_data->getColorMarkers()->setSelectedMarkerValue(m_popup_current_color);
    }

    // アルファスライダーポップアップ
    static float current_alpha{1.0f};
    if (m_data->getAlphaMarkers()->isDoubleClickedMarker(ImGui::GetIO().MousePos)) {
        current_alpha = m_data->getAlphaMarkers()->getSelectedMarkerValue().w;
        ImGui::OpenPopup("alpha_slider_popup2");
    }
    if (ImGui::BeginPopup("alpha_slider_popup2")) {
        if (ImGui::DragFloat("##alpha_value", &current_alpha, 0.01f, 0.0f, 1.0f, "%.2f")) {
            float alpha = std::clamp(current_alpha, 0.0f, 1.0f);
            m_data->getAlphaMarkers()->setSelectedMarkerValue(ImVec4(0, 0, 0, alpha));
        }
        ImGui::EndPopup();
    }

    //
    // 各種ツールボタン
    //
    bool is_reset_all = imgui_utils::squareIconButton(ICON_MS_SYNC, "##reset");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"マーカーをリセット").c_str());
    ImGui::SameLine();

    constexpr int32_t tool_icon_num = 7;
    float tb_width                  = frame_height * tool_icon_num + ImGui::GetStyle().ItemSpacing.x * (tool_icon_num - 2);
    imgui_utils::alignForWidth(tb_width, 1.0f);  // 右揃えにする

    bool select_back_marker = imgui_utils::squareIconButton(ICON_MS_ARROW_BACK_2, "##left");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"前のマーカーを選択").c_str());
    ImGui::SameLine(0, 0);
    bool select_next_marker = imgui_utils::squareIconButton(ICON_MS_PLAY_ARROW, "##right");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"次のマーカーを選択").c_str());
    ImGui::SameLine();
    bool is_distribute_marker = imgui_utils::squareIconButton(ICON_MS_ARROW_RANGE, "##distribute");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"マーカーを等間隔に配置").c_str());
    ImGui::SameLine();
    bool is_distribute_marker_and_midpoint = imgui_utils::squareIconButton(ICON_MS_FORMAT_LETTER_SPACING, "##distribut_bothe");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"マーカーと中間点を等間隔に配置").c_str());
    ImGui::SameLine();
    bool is_reset_midpoint = imgui_utils::squareIconButton(ICON_MS_STAT_0, "##reset_midpoints");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"すべての中間点を中央に再配置").c_str());
    ImGui::SameLine();
    bool is_reverse = imgui_utils::squareIconButton(ICON_MS_SWITCH_LEFT, "##reverse");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"マーカーを反転").c_str());
    ImGui::SameLine();
    bool is_delete_marker = imgui_utils::squareIconButton(ICON_MS_DELETE, "##delete");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", m_config_wrapper->tr(L"選択中のマーカーを削除").c_str());

    // カラーマーカーの操作
    if (is_reset_all) {
        if (m_data != nullptr) {
            m_history_window.pushHistory(*m_data);  // リセット前の状態を保存
        }

        m_data->getColorMarkers()->resetMarkers(m_data->m_default_color_markers);
        m_data->setColorSpace(0);
        m_data->setInterpDir(0);
        m_data->setColorBlurWidth(1.0f);

        // 履歴を設定ファイルに書き出す
        if (m_data != nullptr) {
            m_history_window.writeHistoryToConfig(m_config_manager, m_history_config);
        }
    }
    if (select_back_marker) m_data->getColorMarkers()->selectBackMarker();
    if (select_next_marker) m_data->getColorMarkers()->selectNextMarker();
    if (is_distribute_marker) m_data->getColorMarkers()->distributeMarkersEvenly();
    if (is_distribute_marker_and_midpoint) m_data->getColorMarkers()->distributeMarkersAndMipointsEvenly();
    if (is_reset_midpoint) m_data->getColorMarkers()->resetMidpoints();
    if (is_reverse) m_data->getColorMarkers()->reverseMarkers();
    if (is_delete_marker) m_data->getColorMarkers()->deleteSelectedMarker();

    // アルファマーカーの操作
    if (is_reset_alpha_marker) {
        if (m_data != nullptr) {
            m_history_window.pushHistory(*m_data);
        }

        m_data->getAlphaMarkers()->resetMarkers(m_data->m_default_alpha_markers);
        m_data->setAlphaBlurWidth(1.0f);

        if (m_data != nullptr) {
            m_history_window.writeHistoryToConfig(m_config_manager, m_history_config);
        }
    }
    if (select_back_alpha_marker) m_data->getAlphaMarkers()->selectBackMarker();
    if (select_next_alpha_marker) m_data->getAlphaMarkers()->selectNextMarker();
    if (is_distribute_alpha_marker) m_data->getAlphaMarkers()->distributeMarkersEvenly();
    if (is_distribute_alpha_marker_and_alpha_midpoint) m_data->getAlphaMarkers()->distributeMarkersAndMipointsEvenly();
    if (is_reset_alpha_midpoint) m_data->getAlphaMarkers()->resetMidpoints();
    if (is_reverse_alpha_marker) m_data->getAlphaMarkers()->reverseMarkers();
    if (is_delete_alpha_marker) m_data->getAlphaMarkers()->deleteSelectedMarker();

    // プリセットがクリックされた場合、現在のグラデーションがプリセットのものに置き換わるため、
    // その時のグラデーションのデータを差分検知のために保存しておく
    if (m_preset_window.isPresetClicked()) {
        m_script_bridge.setValues(*m_data);
    }

    // スクリプトからグラデーションエディターに値を読み込む
    if (m_load) {
        m_load = false;
        m_history_window.pushHistory(*m_data);  // 置き換える前のグラデーションデータを履歴に保存

        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            OBJECT_HANDLE object_handle = edit->get_focus_object();
            if (!object_handle) return;

            int32_t effect_count = edit->count_object_effect(object_handle, effect_full_name.c_str());
            if (effect_count <= 0 || m_effect_index >= effect_count) {
                return;
            }

            m_script_bridge.loadGradientFromScript(edit, *m_data, effect_full_name, m_effect_index, m_target_move_index);
        });
    }

    // グラデーションエディターからスクリプトへ値を反映するかどうかのフラグ
    bool is_changed_apply =
        off_to_on ||                                           // 「反映」が OFF から ON に切り替わった
        create_new_object ||                                   // オブジェクトが新たに作成された
        (m_apply && (                                          // または「反映」ON の状態で、
                        m_preset_window.isPresetClicked() ||   // プリセットがクリックされた
                        is_refresh ||                          // 更新ボタンが押された
                        is_changed_section_effect ||           // 対象とするスクリプトが変更された
                        is_changed_effect_index ||             // 同じスクリプトが複数ある際、対象とするスクリプトのインデックスが変更された
                        is_reverse || is_reverse_alpha_marker  // マーカー反転のボタンが押された
                        ));
    // または各値がグラデーションエディター側で変更された
    // マーカーの削除、リセット、均等配置による変更は isChangedValues() で検知できる
    if (is_changed_apply || (m_apply && m_script_bridge.getIsChangedValues())) {
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            m_script_bridge.applyGradientToScript(edit, *m_data, effect_full_name, m_effect_index, m_target_move_index);
        });
    }

    // プロパティエディタを描画する
    bool draw_color_peditor        = false;
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("##PropertyEditorTabBar", tab_bar_flags)) {
        if (ImGui::BeginTabItem(m_config_wrapper->tr(L"色設定").c_str())) {
            draw_color_peditor = true;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(m_config_wrapper->tr(L"アルファ設定").c_str())) {
            renderAlphaPropertyEditor(m_data);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // 内部でポップアップを呼び出す処理があるため外に出す
    if (draw_color_peditor) {
        renderColorPropertyEditor(m_data);
    }

    ImGui::End();
}

void MainView::renderColorPropertyEditor(GradientData* data)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_SeparatorActive, ImVec4(0, 0, 0, 0));

    float height      = ImGui::GetFrameHeight() * 6 + ImGui::GetStyle().ItemSpacing.y * 5;
    float label_width = ImGui::GetFrameHeight() * scale::relative::ITEM_NAME_BUTTON_WIDTH;

    ImGui::BeginChild("##color_item_labels", ImVec2(label_width, height), ImGuiChildFlags_ResizeX, ImGuiWindowFlags_NoScrollbar);
    {
        ImVec2 size(ImGui::GetWindowSize().x, ImGui::GetFrameHeight());
        auto& colors = ImGui::GetStyle().Colors;
        ImGui::PushStyleColor(ImGuiCol_Button, colors[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors[ImGuiCol_Button]);
        ImGui::Button(m_config_wrapper->tr(L"色").c_str(), size);
        ImGui::Button(m_config_wrapper->tr(L"位置").c_str(), size);
        ImGui::Button(m_config_wrapper->tr(L"中間点").c_str(), size);
        ImGui::Button(m_config_wrapper->tr(L"ぼかし幅").c_str(), size);
        ImGui::Button(m_config_wrapper->tr(L"色空間").c_str(), size);
        ImGui::Button(m_config_wrapper->tr(L"補間経路").c_str(), size);
        ImGui::PopStyleColor(3);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        auto curr    = m_script_bridge.getValues();
        float col[4] = {curr.selected_color.x, curr.selected_color.y, curr.selected_color.z, curr.selected_color.w};
        float width  = ImGui::GetContentRegionAvail().x;
        float btn_sz = ImGui::GetFrameHeight();

        ImGui::SetNextItemWidth(width - ImGui::GetStyle().ItemInnerSpacing.x - btn_sz);
        bool click_edit = ImGui::ColorEdit4("##selected_color", col, ImGuiColorEditFlags_NoSmallPreview);
        ImVec4 new_col(col[0], col[1], col[2], col[3]);
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        bool click_btn = ImGui::ColorButton("##marker_color_button", new_col, ImGuiColorEditFlags_NoTooltip);

        if (click_edit) {
            m_data->getColorMarkers()->setSelectedMarkerValue(new_col);
        }
        if (click_btn) {
            m_popup_previous_color = m_popup_current_color;
            m_popup_current_color  = m_data->getColorMarkers()->getSelectedMarkerValue();
            ImGui::OpenPopup(COLOR_PICKER_POPUP_ID);
        }

        ImGui::SetNextItemWidth(width);
        float pos = curr.selected_marker_pos * 100.0f;
        if (ImGui::DragFloat("##marker_pos", &pos, 0.01f, 0.0f, 100.0f, "%.2f")) {
            data->getColorMarkers()->setSelectedMarkerPosition(pos / 100.0f);
        }

        ImGui::SetNextItemWidth(width);
        float mid = curr.selected_midpoint_ratio * 100.0f;
        if (ImGui::DragFloat("##midpoint_ratio", &mid, 0.01f, 0.0f, 100.0f, "%.2f")) {
            data->getColorMarkers()->setSelectedMidpointRatio(mid / 100.0f);
        }

        ImGui::SetNextItemWidth(width);
        float blur = curr.blur_width * 100.0f;
        if (ImGui::DragFloat("##blur_width", &blur, 0.1f, 0.0f, 100.0f, "%.0f")) {
            data->setColorBlurWidth(blur / 100.0f);
        }

        ImGui::SetNextItemWidth(width);
        if (ImGui::BeginCombo("##color_space", COLOR_SPACE_NAMES[curr.color_space_index])) {
            for (uint32_t i = 0; i < IM_ARRAYSIZE(COLOR_SPACE_NAMES); i++) {
                if (ImGui::Selectable(COLOR_SPACE_NAMES[i], curr.color_space_index == i)) data->setColorSpace(i);
            }
            ImGui::EndCombo();
        }

        ImGui::SetNextItemWidth(width);
        if (ImGui::BeginCombo("##interp_dir", m_config_wrapper->tr(str_conv::multiByteToWideChar(INTERP_DIR_NAMES[curr.interp_dir_index]).c_str()).c_str())) {
            for (uint32_t i = 0; i < IM_ARRAYSIZE(INTERP_DIR_NAMES); i++) {
                if (ImGui::Selectable(m_config_wrapper->tr(str_conv::multiByteToWideChar(INTERP_DIR_NAMES[i]).c_str()).c_str(), curr.interp_dir_index == i))
                    data->setInterpDir(i);
            }
            ImGui::EndCombo();
        }
    }
    ImGui::EndGroup();
}

void MainView::renderAlphaPropertyEditor(GradientData* data)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_SeparatorActive, ImVec4(0, 0, 0, 0));

    float height      = ImGui::GetFrameHeight() * 4 + ImGui::GetStyle().ItemSpacing.y * 3;
    float label_width = ImGui::GetFrameHeight() * scale::relative::ITEM_NAME_BUTTON_WIDTH;

    ImGui::BeginChild("##alpha_item_labels", ImVec2(label_width, height), ImGuiChildFlags_ResizeX, ImGuiWindowFlags_NoScrollbar);
    {
        ImVec2 size(ImGui::GetWindowSize().x, ImGui::GetFrameHeight());
        auto& colors = ImGui::GetStyle().Colors;
        ImGui::PushStyleColor(ImGuiCol_Button, colors[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors[ImGuiCol_Button]);
        ImGui::Button(m_config_wrapper->tr(L"アルファ値").c_str(), size);
        ImGui::Button(m_config_wrapper->tr(L"位置").c_str(), size);
        ImGui::Button(m_config_wrapper->tr(L"中間点").c_str(), size);
        ImGui::Button(m_config_wrapper->tr(L"ぼかし幅").c_str(), size);
        ImGui::PopStyleColor(3);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        auto curr   = m_script_bridge.getValues();
        float width = ImGui::GetContentRegionAvail().x;

        ImGui::SetNextItemWidth(width);
        float value = curr.selected_alpha_marker_value * 100.0f;
        if (ImGui::DragFloat("##alpha_marker_value", &value, 0.01f, 0.0f, 100.0f, "%.2f")) {
            data->getAlphaMarkers()->setSelectedMarkerValue(ImVec4(0, 0, 0, value / 100.0f));
        }

        ImGui::SetNextItemWidth(width);
        float pos = curr.selected_alpha_marker_pos * 100.0f;
        if (ImGui::DragFloat("##alpha_marker_pos", &pos, 0.01f, 0.0f, 100.0f, "%.2f")) {
            data->getAlphaMarkers()->setSelectedMarkerPosition(pos / 100.0f);
        }

        ImGui::SetNextItemWidth(width);
        float midpoint = curr.selected_alpha_midpoint_ratio * 100.0f;
        if (ImGui::DragFloat("##alpha_marker_midpoint", &midpoint, 0.01f, 0.0f, 100.0f, "%.2f")) {
            data->getAlphaMarkers()->setSelectedMidpointRatio(midpoint / 100.0f);
        }

        ImGui::SetNextItemWidth(width);
        float blur = curr.alpha_blur_width * 100.0f;
        if (ImGui::DragFloat("##alpha_blur_width", &blur, 0.1f, 0.0f, 100.0f, "%.0f")) {
            data->setAlphaBlurWidth(blur / 100.0f);
        }
    }
    ImGui::EndGroup();
}

void MainView::writeHistories()
{
    m_history_window.pushHistory(*m_data);  // この関数が呼ばれた時点のグラデーションを履歴に保存する
    m_history_window.writeHistoryToConfig(m_config_manager, m_history_config);
    m_preset_window.writeSelectedCategoryToConfig(m_config_manager, m_preset_config);
}
