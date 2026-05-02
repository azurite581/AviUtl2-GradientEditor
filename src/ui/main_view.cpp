#include "main_view.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

#include "IconsMaterialSymbols.h"
#include "color_conv.h"
#include "constants.h"
#include "gradient_widget.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_utils.h"
#include "plugin2_utils.h"
#include "str_conv.h"

MainView::MainView(LoggerWrapperInterface* logger_wrapper, ConfigWrapperInterface* config_wrapper)
    : m_logger_wrapper{logger_wrapper}, m_config_wrapper{config_wrapper}
{
    // std::cout の出力先をファイルにリダイレクトする（デバッグ用）
    //std::ofstream file("log.txt");
    //std::cout.rdbuf(file.rdbuf());

    m_preset_window.setLoggerWrapper(m_logger_wrapper);
    m_preset_window.setConfigWrapper(m_config_wrapper);
    m_history_window.setLoggerWrapper(m_logger_wrapper);
    m_history_window.setConfigWrapper(m_config_wrapper);
    m_script_bridge.setLoggerWrapper(m_logger_wrapper);

    std::filesystem::path data_path = gradient_editor::g_app_state.config_handle->app_data_path;
    std::filesystem::path config_folder_path = data_path / L"Plugin" / CONFIG_FOLDER_NAME;

    // プリセットフォルダがなければ作成
    if (!std::filesystem::exists(config_folder_path) || !std::filesystem::is_directory(config_folder_path)) {
        std::filesystem::create_directories(config_folder_path);
    }

    // プリセットファイルがなければ作成
    std::filesystem::path preset_path = config_folder_path / PRESET_FILE_NAME;
    if (!std::filesystem::exists(preset_path)) {
        std::error_code ec;

        bool is_copied = std::filesystem::copy_file(
            config_folder_path / L"gradient_editor_default_preset.json",
            preset_path,
            std::filesystem::copy_options::skip_existing,
            ec
        );

        // コピー元ファイルが存在しない、またはコピーに失敗した場合
        if (!is_copied) {
            m_logger_wrapper->warn("Copy skipped or failed: {}", ec.message());

            std::ofstream ofs(preset_path, std::ios::binary);
            if (ofs.is_open()) {
                ofs << GradientConfigManager::DEFAULT_PRESET_FILE_JSON;
            } else {
                m_logger_wrapper->error("Failed to create preset file at: {}", preset_path.string());
            }
        }
    }

    // プリセットを読み込む
    m_config_manager.setPresetFilePath(preset_path);
    auto preset_load_result = m_config_manager.loadPresetConfig();
    if (!preset_load_result.is_success()) {
        m_logger_wrapper->error("{}", preset_load_result.error.c_str());  // エラーメッセージに '{' または '}' があると std::format_error になるため "{}" で受け取る
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
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu(m_config_wrapper->tr(L"表示").c_str())) {
            ImGui::MenuItem(m_config_wrapper->tr(L"プリセット").c_str(), nullptr, &m_window_visible.preset_window);
            ImGui::MenuItem(m_config_wrapper->tr(L"履歴").c_str(), nullptr, &m_window_visible.history_window);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
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

    // グラデーションエディタを描画
    renderGradientEditor();

    ImGui::End();

    m_is_init = true;
}

void MainView::renderGradientEditor()
{
    ImGui::Begin("###gradient_editor_window");

    static float frame_height = ImGui::GetFrameHeight();

    static GradientData replace_data;
    if (!m_is_init && !m_history_window.m_history_data.empty()) {
        replace_data = m_history_window.m_history_data.back().data;
    }

    //
    // セクション選択
    //
    bool is_changed_section = false;
    ImGui::AlignTextToFramePadding();
    ImGui::Text(m_config_wrapper->tr(L"セクション").c_str());
    ImGui::SameLine();

    if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
        is_changed_section = true;
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            OBJECT_HANDLE obj = edit->get_focus_object();
            if (obj) {
                if (auto* alias = edit->get_object_alias(obj))
                    m_frame_count = alias_parser::getFrameCount(alias);
            }
        });
        m_target_move_index = std::clamp(m_target_move_index - 1, 0, m_frame_count - 1);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"前のセクションに移動").c_str());

    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
    if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
        is_changed_section = true;
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            OBJECT_HANDLE obj = edit->get_focus_object();
            if (obj) {
                if (auto* alias = edit->get_object_alias(obj))
                    m_frame_count = alias_parser::getFrameCount(alias);
            }
        });
        m_target_move_index = std::clamp(m_target_move_index + 1, 0, m_frame_count - 1);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"次のセクションに移動").c_str());

    static bool is_refresh = false;
    if (is_refresh) {
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            OBJECT_HANDLE obj = edit->get_focus_object();
            if (obj) {
                if (auto* alias = edit->get_object_alias(obj))
                    m_frame_count = alias_parser::getFrameCount(alias);
            }
        });
    }

    ImGui::SameLine();
    float avail_width = ImGui::GetContentRegionAvail().x;
    if (avail_width > 0) {
        ImVec2 p0      = ImGui::GetCursorScreenPos();
        ImVec2 p1      = ImVec2(p0.x + avail_width, p0.y + frame_height);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_TitleBg]));
        float p0_y = p0.y + frame_height * 0.4f;
        dl->AddRectFilledMultiColor(ImVec2(p0.x, p0_y), p1, m_object_video_color_start, m_object_video_color_stop, m_object_video_color_stop, m_object_video_color_start);

        for (int32_t i = 0; i < m_frame_count; ++i) {
            float y0    = (i == 0 || i == m_frame_count - 1) ? p0.y : p0_y;
            float ratio = m_frame_count == 1 ? 0.0f : (i / static_cast<float>(m_frame_count - 1));
            ImVec2 lp0(p0.x + ratio * avail_width, y0);
            ImVec2 lp1(lp0.x, p1.y);
            ImU32 col = (i == m_target_move_index) ? m_frame_cursor_color : ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Border]);
            dl->AddLine(lp0, lp1, col, 1.0f);
        }
    }
    ImGui::Dummy(ImVec2(0.0f, 0.0f));

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
    ImGui::Text(m_config_wrapper->tr(L"対象").c_str());
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
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"編集対象のスクリプト名").c_str());

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
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"編集対象のスクリプトのインデックス").c_str());

    //
    // 各種データ操作ボタン
    //
    // スクリプトへ反映
    bool off_to_on = false;
    if (imgui_utils::pushToggleButton(m_config_wrapper->tr(L"反映").c_str(), &m_apply)) {
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            if (auto obj = edit->get_focus_object()) m_layer_frame = edit->get_object_layer_frame(obj);
        });
        if (m_apply) off_to_on = true;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"スクリプトへ値を反映").c_str());
    if (m_apply) m_load = false;

    // スクリプトから読み込む
    ImGui::SameLine(0, 0);
    m_load = ImGui::Button(m_config_wrapper->tr(L"読込").c_str());
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"スクリプトから値を読み込む").c_str());

    // 再読み込み
    ImGui::SameLine();
    is_refresh = imgui_utils::squareIconButton(ICON_MS_REFRESH, "##refresh");
    if (is_refresh) {
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            if (auto obj = edit->get_focus_object()) m_layer_frame = edit->get_object_layer_frame(obj);
        });
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"選択オブジェクトの再読み込み").c_str());

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text((m_config_wrapper->tr(L"レイヤー") + "=%d, " + m_config_wrapper->tr(L"フレーム") + "=[%d - %d]").c_str(), m_layer_frame.layer + 1, m_layer_frame.start + 1, m_layer_frame.end + 1);

    //
    // グラデーションエディタの描画
    //
    ImGui::Dummy(ImVec2(0, frame_height * scale::relative::GRADIENT_MARGIN_Y));

    // 描画設定
    static CustomUI::GradientEditorConfig config;
    config.max_marker_count = MAX_MARKER_COUNT;
    config.marker_width     = frame_height * scale::relative::GRADIENT_MARKER_WIDTH;
    config.io_enable        = !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);  // ポップアップが開いているときはマーカーの操作を受け付けない

    // プリセット、履歴がクリックされたときはそのグラデーションを使用する
    if (m_preset_window.isPresetClicked()) {
        if (m_data != nullptr) m_history_window.pushHistory(*m_data);
        replace_data = m_preset_window.getSelectedGradientData();
    } else if (m_history_window.isHistoryClicked()) {
        replace_data = m_history_window.getSelectedGradient();
    }

    bool should_replace = m_preset_window.isPresetClicked() || m_history_window.isHistoryClicked() || (!m_is_init && !m_history_window.m_history_data.empty());
    m_data              = CustomUI::drawGradientEditor(
        "gradient",
        ImVec2(std::clamp(ImGui::GetContentRegionAvail().x, 1.0f, 4096.0f), frame_height * scale::relative::GRADIENT_HEIGHT),
        replace_data,
        CustomUI::GradientEditorFlags_None | CustomUI::GradientEditorFlags_MidpointBelowGradient,
        should_replace,
        config);

    ImGui::Dummy(ImVec2(0, frame_height * scale::relative::GRADIENT_MARGIN_Y));
    m_preset_window.setTargetGradientData(*m_data);

    // 更新
    m_script_bridge.update(*m_data);

    //
    // 各種ツールボタン
    //
    bool is_reset_all = imgui_utils::squareIconButton(ICON_MS_SYNC, "##reset");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"リセット").c_str());
    ImGui::SameLine();

    float tb_width = frame_height * 5 + ImGui::GetStyle().ItemSpacing.x * 4;
    imgui_utils::alignForWidth(tb_width, 1.0f);  // 右揃えにする
    bool is_distribute_marker = imgui_utils::squareIconButton(ICON_MS_ARROW_RANGE, "##distribute");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"マーカーを等間隔に配置").c_str());
    ImGui::SameLine();
    bool is_distribute_marker_and_midpoint = imgui_utils::squareIconButton(ICON_MS_FORMAT_LETTER_SPACING, "##distribut_bothe");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"マーカーと中間点を等間隔に配置").c_str());
    ImGui::SameLine();
    bool is_reset_midpoint = imgui_utils::squareIconButton(ICON_MS_STAT_0, "##reset_midpoints");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"すべての中間点を中央に再配置").c_str());
    ImGui::SameLine();
    bool is_reverse = imgui_utils::squareIconButton(ICON_MS_SWITCH_LEFT, "##reverse");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"マーカーを反転").c_str());
    ImGui::SameLine();
    bool is_del = imgui_utils::squareIconButton(ICON_MS_DELETE, "##delete");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip(m_config_wrapper->tr(L"選択中のマーカーを削除").c_str());

    if (is_distribute_marker) m_data->getMarkerManager()->distributeMarkersEvenly();
    if (is_distribute_marker_and_midpoint) m_data->getMarkerManager()->distributeMarkersAndMipointsEvenly();
    if (is_reset_all) {
        m_data->getMarkerManager()->setDefaultMarkers();
        m_data->setColorSpace(0);
        m_data->setInterpDir(0);
        m_data->setBlurWidth(1.0f);
        if (m_apply) {
            plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
                m_script_bridge.resetScriptData(edit, static_cast<uint32_t>(m_data->getMarkerManager()->getMarkers().size()), MAX_MARKER_COUNT, effect_full_name, m_effect_index, m_target_move_index, MAX_MARKER_COUNT);
            });
        }
    }
    if (is_reset_midpoint) m_data->getMarkerManager()->resetMidpoints();
    if (is_reverse) m_data->getMarkerManager()->reverseMarkers();
    if (is_del) m_data->getMarkerManager()->deleteSelectedMarker();

    // プリセットがクリックされた場合、現在のグラデーションがプリセットのものに置き換わるため、
    // その時のグラデーションのデータを差分検知のために保存しておく
    if (m_preset_window.isPresetClicked()) {
        m_script_bridge.setValues(*m_data);
    }

    // スクリプトからグラデーションエディタに値を読み込む
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

    // グラデーションエディタからスクリプトへ値を反映するかどうかのフラグ
    bool is_changed_apply =
        off_to_on ||                                          // 「反映」が OFF から ON に切り替わった
        (m_apply && (                                         // または「反映」ON の状態で、
                        m_preset_window.isPresetClicked() ||  // プリセットがクリックされた
                        is_refresh ||                         // 更新ボタンが押された
                        is_changed_section ||                 // セクションが変更された
                        is_changed_section_effect ||          // 対象とするスクリプトが変更された
                        is_changed_effect_index ||            // 同じスクリプトが複数ある際、対象とするスクリプトのインデックスが変更された
                        is_reverse                            // マーカー反転のボタンが押された
                        ));
    // または各値がグラデーションエディタ側で変更された
    // マーカーの削除、リセット、均等配置による変更は isChangedValues() で検知できる
    if (is_changed_apply || (m_apply && m_script_bridge.getIsChangedValues())) {
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            m_script_bridge.applyGradientToScript(edit, *m_data, effect_full_name, m_effect_index, m_target_move_index);
        });
    }

    // プリセットが変更されたとき、プリセットの範囲外の値はデフォルト値にリセットする
    if (m_apply && m_preset_window.isPresetClicked()) {
        plugin2_utils::call_edit_lambda(gradient_editor::g_app_state.edit_handle->call_edit_section_param, [&](EDIT_SECTION* edit) {
            m_script_bridge.resetScriptData(edit, static_cast<uint32_t>(m_data->getMarkerManager()->getMarkers().size()), MAX_MARKER_COUNT, effect_full_name, m_effect_index, m_target_move_index, MAX_MARKER_COUNT);
        });
    }

    // AviUtl2 ライクなプロパティエディタ（トラックバー、コンボボックスなど）を描画する
    renderPropertyEditor(m_data);

    ImGui::End();
}

void MainView::renderPropertyEditor(GradientData* data)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_SeparatorActive, ImVec4(0, 0, 0, 0));

    float height      = ImGui::GetFrameHeight() * 6 + ImGui::GetStyle().ItemSpacing.y * 5;
    float label_width = ImGui::GetFrameHeight() * scale::relative::ITEM_NAME_BUTTON_WIDTH;

    ImGui::BeginChild("##item_labels", ImVec2(label_width, height), ImGuiChildFlags_ResizeX, ImGuiWindowFlags_NoScrollbar);
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
        bool click_btn = ImGui::ColorButton("##picker_color_button", new_col, ImGuiColorEditFlags_NoTooltip);

        // カラーエディターかカラーボタンがクリックされた場合
        if (click_edit || click_btn) {
            data->getMarkerManager()->setBackupPickerColor(data->getMarkerManager()->getColorPickerColor());
            data->getMarkerManager()->setMarkerColorPickerColor(new_col);
            if (click_btn) {
                ImGui::PushID(data->getMarkerManager());
                ImGui::OpenPopup("marker_color_picker");
                ImGui::PopID();
            }
        }

        if (click_edit || click_btn) data->getMarkerManager()->setSelectedMarkerColor(data->getMarkerManager()->getColorPickerColor());

        ImGui::SetNextItemWidth(width);
        float pos = curr.selected_marker_pos * 100.0f;
        if (ImGui::DragFloat("##marker_pos", &pos, 0.01f, 0.0f, 100.0f, "%.2f")) data->getMarkerManager()->setSelectedMarkerPos(pos / 100.0f);

        ImGui::SetNextItemWidth(width);
        float mid = curr.selected_midpoint_ratio * 100.0f;
        if (ImGui::DragFloat("##midpoint_ratio", &mid, 0.01f, 0.0f, 100.0f, "%.2f")) data->getMarkerManager()->setSelectedMidpointRatio(mid / 100.0f);

        ImGui::SetNextItemWidth(width);
        float blur = curr.blur_width * 100.0f;
        if (ImGui::DragFloat("##blur_width", &blur, 0.1f, 0.0f, 100.0f, "%.0f")) data->setBlurWidth(blur / 100.0f);

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

void MainView::writeHistories()
{
    m_history_window.pushHistory(*m_data);  // この関数が呼ばれた時点のグラデーションを履歴に保存する
    m_history_window.writeHistoryToConfig(m_config_manager, m_history_config);
}
