#include "pch.h"

#include "RenderTexture.h"

RenderTexture::RenderTexture(DXGI_FORMAT format) :
    m_format(format),
    m_viewport()
{};

HRESULT RenderTexture::CreateResources(ID3D11Device* device, UINT width, UINT height)
{
    HRESULT hr = S_OK;

    // Create a render target
    CD3D11_TEXTURE2D_DESC rtd(
        m_format,
        width,
        height,
        1,
        1,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
        D3D11_USAGE_DEFAULT,
        0,
        1
    );

    hr = device->CreateTexture2D(&rtd, nullptr, m_pRenderTarget.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        return hr;

    // Create render target view
    CD3D11_RENDER_TARGET_VIEW_DESC rtvd(D3D11_RTV_DIMENSION_TEXTURE2D, m_format);

    hr = device->CreateRenderTargetView(m_pRenderTarget.Get(), &rtvd, m_pRenderTargetView.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        return hr;

    // Create shader resource view
    CD3D11_SHADER_RESOURCE_VIEW_DESC srvd(D3D11_SRV_DIMENSION_TEXTURE2D, m_format);

    hr = device->CreateShaderResourceView(m_pRenderTarget.Get(), &srvd, m_pShaderResourceView.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        return hr;

    CD3D11_UNORDERED_ACCESS_VIEW_DESC uavd(D3D11_UAV_DIMENSION_TEXTURE2D, m_format);
    hr = device->CreateUnorderedAccessView(m_pRenderTarget.Get(), &uavd, m_pUnorderedAccessView.ReleaseAndGetAddressOf());

    m_viewport.Width = static_cast<FLOAT>(width);
    m_viewport.Height = static_cast<FLOAT>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;

    return hr;
}

RenderTexture::~RenderTexture()
{}
