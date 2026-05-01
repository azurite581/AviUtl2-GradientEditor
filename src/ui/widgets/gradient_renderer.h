#ifndef GRADIENT_RENDERER_H
#define GRADIENT_RENDERER_H

#include <d3d11.h>
#include <wrl/client.h>

#include <cmath>
#include <expected>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace {
#include "pixel_shader.h"
#include "vertex_shader.h"
}  // namespace

class D3DStateSaver {
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_ctx;
    ID3D11RenderTargetView* m_old_rtv   = nullptr;
    ID3D11DepthStencilView* m_old_dsv   = nullptr;
    ID3D11BlendState* m_old_blend_state = nullptr;
    float m_old_blend_factor[4];
    UINT m_old_sample_mask;
    D3D11_VIEWPORT m_old_viewport;
    UINT m_num_viewports = 1;

public:
    D3DStateSaver(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) : m_ctx{context}
    {
        m_ctx->OMGetRenderTargets(1, &m_old_rtv, &m_old_dsv);
        m_ctx->RSGetViewports(&m_num_viewports, &m_old_viewport);
        m_ctx->OMGetBlendState(&m_old_blend_state, m_old_blend_factor, &m_old_sample_mask);
    }

    ~D3DStateSaver()
    {
        m_ctx->OMSetRenderTargets(1, &m_old_rtv, m_old_dsv);
        m_ctx->RSSetViewports(1, &m_old_viewport);
        m_ctx->OMSetBlendState(m_old_blend_state, m_old_blend_factor, m_old_sample_mask);
        if (m_old_rtv) m_old_rtv->Release();
        if (m_old_dsv) m_old_dsv->Release();
        if (m_old_blend_state) m_old_blend_state->Release();
    }
};

class GradientRenderer {
private:
    struct Vertex {
        float pos[3];  // x, y, z
        float uv[2];   // u, v
    };

public:
    GradientRenderer() = default;

#ifdef MARKER_COUNT
    static constexpr uint32_t MAX_MARKER_COUNT = MARKER_COUNT;
#else
    static constexpr uint32_t MAX_MARKER_COUNT = 30;
#endif

    struct PixelConstantBuffer {
        struct GradientInfo {
            struct Color {
                float r{}, g{}, b{}, a{};
            };
            Color start_color{};
            Color stop_color{};
            float start_pos{};
            float stop_pos{};
            float ratio{};
            const float PAD{};
        } gradient[MAX_MARKER_COUNT];
        int32_t gradient_num;
        int32_t gradient_type;
        int32_t interp_dir;
        float blur_width;
        float texture_size[2];
        float gradient_display_size[2];
        struct AlphaMarkerInfo {
            float start_pos{};
            float stop_pos{};
            float start_value{};
            float stop_value{};
        } alpha_stops[MAX_MARKER_COUNT];
        int32_t alpha_sec_num;
        float pad[3];
    };

    // すべてのリソースを保持する構造体
    struct RenderResources {
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader = nullptr;
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer       = nullptr;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout   = nullptr;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader   = nullptr;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_state = nullptr;
        Microsoft::WRL::ComPtr<ID3D11BlendState> blend_state     = nullptr;

        void cleanup()
        {
            if (vertex_shader) {
                vertex_shader.Reset();
                vertex_shader = nullptr;
            }
            if (vertex_buffer) {
                vertex_buffer.Reset();
                vertex_buffer = nullptr;
            }
            if (input_layout) {
                input_layout.Reset();
                input_layout = nullptr;
            }
            if (pixel_shader) {
                pixel_shader.Reset();
                pixel_shader = nullptr;
            }
            if (sampler_state) {
                sampler_state.Reset();
                sampler_state = nullptr;
            }
            if (blend_state) {
                blend_state.Reset();
                blend_state = nullptr;
            }
        }
    };

    // すべてのリソースを作成する初期化関数
    static bool init(
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_device_context,
        RenderResources& resources);

    // 個別のリソース作成関数
    static std::expected<std::monostate, std::string> createSolidColorTexture(
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
        uint64_t width,
        uint64_t height,
        UINT32 color[4],
        ID3D11RenderTargetView** out_rtv,
        ID3D11ShaderResourceView** out_srv);

    static std::expected<std::monostate, std::string> createPixelShader(
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
        ID3D11PixelShader** out_pixel_shader);

    static std::expected<std::monostate, std::string> initPixelConstantBuffer(
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
        Microsoft::WRL::ComPtr<ID3D11Buffer>& pixel_constant_buffer);

    static std::expected<std::monostate, std::string> updatePixelConstantBuffer(
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_device_context,
        ID3D11Buffer* pixel_constant_buffer,
        const PixelConstantBuffer* buffer_values);

    static std::expected<std::monostate, std::string> initVertexBuffer(
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
        ID3D11Buffer** out_vertex_buffer,
        ID3D11InputLayout** out_input_layout);

    static std::expected<std::monostate, std::string> createVertexShader(
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
        ID3D11VertexShader** out_vertex_shader,
        ID3D11Buffer** out_vertex_buffer,
        ID3D11InputLayout** out_input_layout);

    static std::expected<std::monostate, std::string> createBlendState(
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
        ID3D11BlendState** out_blend_state);

    static std::expected<std::monostate, std::string> createSamplerState(
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
        ID3D11SamplerState** out_sampler_state);

    static void runOffscreenRendering(
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_device_context,
        const RenderResources& resources,
        ID3D11Buffer* pixel_constant_buffer,
        PixelConstantBuffer* buffer_values,
        uint32_t width,
        uint32_t height,
        ID3D11RenderTargetView* rtv,
        ID3D11ShaderResourceView* srv);

    static std::expected<std::vector<float>, std::string> readPixelColorFromTexture2D(
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_device_context,
        ID3D11Texture2D* source_texture,
        int32_t x,
        int32_t y);
};

#endif  // !GRADIENT_RENDERER_H
