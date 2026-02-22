#include "core/d3d_manager.h"

#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace gradient_editor {

bool D3DManager::initialize(HWND hwnd)
{
    return createDeviceD3D(hwnd);
}

bool D3DManager::createDeviceD3D(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = 0;
    sd.BufferDesc.Height                  = 0;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    UINT create_device_flags = 0;
#ifndef NDEBUG
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL feature_level;
    const D3D_FEATURE_LEVEL feature_level_array[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, feature_level_array, 2, D3D11_SDK_VERSION, &sd, &m_swap_chain, &m_device, &feature_level, &m_device_context);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, create_device_flags, feature_level_array, 2, D3D11_SDK_VERSION, &sd, &m_swap_chain, &m_device, &feature_level, &m_device_context);
    if (res != S_OK)
        return false;

    createRenderTarget();
    return true;
}

void D3DManager::cleanup()
{
    cleanupRenderTarget();

    if (m_device_context) {
        m_device_context.Reset();
        m_device_context = nullptr;
    }

    if (m_swap_chain) {
        m_swap_chain.Reset();
        m_swap_chain = nullptr;
    }

    // COM オブジェクトの生存状況を出力
    reportLiveDeviceObjectsDetail();

    if (m_device) {
        m_device.Reset();
        m_device = nullptr;
    }

    if (m_debug) {
        m_debug.Reset();
        m_debug = nullptr;
    }
}

void D3DManager::createRenderTarget()
{
    if (!m_swap_chain) {
        return;
    }

    ID3D11Texture2D* back_buffer = nullptr;
    if (SUCCEEDED(m_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer))) && back_buffer && m_device) {
        m_device->CreateRenderTargetView(back_buffer, nullptr, &m_render_target_view);
        back_buffer->Release();
    }
}

void D3DManager::cleanupRenderTarget()
{
    if (m_render_target_view) {
        m_render_target_view.Reset();
        m_render_target_view = nullptr;
    }
}

void D3DManager::reportLiveDeviceObjectsDetail()
{
    if (!m_device) return;

    if (SUCCEEDED(m_device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(m_debug.GetAddressOf())))) {
        if (m_debug) {
            m_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        }
    }
}

void D3DManager::handleWindowResize()
{
    if (m_swap_chain && m_resize_width != 0 && m_resize_height != 0) {
        cleanupRenderTarget();
        m_swap_chain->ResizeBuffers(0, m_resize_width, m_resize_height, DXGI_FORMAT_UNKNOWN, 0);
        m_resize_width = m_resize_height = 0;
        createRenderTarget();
    }
}

}  // namespace gradient_editor
