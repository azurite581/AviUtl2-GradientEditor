#include "app.h"

#include "IconsMaterialSymbols.h"
#include "font_loader.h"
#include "gradient_widget.h"
#include "str_conv.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui_internal.h"
#include "material_symbols.cpp"

extern LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

void App::applyAviutl2Style()
{
    auto aulColor2imVec4 = [this](const std::string& name) -> ImVec4 {
        if (!m_config_handle) return {};
        auto rgb      = m_config_handle->get_color_code(m_config_handle, name.c_str());
        float inv_255 = 1.0f / 255.0f;
        float r       = static_cast<float>((rgb >> 16) & 0xFF) * inv_255;
        float g       = static_cast<float>((rgb >> 8) & 0xFF) * inv_255;
        float b       = static_cast<float>(rgb & 0xFF) * inv_255;
        return {r, g, b, 1.0f};
    };

    ImGuiStyle& style = ImGui::GetStyle();

    // テキスト
    style.Colors[ImGuiCol_Text]         = aulColor2imVec4("Text");
    style.Colors[ImGuiCol_TextDisabled] = aulColor2imVec4("TextDisable");
    // ウィンドウ
    style.Colors[ImGuiCol_WindowBg] = aulColor2imVec4("Grouping");
    style.Colors[ImGuiCol_ChildBg]  = aulColor2imVec4("Grouping");
    style.Colors[ImGuiCol_PopupBg]  = aulColor2imVec4("Grouping");
    // 境界線
    style.Colors[ImGuiCol_Border]       = aulColor2imVec4("Border");
    style.Colors[ImGuiCol_BorderShadow] = aulColor2imVec4("Border");
    // フレーム
    style.Colors[ImGuiCol_FrameBg]        = aulColor2imVec4("ButtonBody");
    style.Colors[ImGuiCol_FrameBgHovered] = aulColor2imVec4("ButtonBodyHover");
    style.Colors[ImGuiCol_FrameBgActive]  = aulColor2imVec4("ButtonBodySelect");
    // タイトルバー
    style.Colors[ImGuiCol_TitleBg]          = aulColor2imVec4("Background");
    style.Colors[ImGuiCol_TitleBgActive]    = aulColor2imVec4("Background");
    style.Colors[ImGuiCol_TitleBgCollapsed] = aulColor2imVec4("Background");
    // メニューバー
    style.Colors[ImGuiCol_MenuBarBg] = aulColor2imVec4("GroupingHover");
    // スクロールバー
    style.Colors[ImGuiCol_ScrollbarBg]          = aulColor2imVec4("Background");
    style.Colors[ImGuiCol_ScrollbarGrab]        = aulColor2imVec4("ButtonBody");
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = aulColor2imVec4("ButtonBodyHover");
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = aulColor2imVec4("ButtonBodyPress");
    // チェックボックス
    style.Colors[ImGuiCol_CheckMark]          = aulColor2imVec4("Text");
    style.Colors[ImGuiCol_CheckboxSelectedBg] = aulColor2imVec4("ButtonBody");
    // スライダー
    style.Colors[ImGuiCol_SliderGrab]       = aulColor2imVec4("SliderCursor");
    style.Colors[ImGuiCol_SliderGrabActive] = aulColor2imVec4("SliderCursor");
    // ボタン
    style.Colors[ImGuiCol_Button]        = aulColor2imVec4("ButtonBody");
    style.Colors[ImGuiCol_ButtonHovered] = aulColor2imVec4("ButtonBodyHover");
    style.Colors[ImGuiCol_ButtonActive]  = aulColor2imVec4("ButtonBodyPress");
    // コンボボックス
    style.Colors[ImGuiCol_Header]        = aulColor2imVec4("ButtonBodySelect");
    style.Colors[ImGuiCol_HeaderHovered] = aulColor2imVec4("ButtonBodySelect");
    style.Colors[ImGuiCol_HeaderActive]  = aulColor2imVec4("ButtonBodySelect");
    // セパレーター
    style.Colors[ImGuiCol_Separator]        = aulColor2imVec4("Border");
    style.Colors[ImGuiCol_SeparatorHovered] = aulColor2imVec4("Border");
    style.Colors[ImGuiCol_SeparatorActive]  = aulColor2imVec4("BorderFocus");
    // リサイズグリップ
    style.Colors[ImGuiCol_ResizeGrip]        = aulColor2imVec4("Border");
    style.Colors[ImGuiCol_ResizeGripHovered] = aulColor2imVec4("Border");
    style.Colors[ImGuiCol_ResizeGripActive]  = aulColor2imVec4("BorderFocus");
    // テキストカーソル
    style.Colors[ImGuiCol_InputTextCursor] = aulColor2imVec4("Text");
    // タブ
    style.Colors[ImGuiCol_TabHovered]                = aulColor2imVec4("ButtonBodySelect");
    style.Colors[ImGuiCol_Tab]                       = aulColor2imVec4("Grouping");
    style.Colors[ImGuiCol_TabSelected]               = aulColor2imVec4("GroupingSelect");
    style.Colors[ImGuiCol_TabSelectedOverline]       = aulColor2imVec4("BorderFocus");
    style.Colors[ImGuiCol_TabDimmed]                 = aulColor2imVec4("GroupingHover");
    style.Colors[ImGuiCol_TabDimmedSelected]         = aulColor2imVec4("GroupingSelect");
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = aulColor2imVec4("GroupingSelect");
    // テキスト選択
    style.Colors[ImGuiCol_TextSelectedBg] = aulColor2imVec4("TextSelect");
    // ドラッグ&ドロップ時の枠線
    style.Colors[ImGuiCol_DragDropTarget] = aulColor2imVec4("BorderFocus");
}

void App::run(std::promise<HWND>&& hwnd_promise)
{
    // ImGui の DPI スケーリングを有効にする
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

    // ウィンドウの作成
    if (!m_window_manager.createPluginWindow(WINDOW_NAME, main_scale, wnd_proc)) {
        hwnd_promise.set_exception(std::make_exception_ptr(std::runtime_error("Failed to create window")));
        return;
    }

    HWND hwnd = m_window_manager.getWindowHandle();
    hwnd_promise.set_value(hwnd);

    //
    // D3D の初期化
    //
    if (!m_d3d_manager.initialize(hwnd)) {
        m_d3d_manager.cleanup();
        m_window_manager.unregisterClass();
        return;
    }

    //
    // ImGui の初期化
    //
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi         = main_scale;
    io.ConfigDpiScaleFonts     = true;
    io.ConfigDpiScaleViewports = true;

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // imgui.ini を自動生成しないようにする
    io.IniFilename = nullptr;
    readSettings();  // 設定を読み込む
    style.FontScaleMain = m_settings.ui_scale / 100.0f;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(m_d3d_manager.getDevice().Get(), m_d3d_manager.getDeviceContext().Get());

    //
    // フォントの設定
    //
    static ImWchar exclude_ranges[] = {static_cast<ImWchar>(ICON_MIN_MS), static_cast<ImWchar>(ICON_MAX_MS), 0};
    ImFontConfig config1;
    config1.GlyphExcludeRanges = exclude_ranges;

    // sytle.conf からフォント名を取得
    FONT_INFO* font_info = m_config_handle->get_font_info(m_config_handle, "DefaultFamily");
    // フォント名からフォントデータを取得
    auto font_data = getFontDataByName(font_info->name);

    if (font_data.has_value()) {
        // AddFontFromMemoryTTF() はバッファの所有権をフォントアトラスに転送し、フォントアトラス破棄時にバッファを解放するため、手動で解放しなくて良い。
        // https://github.com/ocornut/imgui/blob/master/docs/FONTS.md#loading-font-data-from-memory
        // void* buffer = malloc(font_data->bytes.size());
        // memcpy(buffer, font_data->bytes.data(), font_data->bytes.size());

        // config1.FontNo = static_cast<int32_t>(font_data->face_index);
        // io.Fonts->AddFontFromMemoryTTF(
        //     buffer,
        //     static_cast<int>(font_data->bytes.size()),
        //     font_info->size,
        //     &config1);

        // style.conf によって指定されたフォントを同じサイズで読み込んでも、見かけ上のサイズが等しくならない。
        // そのため AviUtl2 側と見かけ上のサイズがほぼ等しくなる Yu Gothic Medium 固定にする。
        // 関連：https://github.com/ocornut/imgui/issues/8822
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\YuGothM.ttc", Scale::Absolute::DEFAULT_FONT_SIZE, &config1);
    } else {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\YuGothM.ttc", Scale::Absolute::DEFAULT_FONT_SIZE, &config1);
    }

    // アイコンフォントの設定
    ImFontConfig config2;
    config2.MergeMode        = true;
    config2.GlyphMinAdvanceX = font_info->size;
    config2.GlyphOffset.y += Scale::Absolute::ICON_FONT_GLYPHOFFSET_Y;
    io.Fonts->AddFontFromMemoryCompressedTTF(material_symbols_compressed_data, material_symbols_compressed_size, font_info->size, &config2);

    //
    // テーマ適用
    //
    applyAviutl2Style();

    style.FrameRounding        = Scale::Absolute::FRAME_ROUNDING;
    style.GrabMinSize          = Scale::Absolute::GRAB_MIN_SIZE;
    style.FrameBorderSize      = Scale::Absolute::FRAME_BORDER_SIZE;
    style.TabRounding          = Scale::Absolute::TAB_ROUNDING;
    style.DockingSeparatorSize = Scale::Absolute::DOCKING_SEPARATOR_SIZE;
    style.ScrollbarRounding    = Scale::Absolute::SCROLLBAR_ROUNDING;

    //
    // グラデーションエディター用の D3D を初期化
    //
    custom_ui::initD3D11(m_d3d_manager.getDevice(), m_d3d_manager.getDeviceContext());

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    //
    // メインビューの初期化
    //
    m_main_view = std::make_unique<MainView>(
        get_logger_wrapper_interface(m_log_handle),
        get_config_wrapper_interface(m_config_handle));

    m_main_view->setPluginInfo(str_conv::wideCharToMultiByte(PLUGIN_NAME), PLUGIN_VERSION, "azurite");
    m_main_view->setWindowVisible({static_cast<bool>(m_settings.preset_tab),
                                   static_cast<bool>(m_settings.history_tab)});

    // WM_SIZE で ImGui のレンダリング処理を呼び出すために保存する
    m_render = [this]() {
        renderFrame();
    };

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        // ウィンドウの表示状態を取得する
        m_is_window_visible = (::IsWindowVisible(m_window_manager.getWindowHandle()) != 0);

        renderFrame();
    }
}

void App::renderFrame()
{
    static bool was_visible = true;
    bool is_visible         = m_is_window_visible;
    bool just_hidden        = (was_visible && !is_visible);
    was_visible             = is_visible;

    // 非表示になった瞬間だけはオクルージョン判定を無視
    if (!just_hidden && m_d3d_manager.isSwapChainOccluded() && m_d3d_manager.getSwapChain()->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
        ::Sleep(10);
        return;
    }

    m_d3d_manager.setSwapChainOccluded(false);
    m_d3d_manager.handleWindowResize();

    // 再入を防ぐ（WM_SIZE 等で renderFrame が再帰的に呼ばれる可能性がある）
    static thread_local bool s_in_render = false;
    if (s_in_render)
        return;
    s_in_render = true;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 非表示になった瞬間にすべてのポップアップを閉じる
    if (just_hidden) {
        ImGui::ClosePopupsOverWindow(nullptr, false);
    }

    static bool last_mouse_pressed = false;
    bool any_mouse_pressed =
        (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ||
        (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ||
        (GetAsyncKeyState(VK_MBUTTON) & 0x8000) ||
        (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) ||
        (GetAsyncKeyState(VK_XBUTTON2) & 0x8000);

    // 現在フレームで新しくクリックされたかどうかを判定
    bool mouse_clicked_this_frame = any_mouse_pressed && !last_mouse_pressed;
    last_mouse_pressed            = any_mouse_pressed;

    if (ImGuiContext& g = *GImGui; mouse_clicked_this_frame && !g.HoveredWindow && g.MovingWindow == nullptr) {
        ImGui::ClearActiveID();
        ImGui::ClosePopupsOverWindow(nullptr, false);
    }

    // 相対スケールを適用
    ImGuiStyle& style      = ImGui::GetStyle();
    style.ItemSpacing      = ImVec2(ImGui::GetFrameHeight() * Scale::Relative::ITEM_SPACING_X, ImGui::GetFrameHeight() * Scale::Relative::ITEM_SPACING_Y);
    style.ItemInnerSpacing = ImVec2(ImGui::GetFrameHeight() * Scale::Relative::ITEM_INNER_SPACING_X, style.ItemInnerSpacing.y);

    // 非表示の時でもウィンドウ等の状態を維持するために描画処理を呼び出す
    m_main_view->render();

    ImGui::Render();

    ImVec4 clear_color                    = color_conv::u32Rgb2Vec4Rgba<ImVec4>(m_config_handle->get_color_code(m_config_handle, "Background"));
    const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w};

    auto rtv = m_d3d_manager.getRenderTargetView();
    if (rtv) {
        m_d3d_manager.getDeviceContext()->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
        m_d3d_manager.getDeviceContext()->ClearRenderTargetView(rtv.Get(), clear_color_with_alpha);
    }

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    HRESULT hr = m_d3d_manager.getSwapChain()->Present(1, 0);
    m_d3d_manager.setSwapChainOccluded(hr == DXGI_STATUS_OCCLUDED);

    // 再入フラグを解除
    s_in_render = false;
}

void App::cleanup()
{
    // タブの表示状態を保存
    auto visible               = m_main_view->getWindowVisible();
    m_settings.preset_tab  = static_cast<uint32_t>(visible.preset_window);
    m_settings.history_tab = static_cast<uint32_t>(visible.history_window);

    writeSettings();                      // ウィンドウのレイアウト等の設定をファイルに書き込む
    m_main_view.get()->writeHistories();  // グラデーションの履歴をファイルに書き込む

    // ビューを解放
    m_render = nullptr;
    m_main_view.reset();
    custom_ui::cleanup();

    // ImGui のバックエンドを解放
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // グラデーション描画用の D3D を解放
    m_d3d_manager.cleanup();

    // WM_QUIT を App::run() 内のメッセージループに通知
    HWND hwnd = m_window_manager.getWindowHandle();
    if (hwnd) {
        ::PostMessage(hwnd, WM_QUIT, 0, 0);
    }

    // GUI スレッドを停止
    if (m_gui_thread.joinable()) {
        m_gui_thread.join();
    }
}

void App::readSettings()
{
    auto getConfigInt = [](const std::filesystem::path file_path, const char* section_name, const char* key_name, const uint32_t def) {
        if (file_path.empty()) {
            return def;
        }

        auto result = ::GetPrivateProfileIntA(
            section_name,
            key_name,
            def,
            file_path.string().c_str());

        return result;
    };

    // 設定ファイル全体を読み込む
    std::ifstream ifs(m_settings_file_path);
    std::string settings_content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    auto ui_scale              = getConfigInt(m_settings_file_path, "settings", "ui_scale", 100);
    m_settings.ui_scale    = std::clamp(ui_scale, 50u, 400u);
    auto preset_tab            = getConfigInt(m_settings_file_path, "settings", "preset_tab", 0);
    m_settings.preset_tab  = std::clamp(preset_tab, 0u, 1u);
    auto history_tab           = getConfigInt(m_settings_file_path, "settings", "history_tab", 0);
    m_settings.history_tab = std::clamp(history_tab, 0u, 1u);

    // [imgui] セクション以下全体を抽出
    size_t imgui_start = settings_content.find("[imgui]");
    if (imgui_start == std::string::npos) return;

    imgui_start = settings_content.find('\n', imgui_start);
    if (imgui_start == std::string::npos) return;

    if (imgui_start != std::string::npos) {
        size_t imgui_end = settings_content.size();  // ファイル末尾まで
        if (imgui_end == std::string::npos) imgui_end = settings_content.size();

        std::string imgui_ini = settings_content.substr(imgui_start, imgui_end - imgui_start);

        // ImGui の設定を読み込む
        ImGui::LoadIniSettingsFromMemory(imgui_ini.c_str(), imgui_ini.size());
    }
}

void App::writeSettings()
{
    {
        std::ofstream ofs(m_settings_file_path);
        ofs.clear();
    }

    // ImGui の設定を取得
    size_t imgui_size      = 0;
    const char* imgui_data = ImGui::SaveIniSettingsToMemory(&imgui_size);
    std::string imgui_data_str(imgui_data, imgui_size);

    // 再書き込み
    std::string settings_data_str{};
    settings_data_str += "[settings]\n";
    settings_data_str += std::format("ui_scale={}\n", m_settings.ui_scale);
    settings_data_str += std::format("preset_tab={}\n", m_settings.preset_tab);
    settings_data_str += std::format("history_tab={}\n", m_settings.history_tab);

    settings_data_str += "\n";

    settings_data_str += "[imgui]\n";
    settings_data_str += imgui_data_str;
    std::ofstream ofs(m_settings_file_path);
    ofs << settings_data_str;
}

App g_app;
