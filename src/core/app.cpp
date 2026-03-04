#include "core/app.h"

#include <vector>

#include "IconsMaterialSymbols.h"
#include "core/constants.h"
#include "fonts/material_symbols.cpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "ui/style/imgui_style.h"
#include "ui/widgets/gradient_widget.h"
#include "utils/common/color_conv.h"
#include "utils/common/font_loader.h"

namespace gradient_editor {

// Forward declare message handler
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
    if (!g_app_state.window_manager.createPluginWindow(WINDOW_NAME_DEFAULT, main_scale, wnd_proc)) {
        hwnd_promise.set_exception(std::make_exception_ptr(std::runtime_error("Failed to create window")));
        return;
    }

    HWND hwnd = g_app_state.window_manager.getWindowHandle();
    hwnd_promise.set_value(hwnd);

    if (!initialize(hwnd)) {
        return;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    m_main_view = std::make_unique<MainView>();

    // WM_SIZE で ImGui のレンダリング処理を呼び出すために保存する
    g_app_state.render = [this]() {
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
        g_app_state.is_window_visible = (::IsWindowVisible(g_app_state.window_manager.getWindowHandle()) != 0);

        renderFrame();
    }
}

void App::renderFrame()
{
    static bool was_visible = true;
    bool is_visible = g_app_state.is_window_visible;
    bool just_hidden = (was_visible && !is_visible);
    was_visible = is_visible;

    // 非表示になった瞬間だけはオクルージョン判定を無視
    if (!just_hidden && g_app_state.d3d_manager.isSwapChainOccluded() && g_app_state.d3d_manager.getSwapChain()->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
        ::Sleep(10);
        return;
    }

    g_app_state.d3d_manager.setSwapChainOccluded(false);
    g_app_state.d3d_manager.handleWindowResize();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 非表示になった瞬間にすべてのポップアップを閉じる
    if (just_hidden) {
        ImGui::ClosePopupsOverWindow(nullptr, false);
    }

    // 表示状態のときのみ描画する
    if (is_visible) {
        m_main_view->render();
    }

    ImGui::Render();

    ImVec4 clear_color                    = color_conv::u32Rgb2Vec4Rgba<ImVec4>(g_app_state.config_handle->get_color_code(g_app_state.config_handle, "Background"));
    const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w};

    auto rtv = g_app_state.d3d_manager.getRenderTargetView();
    g_app_state.d3d_manager.getDeviceContext()->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
    g_app_state.d3d_manager.getDeviceContext()->ClearRenderTargetView(rtv.Get(), clear_color_with_alpha);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    HRESULT hr = g_app_state.d3d_manager.getSwapChain()->Present(1, 0);
    g_app_state.d3d_manager.setSwapChainOccluded(hr == DXGI_STATUS_OCCLUDED);
}

bool App::initialize(HWND hwnd)
{
    if (!g_app_state.d3d_manager.initialize(hwnd)) {
        g_app_state.d3d_manager.cleanup();
        g_app_state.window_manager.unregisterClass();
        return false;
    }

    setupImGui(hwnd);
    setupFonts();
    applyCustomColors();
    CustomUI::initDX11(g_app_state.d3d_manager.getDevice(), g_app_state.d3d_manager.getDeviceContext());

    return true;
}

void App::setupImGui(HWND hwnd)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style          = ImGui::GetStyle();
    style.GrabMinSize          = scale::absolute::GRAB_MIN_SIZE;
    style.FrameBorderSize      = scale::absolute::FRAME_BORDER_SIZE;
    style.TabRounding          = scale::absolute::TAB_ROUNDING;
    style.DockingSeparatorSize = scale::absolute::DOCKING_SEPARATOR_SIZE;
    style.ItemSpacing          = ImVec2(scale::absolute::ITEM_SPACING_X, scale::absolute::ITEM_SPACING_Y);
    style.ItemInnerSpacing     = ImVec2(scale::absolute::ITEM_INNER_SPACING_X, style.ItemInnerSpacing.y);

    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi         = main_scale;
    io.ConfigDpiScaleFonts     = true;
    io.ConfigDpiScaleViewports = true;

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_app_state.d3d_manager.getDevice().Get(), g_app_state.d3d_manager.getDeviceContext().Get());
}

void App::setupFonts()
{
    ImGuiIO& io                     = ImGui::GetIO();
    static ImWchar exclude_ranges[] = {ICON_MIN_MS, ICON_MAX_MS, 0};
    ImFontConfig config1;
    config1.GlyphExcludeRanges = exclude_ranges;

    // sytle.conf からフォント名を取得
    FONT_INFO* font_info = g_app_state.config_handle->get_font_info(g_app_state.config_handle, "DefaultFamily");
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
}

void App::cleanup()
{
    g_app_state.render = nullptr;
    m_main_view.reset();
    CustomUI::cleanup();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_app_state.d3d_manager.cleanup();
}

}  // namespace gradient_editor
