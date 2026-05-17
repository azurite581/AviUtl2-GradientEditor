#include "app.h"

#include <algorithm>
#include <iterator>
#include <vector>

#include "IconsMaterialSymbols.h"
#include "color_conv.h"
#include "config2_wrapper_interface.h"
#include "constants.h"
#include "font_loader.h"
#include "gradient_widget.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui_internal.h"
#include "logger_wrapper_interface.h"
#include "material_symbols.cpp"

extern LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

App::App() = default;

App::~App()
{
    cleanup();
}

void App::run(std::promise<HWND>&& hwnd_promise)
{
    // ImGui の DPI スケーリングを有効にする
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

    // ウィンドウの作成
    if (!gradient_editor::g_app_state.window_manager.createPluginWindow(WINDOW_NAME, main_scale, wnd_proc)) {
        hwnd_promise.set_exception(std::make_exception_ptr(std::runtime_error("Failed to create window")));
        return;
    }

    HWND hwnd = gradient_editor::g_app_state.window_manager.getWindowHandle();
    hwnd_promise.set_value(hwnd);

    //
    // D3D の初期化
    //
    if (!gradient_editor::g_app_state.d3d_manager.initialize(hwnd)) {
        gradient_editor::g_app_state.d3d_manager.cleanup();
        gradient_editor::g_app_state.window_manager.unregisterClass();
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
    ImGuiStyle& style          = ImGui::GetStyle();
    style.FrameRounding        = scale::absolute::FRAME_ROUNDING;
    style.GrabMinSize          = scale::absolute::GRAB_MIN_SIZE;
    style.FrameBorderSize      = scale::absolute::FRAME_BORDER_SIZE;
    style.TabRounding          = scale::absolute::TAB_ROUNDING;
    style.DockingSeparatorSize = scale::absolute::DOCKING_SEPARATOR_SIZE;
    style.ItemSpacing          = ImVec2(scale::absolute::ITEM_SPACING_X, scale::absolute::ITEM_SPACING_Y);
    style.ItemInnerSpacing     = ImVec2(scale::absolute::ITEM_INNER_SPACING_X, style.ItemInnerSpacing.y);
    style.ScrollbarRounding    = scale::absolute::SCROLLBAR_ROUNDING;

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
    style.FontScaleMain = gradient_editor::g_app_state.settings.ui_scale / 100.0f;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(gradient_editor::g_app_state.d3d_manager.getDevice().Get(), gradient_editor::g_app_state.d3d_manager.getDeviceContext().Get());

    //
    // フォントの設定
    //
    static ImWchar exclude_ranges[] = {static_cast<ImWchar>(ICON_MIN_MS), static_cast<ImWchar>(ICON_MAX_MS), 0};
    ImFontConfig config1;
    config1.GlyphExcludeRanges = exclude_ranges;

    // sytle.conf からフォント名を取得
    FONT_INFO* font_info = gradient_editor::g_app_state.config_handle->get_font_info(gradient_editor::g_app_state.config_handle, "DefaultFamily");
    // フォント名からフォントデータを取得
    std::vector<unsigned char> font_data = getFontDataByName(font_info->name);

    if (!font_data.empty()) {
        // AddFontFromMemoryTTF() はバッファの所有権をフォントアトラスに転送し、フォントアトラス破棄時にバッファを解放する
        // https://github.com/ocornut/imgui/blob/master/docs/FONTS.md#loading-font-data-from-memory
        void* buffer = malloc(font_data.size());
        memcpy(buffer, font_data.data(), font_data.size());
        io.Fonts->AddFontFromMemoryTTF(buffer, static_cast<int>(font_data.size()), font_info->size, &config1);
    } else {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\YuGothM.ttc", DEFAULT_FONT_SIZE, &config1);
    }

    // アイコンフォントの設定
    ImFontConfig config2;
    config2.MergeMode        = true;
    config2.GlyphMinAdvanceX = font_info->size;
    config2.GlyphOffset.y += ICON_FONT_GLYPHOFFSET_Y;
    io.Fonts->AddFontFromMemoryCompressedTTF(material_symbols_compressed_data, material_symbols_compressed_size, font_info->size, &config2);

    //
    // テーマ設定
    //
    auto aulColor2imVec4 = [](const std::string& name) -> ImVec4 {
        return color_conv::u32Rgb2Vec4Rgba<ImVec4>(gradient_editor::g_app_state.config_handle->get_color_code(gradient_editor::g_app_state.config_handle, name.c_str()));
    };

    // ウィンドウ
    style.Colors[ImGuiCol_WindowBg] = aulColor2imVec4("Grouping");
    style.Colors[ImGuiCol_PopupBg]  = aulColor2imVec4("Grouping");
    style.Colors[ImGuiCol_Border]   = aulColor2imVec4("Border");
    // テキスト
    style.Colors[ImGuiCol_Text] = aulColor2imVec4("Text");
    // ボタン
    style.Colors[ImGuiCol_Button]        = aulColor2imVec4("ButtonBody");
    style.Colors[ImGuiCol_ButtonHovered] = aulColor2imVec4("ButtonBodyHover");
    style.Colors[ImGuiCol_ButtonActive]  = aulColor2imVec4("ButtonBodyPress");
    // フレーム
    style.Colors[ImGuiCol_FrameBg]        = aulColor2imVec4("ButtonBody");
    style.Colors[ImGuiCol_FrameBgHovered] = aulColor2imVec4("ButtonBodyHover");
    style.Colors[ImGuiCol_FrameBgActive]  = aulColor2imVec4("ButtonBodySelect");
    // メニューバー
    style.Colors[ImGuiCol_MenuBarBg] = aulColor2imVec4("GroupingHover");
    // タブ
    style.Colors[ImGuiCol_TitleBg]             = aulColor2imVec4("Background");
    style.Colors[ImGuiCol_TitleBgActive]       = aulColor2imVec4("Background");
    style.Colors[ImGuiCol_Tab]                 = aulColor2imVec4("GroupingHover");
    style.Colors[ImGuiCol_TabDimmed]           = aulColor2imVec4("GroupingHover");
    style.Colors[ImGuiCol_TabDimmedSelected]   = aulColor2imVec4("GroupingSelect");
    style.Colors[ImGuiCol_TabSelected]         = aulColor2imVec4("GroupingSelect");
    style.Colors[ImGuiCol_TabHovered]          = aulColor2imVec4("GroupingSelect");
    style.Colors[ImGuiCol_TabSelectedOverline] = aulColor2imVec4("BorderFocus");
    // コンボボックス
    style.Colors[ImGuiCol_Header]        = aulColor2imVec4("ButtonBodySelect");
    style.Colors[ImGuiCol_HeaderHovered] = aulColor2imVec4("ButtonBodySelect");
    style.Colors[ImGuiCol_HeaderActive]  = aulColor2imVec4("ButtonBodySelect");
    // スライダー
    style.Colors[ImGuiCol_SliderGrab]       = aulColor2imVec4("SliderCursor");
    style.Colors[ImGuiCol_SliderGrabActive] = aulColor2imVec4("SliderCursor");
    // ドラッグ&ドロップ時の枠線
    style.Colors[ImGuiCol_DragDropTarget] = aulColor2imVec4("BorderFocus");
    // ドッキングウィンドウの分割線
    style.Colors[ImGuiCol_ResizeGripHovered] = aulColor2imVec4("Border");
    style.Colors[ImGuiCol_ResizeGripActive]  = aulColor2imVec4("BorderFocus");
    // スクロールバー
    style.Colors[ImGuiCol_ScrollbarBg]          = aulColor2imVec4("Background");
    style.Colors[ImGuiCol_ScrollbarGrab]        = aulColor2imVec4("ButtonBody");
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = aulColor2imVec4("ButtonBodyHover");
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = aulColor2imVec4("ButtonBodyPress");

    //
    // グラデーションエディター用の D3D を初期化
    //
    custom_ui::initDX11(gradient_editor::g_app_state.d3d_manager.getDevice(), gradient_editor::g_app_state.d3d_manager.getDeviceContext());

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // メインビューの初期化
    // コンストラクタ内でプリセットの初期化を行う
    m_main_view = std::make_unique<MainView>(
        get_logger_wrapper_interface(gradient_editor::g_app_state.log_handle),
        get_config_wrapper_interface(gradient_editor::g_app_state.config_handle));

    // WM_SIZE で ImGui のレンダリング処理を呼び出すために保存する
    gradient_editor::g_app_state.render = [this]() {
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
        gradient_editor::g_app_state.is_window_visible = (::IsWindowVisible(gradient_editor::g_app_state.window_manager.getWindowHandle()) != 0);

        renderFrame();
    }
}

void App::renderFrame()
{
    static bool was_visible = true;
    bool is_visible         = gradient_editor::g_app_state.is_window_visible;
    bool just_hidden        = (was_visible && !is_visible);
    was_visible             = is_visible;

    // 非表示になった瞬間だけはオクルージョン判定を無視
    if (!just_hidden && gradient_editor::g_app_state.d3d_manager.isSwapChainOccluded() && gradient_editor::g_app_state.d3d_manager.getSwapChain()->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
        ::Sleep(10);
        return;
    }

    gradient_editor::g_app_state.d3d_manager.setSwapChainOccluded(false);
    gradient_editor::g_app_state.d3d_manager.handleWindowResize();

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

    // 非表示の時でもウィンドウ等の状態を維持するために描画処理を呼び出す
    m_main_view->render();

    ImGui::Render();

    ImVec4 clear_color                    = color_conv::u32Rgb2Vec4Rgba<ImVec4>(gradient_editor::g_app_state.config_handle->get_color_code(gradient_editor::g_app_state.config_handle, "Background"));
    const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w};

    auto rtv = gradient_editor::g_app_state.d3d_manager.getRenderTargetView();
    if (rtv) {
        gradient_editor::g_app_state.d3d_manager.getDeviceContext()->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
        gradient_editor::g_app_state.d3d_manager.getDeviceContext()->ClearRenderTargetView(rtv.Get(), clear_color_with_alpha);
    }

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    HRESULT hr = gradient_editor::g_app_state.d3d_manager.getSwapChain()->Present(1, 0);
    gradient_editor::g_app_state.d3d_manager.setSwapChainOccluded(hr == DXGI_STATUS_OCCLUDED);

    // 再入フラグを解除
    s_in_render = false;
}

void App::cleanup()
{
    writeSettings();                      // ウィンドウのレイアウト等の設定をファイルに書き込む
    m_main_view.get()->writeHistories();  // グラデーションの履歴をファイルに書き込む

    gradient_editor::g_app_state.render = nullptr;
    m_main_view.reset();
    custom_ui::cleanup();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    gradient_editor::g_app_state.d3d_manager.cleanup();
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
    std::ifstream ifs(gradient_editor::g_app_state.settings_file_path);
    std::string settings_content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    auto ui_scale                                  = getConfigInt(gradient_editor::g_app_state.settings_file_path, "settings", "ui_scale", 100);
    gradient_editor::g_app_state.settings.ui_scale = std::clamp(ui_scale, 50u, 400u);

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
        std::ofstream ofs(gradient_editor::g_app_state.settings_file_path);
        ofs.clear();
    }

    // ImGui の設定を取得
    size_t imgui_size      = 0;
    const char* imgui_data = ImGui::SaveIniSettingsToMemory(&imgui_size);
    std::string imgui_data_str(imgui_data, imgui_size);

    // 再書き込み
    std::string settings_data_str{};
    settings_data_str += "[settings]\n";
    settings_data_str += std::format("ui_scale={}\n", gradient_editor::g_app_state.settings.ui_scale);

    settings_data_str += "\n";

    settings_data_str += "[imgui]\n";
    settings_data_str += imgui_data_str;
    std::ofstream ofs(gradient_editor::g_app_state.settings_file_path);
    ofs << settings_data_str;
}
