#include "gradient_data.h"

GradientRenderer::PixelConstantBuffer GradientData::gradientData2pixelConstantBuffer()
{
    GradientRenderer::PixelConstantBuffer buffer_values;

    auto markers = m_color_markers.getMarkers();
    for (const auto& [i, merker] : markers | std::views::take(std::ssize(markers) - 1) | std::views::enumerate) {
        buffer_values.gradient[i].start_pos   = markers[i].pos;
        buffer_values.gradient[i].stop_pos    = markers[i + 1].pos;
        buffer_values.gradient[i].start_color = {markers[i].value.x, markers[i].value.y, markers[i].value.z, markers[i].value.w};
        buffer_values.gradient[i].stop_color  = {markers[i + 1].value.x, markers[i + 1].value.y, markers[i + 1].value.z, markers[i + 1].value.w};
        buffer_values.gradient[i].ratio       = markers[i].midpoint.ratio;
    }
    buffer_values.gradient_num             = static_cast<int32_t>(std::ssize(markers)) - 1;
    buffer_values.gradient_type            = m_color_space;
    buffer_values.interp_dir               = m_interp_dir;
    buffer_values.blur_width               = m_blur_width;
    buffer_values.texture_size[0]          = static_cast<float>(m_texture_width);
    buffer_values.texture_size[1]          = static_cast<float>(m_texture_height);
    buffer_values.gradient_display_size[0] = m_gradient_display_width;
    buffer_values.gradient_display_size[1] = m_gradient_display_height;

    auto alpha_markers = m_alpha_markers.getMarkers();
    auto alpha_sec_num = static_cast<int32_t>(std::ssize(alpha_markers)) - 1;
    for (int32_t i = 0; i < alpha_sec_num; ++i) {
        buffer_values.alpha_stops[i].start_pos   = alpha_markers[i].pos;
        buffer_values.alpha_stops[i].stop_pos    = alpha_markers[i + 1].pos;
        buffer_values.alpha_stops[i].start_value = alpha_markers[i].value.w;
        buffer_values.alpha_stops[i].stop_value  = alpha_markers[i + 1].value.w;
        buffer_values.alpha_stops[i].mid_ratio   = alpha_markers[i].midpoint.ratio;
    }
    buffer_values.alpha_sec_num    = alpha_sec_num;
    buffer_values.alpha_blur_width = m_alpha_blur_width;

    return buffer_values;
}

bool GradientData::init(Microsoft::WRL::ComPtr<ID3D11Device> d3d_device, const int32_t texture_width, const int32_t texture_height)
{
    int32_t new_width  = (texture_width > 0) ? texture_width : 1;
    int32_t new_height = (texture_height > 0) ? texture_height : 1;

    // サイズが変わっているかチェック
    if (m_texture_width != new_width || m_texture_height != new_height) {
        // サイズが違う場合は古いリソースを解放する
        m_srv.Reset();
        m_output_srv.Reset();
        m_rtv.Reset();

        // 新しいサイズを保存
        m_texture_width  = new_width;
        m_texture_height = new_height;
    }

    // コンスタントバッファーを初期化
    if (!m_pixel_constant_buffer) {
        if (!GradientRenderer::initPixelConstantBuffer(d3d_device, m_pixel_constant_buffer)) {
            OutputDebugStringA("Failed to initialize constant buffer\n");
            return false;
        }
    }

    // 入力用テクスチャを作成
    UINT white_color[4] = {255, 255, 255, 255};
    if (!m_srv) {
        auto result = GradientRenderer::createSolidColorTexture(d3d_device, m_texture_width, m_texture_height, white_color, nullptr, m_srv.ReleaseAndGetAddressOf());
        if (!result) {
            OutputDebugStringA(result.error().c_str());
            return false;
        }
    }

    // 出力用テクスチャを作成
    if (!m_rtv) {
        auto result = GradientRenderer::createSolidColorTexture(d3d_device, m_texture_width, m_texture_height, white_color, m_rtv.ReleaseAndGetAddressOf(), m_output_srv.ReleaseAndGetAddressOf());
        if (!result) {
            OutputDebugStringA(result.error().c_str());
            return false;
        }
    }
    return true;
}

std::vector<float> GradientData::getTextureColor(Microsoft::WRL::ComPtr<ID3D11Device> d3d_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_device_context, const int32_t x, const int32_t y)
{
    Microsoft::WRL::ComPtr<ID3D11Resource> resource;
    m_rtv->GetResource(&resource);
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texure;
    resource.As(&texure);

    auto result = GradientRenderer::readPixelColorFromTexture2D(d3d_device, d3d_device_context, texure.Get(), x, y);
    if (result) {
        return result.value();
    } else {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
}
