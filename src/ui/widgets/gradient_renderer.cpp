#include "gradient_renderer.h"

bool GradientRenderer::init(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_device_context,
    RenderResources& resources)
{
    if (!d3d_device) {
        OutputDebugStringA("DirectX11 device is null");
        return false;
    }

    if (!d3d_device_context) {
        OutputDebugStringA("DirectX11 device context is null");
        return false;
    }

    // ピクセルシェーダーを作成
    if (!resources.pixel_shader) {
        auto result = createPixelShader(d3d_device, resources.pixel_shader.ReleaseAndGetAddressOf());
        if (!result) {
            OutputDebugStringA(result.error().c_str());
            return false;
        }
    }

    // 頂点シェーダーを作成
    if (!resources.vertex_shader) {
        auto result = createVertexShader(
            d3d_device,
            resources.vertex_shader.ReleaseAndGetAddressOf(),
            resources.vertex_buffer.ReleaseAndGetAddressOf(),
            resources.input_layout.ReleaseAndGetAddressOf());
        if (!result) {
            OutputDebugStringA(result.error().c_str());
            return false;
        }
    }

    // サンプラーステートを作成
    if (!resources.sampler_state) {
        auto result = createSamplerState(d3d_device, resources.sampler_state.ReleaseAndGetAddressOf());
        if (!result) {
            OutputDebugStringA(result.error().c_str());
            return false;
        }
    }

    // ブレンドステートを作成
    if (!resources.blend_state) {
        auto result = createBlendState(d3d_device, resources.blend_state.ReleaseAndGetAddressOf());
        if (!result) {
            OutputDebugStringA(result.error().c_str());
            return false;
        }
    }

    return true;
}

std::expected<std::monostate, std::string> GradientRenderer::createSolidColorTexture(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
    uint64_t width,
    uint64_t height,
    UINT32 color[4],
    ID3D11RenderTargetView** out_rtv,
    ID3D11ShaderResourceView** out_srv)
{
    if (width == 0 || height == 0 || width > 16384 || height > 16384) {
        return std::unexpected{"Invalid texture size"};
    }

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width            = static_cast<UINT>(width);
    desc.Height           = static_cast<UINT>(height);
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_DEFAULT;
    desc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags   = 0;

    ID3D11Texture2D* p_texture = NULL;
    D3D11_SUBRESOURCE_DATA sub_resource;

    uint64_t buf_size = width * height * 4;
    std::vector<BYTE> pixels(buf_size);

    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            UINT index        = static_cast<UINT>((y * width + x) * 4);
            pixels[index + 0] = static_cast<BYTE>(color[0]);
            pixels[index + 1] = static_cast<BYTE>(color[1]);
            pixels[index + 2] = static_cast<BYTE>(color[2]);
            pixels[index + 3] = static_cast<BYTE>(color[3]);
        }
    }

    sub_resource.pSysMem          = pixels.data();
    sub_resource.SysMemPitch      = static_cast<UINT>(width * 4);  // 1行あたりのバイト数
    sub_resource.SysMemSlicePitch = 0;
    HRESULT hr                    = d3d_device->CreateTexture2D(&desc, &sub_resource, &p_texture);
    if (FAILED(hr)) {
        return std::unexpected{"Failed create texture."};
    }

    // レンダーターゲットビューを作成
    hr = d3d_device->CreateRenderTargetView(p_texture, nullptr, out_rtv);
    if (FAILED(hr)) {
        p_texture->Release();
        return std::unexpected{"Failed create render target view."};
    }

    // シェーダーリソースビューを作成
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ZeroMemory(&srv_desc, sizeof(srv_desc));
    srv_desc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels       = desc.MipLevels;
    srv_desc.Texture2D.MostDetailedMip = 0;
    hr                                 = d3d_device->CreateShaderResourceView(p_texture, &srv_desc, out_srv);
    if (FAILED(hr)) {
        p_texture->Release();
        return std::unexpected{"Failed create shader resource view."};
    }

    p_texture->Release();
    return std::monostate{};
}

std::expected<std::monostate, std::string> GradientRenderer::createPixelShader(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
    ID3D11PixelShader** out_pixel_shader)
{
    HRESULT hr = d3d_device->CreatePixelShader(&g_psmain, sizeof(g_psmain), nullptr, out_pixel_shader);
    if (FAILED(hr)) {
        return std::unexpected{"Failed to create pixel shader."};
    }

    return std::monostate{};
}

std::expected<std::monostate, std::string> GradientRenderer::initPixelConstantBuffer(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
    Microsoft::WRL::ComPtr<ID3D11Buffer>& pixel_constant_buffer)
{
    D3D11_BUFFER_DESC desc   = {};
    desc.ByteWidth           = sizeof(PixelConstantBuffer);
    desc.Usage               = D3D11_USAGE_DYNAMIC;
    desc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;

    HRESULT hr = d3d_device->CreateBuffer(&desc, NULL, pixel_constant_buffer.ReleaseAndGetAddressOf());

    if (FAILED(hr) || pixel_constant_buffer == nullptr) {
        return std::unexpected{"Failed to create pixel constant buffer."};
    }

    return std::monostate{};
}

std::expected<std::monostate, std::string> GradientRenderer::updatePixelConstantBuffer(
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_device_context,
    ID3D11Buffer* pixel_constant_buffer,
    const PixelConstantBuffer* buffer_values)
{
    if (!pixel_constant_buffer) {
        return std::unexpected{"pixelConstantBuffer is null."};
    }
    if (!buffer_values) {
        return std::unexpected{"bufferValues is null."};
    }

    D3D11_MAPPED_SUBRESOURCE mapped_resource{};
    HRESULT hr = d3d_device_context->Map(pixel_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
    if (FAILED(hr)) {
        return std::unexpected{"Failed to map pixel constant buffer."};
    }

    PixelConstantBuffer* data_ptr = (PixelConstantBuffer*)mapped_resource.pData;
    std::memcpy(data_ptr->gradient, buffer_values->gradient, sizeof(data_ptr->gradient));
    data_ptr->gradient_num  = buffer_values->gradient_num;
    data_ptr->gradient_type = buffer_values->gradient_type;
    data_ptr->interp_dir    = buffer_values->interp_dir;
    data_ptr->blur_width    = buffer_values->blur_width;
    std::memcpy(data_ptr->texture_size, buffer_values->texture_size, sizeof(data_ptr->texture_size));
    std::memcpy(data_ptr->gradient_display_size, buffer_values->gradient_display_size, sizeof(data_ptr->gradient_display_size));

    d3d_device_context->Unmap(pixel_constant_buffer, 0);
    d3d_device_context->PSSetConstantBuffers(0, 1, &pixel_constant_buffer);

    return std::monostate{};
}

std::expected<std::monostate, std::string> GradientRenderer::initVertexBuffer(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
    ID3D11Buffer** out_vertex_buffer,
    ID3D11InputLayout** out_input_layout)
{
    const Vertex vertices[6] = {
        // 位置情報              座標情報
        //    x,     y,    z       u,    v
        {-1.0f, 1.0f, 0.0f, 0.0f, 0.0f},   // 左上
        {1.0f, 1.0f, 0.0f, 1.0f, 0.0f},    // 右上
        {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},  // 左下

        {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},  // 左下
        {1.0f, 1.0f, 0.0f, 1.0f, 0.0f},    // 右上
        {1.0f, -1.0f, 0.0f, 1.0f, 1.0f},   // 右下
    };

    // 頂点バッファの作成
    D3D11_BUFFER_DESC bd = {};
    bd.Usage             = D3D11_USAGE_DEFAULT;
    bd.ByteWidth         = sizeof(Vertex) * static_cast<int>(std::size(vertices));  // データ全体のサイズ
    bd.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem                = vertices;

    HRESULT hr = d3d_device->CreateBuffer(&bd, &initData, out_vertex_buffer);
    if (FAILED(hr)) {
        return std::unexpected{"Failed to create vertex buffer."};
    }

    // Input Layout の作成
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        // SemanticName, Index, Format, Slot, Offset, ...
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    // レイアウトを作成
    hr = d3d_device->CreateInputLayout(layout, 2, &g_vsmain, sizeof(g_vsmain), out_input_layout);
    if (FAILED(hr)) {
        return std::unexpected{"Failed to create input layout."};
    }

    return std::monostate{};
}

std::expected<std::monostate, std::string> GradientRenderer::createVertexShader(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
    ID3D11VertexShader** out_vertex_shader,
    ID3D11Buffer** out_vertex_buffer,
    ID3D11InputLayout** out_input_layout)
{
    HRESULT hr = d3d_device->CreateVertexShader(&g_vsmain, sizeof(g_vsmain), NULL, out_vertex_shader);
    if (FAILED(hr)) {
        return std::unexpected{"Failed to create vertex shader."};
    }

    // 頂点バッファを初期化
    auto result = initVertexBuffer(d3d_device, out_vertex_buffer, out_input_layout);
    if (!result) {
        return std::unexpected{"Failed to init vertex buffer."};
    }

    return std::monostate{};
}

std::expected<std::monostate, std::string> GradientRenderer::createBlendState(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
    ID3D11BlendState** out_blend_state)
{
    D3D11_BLEND_DESC desc                      = {};
    desc.RenderTarget[0].BlendEnable           = TRUE;
    desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = d3d_device->CreateBlendState(&desc, out_blend_state);
    if (FAILED(hr)) {
        return std::unexpected{"create blend state error"};
    }

    return std::monostate{};
}

std::expected<std::monostate, std::string> GradientRenderer::createSamplerState(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
    ID3D11SamplerState** out_sampler_state)
{
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter             = D3D11_FILTER_MIN_MAG_MIP_POINT;
    desc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.ComparisonFunc     = D3D11_COMPARISON_NEVER;
    desc.MinLOD             = 0;
    desc.MaxLOD             = D3D11_FLOAT32_MAX;
    HRESULT hr              = d3d_device->CreateSamplerState(&desc, out_sampler_state);
    if (FAILED(hr)) {
        return std::unexpected{"create sampler state error"};
    }

    return std::monostate{};
}

void GradientRenderer::runOffscreenRendering(
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_device_context,
    const RenderResources& resources,
    ID3D11Buffer* pixel_constant_buffer,
    PixelConstantBuffer* buffer_values,
    uint32_t width,
    uint32_t height,
    ID3D11RenderTargetView* rtv,
    ID3D11ShaderResourceView* srv)
{
    // 現在のステートを保存
    D3DStateSaver saver(d3d_device_context);

    // クリア
    float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    d3d_device_context->ClearRenderTargetView(rtv, clear_color);

    // 描画設定の切り替え
    // レンダーターゲット設定
    d3d_device_context->OMSetRenderTargets(1, &rtv, nullptr);

    // ビューポート設定
    D3D11_VIEWPORT vp = {};
    vp.Width          = (float)width;
    vp.Height         = (float)height;
    vp.MinDepth       = 0.0f;
    vp.MaxDepth       = 1.0f;
    d3d_device_context->RSSetViewports(1, &vp);

    // コンスタントバッファー
    if (buffer_values) {
        auto result = updatePixelConstantBuffer(d3d_device_context, pixel_constant_buffer, buffer_values);
        if (!result) {
            OutputDebugStringA(result.error().c_str());
        }
    }

    // シェーダー設定
    d3d_device_context->VSSetShader(resources.vertex_shader.Get(), nullptr, 0);
    d3d_device_context->PSSetShader(resources.pixel_shader.Get(), nullptr, 0);

    // 入力テクスチャ設定
    ID3D11ShaderResourceView* srvs[] = {srv};
    d3d_device_context->PSSetShaderResources(0, 1, srvs);

    // サンプラー設定
    d3d_device_context->PSSetSamplers(0, 1, resources.sampler_state.GetAddressOf());

    // 描画コマンド
    d3d_device_context->IASetInputLayout(resources.input_layout.Get());
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    d3d_device_context->IASetVertexBuffers(0, 1, resources.vertex_buffer.GetAddressOf(), &stride, &offset);
    d3d_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3d_device_context->Draw(6, 0);

    // SRV を解除
    ID3D11ShaderResourceView* null_srv[1] = {nullptr};
    d3d_device_context->PSSetShaderResources(0, 1, null_srv);
}

std::expected<std::vector<float>, std::string> GradientRenderer::readPixelColorFromTexture2D(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device,
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_device_context,
    ID3D11Texture2D* source_texture,
    int32_t x,
    int32_t y)
{
    std::vector<float> color(4);

    // CPU 読み取り用のステージングテクスチャを作る
    ID3D11Texture2D* staging_texture = nullptr;
    D3D11_TEXTURE2D_DESC desc;
    source_texture->GetDesc(&desc);  // 元のフォーマットを保持

    desc.Width          = 1;  // 1ピクセル分だけ確保
    desc.Height         = 1;
    desc.MipLevels      = 1;
    desc.ArraySize      = 1;
    desc.Usage          = D3D11_USAGE_STAGING;    // CPUからアクセスできるように設定
    desc.BindFlags      = 0;                      // シェーダー入力や描画先にはしない
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;  // 読み取り許可
    desc.MiscFlags      = 0;

    if (FAILED(d3d_device->CreateTexture2D(&desc, nullptr, &staging_texture))) {
        return std::unexpected{"create texture2d error"};
    }

    // GPU 上で特定の1ピクセルだけをステージングテクスチャにコピー
    D3D11_BOX source_region;
    source_region.left   = x;
    source_region.right  = x + 1;
    source_region.top    = y;
    source_region.bottom = y + 1;
    source_region.front  = 0;
    source_region.back   = 1;

    // sourceTexture の (x,y) から stagingTexture の (0,0) へコピー
    d3d_device_context->CopySubresourceRegion(staging_texture, 0, 0, 0, 0, source_texture, 0, &source_region);

    // CPUメモリにマップして読み取る
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(d3d_device_context->Map(staging_texture, 0, D3D11_MAP_READ, 0, &mapped))) {
        // データの先頭ポインタ取得
        unsigned char* data = (unsigned char*)mapped.pData;

        unsigned char r = data[0];
        unsigned char g = data[1];
        unsigned char b = data[2];
        unsigned char a = data[3];

        color[0] = r / 255.0f, color[1] = g / 255.0f, color[2] = b / 255.0f, color[3] = a / 255.0f;

        d3d_device_context->Unmap(staging_texture, 0);
    }
    staging_texture->Release();

    return color;
}
