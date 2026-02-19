#ifndef D3D_MANAGER_H
#define D3D_MANAGER_H

#include <d3d11.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wrl/client.h>

namespace gradient_editor {

/// @brief Direct3D 11 リソース管理クラス
/// @note デバイス、スワップチェーン、レンダーターゲットの管理を担当
class D3DManager {
public:
    D3DManager() = default;

    // コピー・ムーブ禁止
    D3DManager(const D3DManager&)            = delete;
    D3DManager& operator=(const D3DManager&) = delete;
    D3DManager(D3DManager&&)                 = delete;
    D3DManager& operator=(D3DManager&&)      = delete;

    /// @brief Direct3D デバイスとスワップチェーンを作成
    /// @param hwnd ウィンドウハンドル
    /// @return 成功時true、失敗時false
    bool initialize(HWND hwnd);

    bool createDeviceD3D(HWND hwnd);

    /// @brief すべてのDirect3Dリソースを解放
    void cleanup();

    /// @brief レンダーターゲットビューを作成
    void createRenderTarget();

    /// @brief レンダーターゲットビューをクリーンアップ
    void cleanupRenderTarget();

    void reportLiveDeviceObjectsDetail();

    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D11Device> getDevice() const noexcept { return m_device; }
    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D11DeviceContext> getDeviceContext() const noexcept { return m_device_context; }
    [[nodiscard]] Microsoft::WRL::ComPtr<IDXGISwapChain> getSwapChain() const noexcept { return m_swap_chain; }
    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D11RenderTargetView> getRenderTargetView() const noexcept { return m_render_target_view; }

    [[nodiscard]] bool isSwapChainOccluded() const noexcept { return m_swap_chain_occluded; }
    void setSwapChainOccluded(bool occluded) noexcept { m_swap_chain_occluded = occluded; }

    void handleWindowResize();
    void setResizeWidth(const UINT width) noexcept { m_resize_width = width; }
    void setResizeHeight(const UINT height) noexcept { m_resize_height = height; }

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_device                       = nullptr;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_device_context        = nullptr;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swap_chain                 = nullptr;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_render_target_view = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Debug> m_debug                         = nullptr;

    bool m_swap_chain_occluded = false;
    UINT m_resize_width        = 0;
    UINT m_resize_height       = 0;
};

}  // namespace gradient_editor

#endif  // D3D_MANAGER_H
