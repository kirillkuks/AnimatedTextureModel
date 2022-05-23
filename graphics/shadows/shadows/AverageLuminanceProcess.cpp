#include "pch.h"

#include <cmath>

#include "AverageLuminanceProcess.h"
#include "Utils.h"

AverageLuminanceProcess::AverageLuminanceProcess() :
    m_adaptedLuminance(0.0)
{
    QueryPerformanceFrequency(&m_qpcFrequency);
    QueryPerformanceCounter(&m_qpcLastTime);
}

HRESULT AverageLuminanceProcess::CreateDeviceDependentResources(ID3D11Device* device)
{
    HRESULT hr = S_OK;

    std::vector<BYTE> bytes;

    // Create the vertex shader
    hr = CreateVertexShader(device, L"CopyVertexShader.cso", bytes, &m_pVertexShader);
    if (FAILED(hr))
        return hr;

    // Create the copy pixel shader
    hr = CreatePixelShader(device, L"CopyPixelShader.cso", bytes, &m_pCopyPixelShader);
    if (FAILED(hr))
        return hr;

    // Create the luminance pixel shader
    hr = CreatePixelShader(device, L"LuminancePixelShader.cso", bytes, &m_pLuminancePixelShader);
    if (FAILED(hr))
        return hr;

    // Create the sampler state
    D3D11_SAMPLER_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    sd.MaxAnisotropy = D3D11_MAX_MAXANISOTROPY;
    hr = device->CreateSamplerState(&sd, m_pSamplerState.GetAddressOf());
    if (FAILED(hr))
        return hr;

    CD3D11_TEXTURE2D_DESC ltd(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        1,
        1,
        1,
        1,
        0,
        D3D11_USAGE_STAGING,
        D3D11_CPU_ACCESS_READ
    );
    hr = device->CreateTexture2D(&ltd, nullptr, &m_pLuminanceTexture);
	if (FAILED(hr))
		return hr;

	D3D11_RASTERIZER_DESC rd;
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;
	rd.DepthBias = 0;
	rd.DepthBiasClamp = 0;
	rd.SlopeScaledDepthBias = 0;
	rd.DepthClipEnable = true;
	hr = device->CreateRasterizerState(&rd, &m_pRasterizerState);
	if (FAILED(hr))
		return hr;

    return hr;
}

HRESULT AverageLuminanceProcess::CreateWindowSizeDependentResources(ID3D11Device* device, UINT width, UINT height)
{
    HRESULT hr = S_OK;

    size_t minSize = static_cast<size_t>(min(width, height));
    size_t n;
    for (n = 0; static_cast<size_t>(1) << (n + 1) <= minSize; n++);
    m_renderTextures.clear();
    m_renderTextures.reserve(n + 2);

    UINT size = 1 << n;
    RenderTexture initTexture(DXGI_FORMAT_R32G32B32A32_FLOAT);
    hr = initTexture.CreateResources(device, size, size);
    if (FAILED(hr))
        return hr;
    m_renderTextures.push_back(initTexture);
    
    for (size_t i = 0; i <= n; i++)
    {
        size = 1 << (n - i);
        RenderTexture texture(DXGI_FORMAT_R32G32B32A32_FLOAT);
        hr = texture.CreateResources(device, size, size);
        if (FAILED(hr))
            return hr;
        m_renderTextures.push_back(texture);
    }

    return hr;
}

void AverageLuminanceProcess::CopyTexture(ID3D11DeviceContext* context, ID3D11ShaderResourceView* sourceTexture, RenderTexture& dst, ID3D11PixelShader* pixelShader)
{
    ID3D11RenderTargetView* renderTarget = dst.GetRenderTargetView();

    D3D11_VIEWPORT viewport = dst.GetViewPort();

    context->OMSetRenderTargets(1, &renderTarget, nullptr);
    context->RSSetViewports(1, &viewport);

	D3D11_RECT rect;
	rect.left = rect.top = 0;
	rect.right = (UINT)viewport.Width;
	rect.bottom = (UINT)viewport.Height;
	context->RSSetScissorRects(1, &rect);

    context->PSSetShader(pixelShader, nullptr, 0);
    context->PSSetShaderResources(0, 1, &sourceTexture);
    
    context->Draw(4, 0);

    ID3D11ShaderResourceView* nullsrv[] = { nullptr };
    context->PSSetShaderResources(0, 1, nullsrv);
}

float AverageLuminanceProcess::Process(ID3D11DeviceContext* context, ID3D11ShaderResourceView* sourceTexture)
{
    if (m_renderTextures.size() == 0)
        return m_adaptedLuminance;

    float backgroundColour[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    for (size_t i = 0; i < m_renderTextures.size(); i++)
        context->ClearRenderTargetView(m_renderTextures[i].GetRenderTargetView(), backgroundColour);

    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);

    context->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());

    context->RSSetState(m_pRasterizerState.Get());

    CopyTexture(context, sourceTexture, m_renderTextures[0], m_pCopyPixelShader.Get());
    CopyTexture(context, m_renderTextures[0].GetShaderResourceView(), m_renderTextures[1], m_pLuminancePixelShader.Get());

    for (size_t i = 2; i < m_renderTextures.size(); i++)
    {
        CopyTexture(context, m_renderTextures[i - 1].GetShaderResourceView(), m_renderTextures[i], m_pCopyPixelShader.Get());
    }

    ID3D11ShaderResourceView* nullsrv[] = { nullptr };
    context->PSSetShaderResources(0, 1, nullsrv);

    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    size_t timeDelta = currentTime.QuadPart - m_qpcLastTime.QuadPart;
    m_qpcLastTime = currentTime;
    double delta = static_cast<double>(timeDelta) / m_qpcFrequency.QuadPart;

    D3D11_MAPPED_SUBRESOURCE luminanceAccessor;
    context->CopyResource(m_pLuminanceTexture.Get(), m_renderTextures[m_renderTextures.size() - 1].GetRenderTarget());
    context->Map(m_pLuminanceTexture.Get(), 0, D3D11_MAP_READ, 0, &luminanceAccessor);
    float luminance = *(float*)luminanceAccessor.pData;
    context->Unmap(m_pLuminanceTexture.Get(), 0);

    float sigma = 0.04f / (0.04f + luminance);
    float tau = sigma * 0.4f + (1 - sigma) * 0.1f;
    m_adaptedLuminance += (luminance - m_adaptedLuminance) * static_cast<float>(1 - std::exp(-delta * tau));
    return m_adaptedLuminance;
}

AverageLuminanceProcess::~AverageLuminanceProcess()
{}
