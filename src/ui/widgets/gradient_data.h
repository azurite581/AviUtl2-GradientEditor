#ifndef GRADIENT_DATA_H
#define GRADIENT_DATA_H

#include <d3d11.h>
#include <wrl/client.h>

#include <compare>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <ranges>
#include <vector>

#include "gradient_marker.h"
#include "gradient_renderer.h"
#include "imgui.h"

class GradientData {
private:
    int32_t m_texture_width{}, m_texture_height{};
    float m_gradient_display_width{}, m_gradient_display_height{};

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv          = nullptr;  // 書き込み先
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv        = nullptr;  // 書き込み先をImGuiで表示するためのSRV
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pixel_constant_buffer  = nullptr;  // グラデーションを描画するピクセルシェーダーのコンスタントバッファー
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_output_srv = nullptr;

public:
    ~GradientData()
    {
        cleanup();
    }

    MarkerManager m_color_markers{};
    MarkerManager m_alpha_markers{};

    GradientMarkerManager m_marker_manager;
    int32_t m_color_space{0};
    int32_t m_interp_dir{0};
    float m_blur_width{1.0f};
    float m_alpha_blur_width{1.0f};

    std::partial_ordering operator<=>(const GradientData& other) const
    {
        if (auto cmp = m_marker_manager <=> other.m_marker_manager; cmp != 0) return cmp;
        if (auto cmp = m_color_space <=> other.m_color_space; cmp != 0) return cmp;
        if (auto cmp = m_interp_dir <=> other.m_interp_dir; cmp != 0) return cmp;
        if (auto cmp = m_blur_width <=> other.m_blur_width; cmp != 0) return cmp;
        return m_alpha_blur_width <=> other.m_alpha_blur_width;
    }

    bool operator==(const GradientData& other) const
    {
        return m_marker_manager == other.m_marker_manager &&
               m_color_space == other.m_color_space &&
               m_interp_dir == other.m_interp_dir &&
               m_blur_width == other.m_blur_width &&
               m_alpha_blur_width == other.m_alpha_blur_width;
    }

    [[nodiscard]] auto getColorMarkers() noexcept { return &m_color_markers; }
    [[nodiscard]] auto getAlphaMarkers() noexcept { return &m_alpha_markers; }
    [[nodiscard]] GradientMarkerManager* getMarkerManager() noexcept { return &m_marker_manager; }
    [[nodiscard]] int32_t getColorSpace() const noexcept { return m_color_space; }
    [[nodiscard]] int32_t getInterpDir() const noexcept { return m_interp_dir; }
    [[nodiscard]] float getBlurWidth() const noexcept { return m_blur_width; }
    [[nodiscard]] float getAlphaBlurWidth() const noexcept { return m_alpha_blur_width; }

    [[nodiscard]] int32_t getTextureWidth() const noexcept { return m_texture_width; }
    [[nodiscard]] int32_t getTextureHeight() const noexcept { return m_texture_height; }
    [[nodiscard]] ID3D11RenderTargetView* getRtv() const noexcept { return m_rtv.Get(); }
    [[nodiscard]] ID3D11ShaderResourceView* getSrv() const noexcept { return m_srv.Get(); }
    [[nodiscard]] ID3D11Buffer* getPixelConstantBuffer() const noexcept { return m_pixel_constant_buffer.Get(); }
    [[nodiscard]] ID3D11ShaderResourceView* getOutputSrv() const noexcept { return m_output_srv.Get(); }

    void setColorSpace(const int32_t color_space) noexcept { m_color_space = color_space; }
    void setInterpDir(const int32_t interp_dir) noexcept { m_interp_dir = interp_dir; }
    void setColorBlurWidth(const float blur_width) noexcept { m_blur_width = blur_width; }
    void setAlphaBlurWidth(const float blur_width) noexcept { m_alpha_blur_width = blur_width; }
    void setGradientDisplayWidth(const float gradient_display_width) noexcept { m_gradient_display_width = gradient_display_width; }
    void setGradientDisplayHeight(const float gradient_display_height) noexcept { m_gradient_display_height = gradient_display_height; }

    GradientRenderer::PixelConstantBuffer gradientData2pixelConstantBuffer();

    bool init(Microsoft::WRL::ComPtr<ID3D11Device> d3d_device, const int32_t texture_width, const int32_t texture_height);

    void cleanup()
    {
        if (m_rtv) {
            m_rtv.Reset();
            m_rtv = nullptr;
        }
        if (m_srv) {
            m_srv.Reset();
            m_srv = nullptr;
        }
        if (m_pixel_constant_buffer) {
            m_pixel_constant_buffer.Reset();
            m_pixel_constant_buffer = nullptr;
        }
        if (m_output_srv) {
            m_output_srv.Reset();
            m_output_srv = nullptr;
        }
    }

    std::vector<float> getTextureColor(
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_device_context, const int32_t x, const int32_t y);
};

#endif  // !GRADIENT_DATA_H
