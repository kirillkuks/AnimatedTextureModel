#include "pch.h"

#include "ToneMapPostProcess.h"
#include "ShaderStructures.h"
#include "Utils.h"

ToneMapPostProcess::ToneMapPostProcess()
{};

HRESULT ToneMapPostProcess::CreateDeviceDependentResources(ID3D11Device* device)
{
    HRESULT hr = S_OK;

    std::vector<BYTE> bytes;

    // Create the vertex shader
    hr = CreateVertexShader(device, L"CopyVertexShader.cso", bytes, &m_pVertexShader);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader
    hr = CreatePixelShader(device, L"ToneMapPixelShader.cso", bytes, &m_pPixelShader);
    if (FAILED(hr))
        return hr;

    // Create the sampler state
    D3D11_SAMPLER_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
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

    m_pAverageLuminance = std::unique_ptr<AverageLuminanceProcess>(new AverageLuminanceProcess());
    hr = m_pAverageLuminance->CreateDeviceDependentResources(device);
    if (FAILED(hr))
        return hr;

    // Create the constant buffer for average luminance
    CD3D11_BUFFER_DESC albd(sizeof(LuminanceConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    hr = device->CreateBuffer(&albd, nullptr, &m_pLuminanceBuffer);
    
    return hr;
}

HRESULT ToneMapPostProcess::CreateWindowSizeDependentResources(ID3D11Device* device, UINT width, UINT height)
{
    HRESULT hr = S_OK;

    hr = m_pAverageLuminance->CreateWindowSizeDependentResources(device, width, height);

    return hr;
}

void ToneMapPostProcess::Process(ID3D11DeviceContext* context, ID3D11ShaderResourceView* sourceTexture, ID3D11RenderTargetView* renderTarget, D3D11_VIEWPORT viewport)
{
    float averageLuminance = m_pAverageLuminance->Process(context, sourceTexture);

    LuminanceConstantBuffer luminanceBufferData = { averageLuminance };
    context->UpdateSubresource(m_pLuminanceBuffer.Get(), 0, nullptr, &luminanceBufferData, 0, 0);

    context->OMSetRenderTargets(1, &renderTarget, nullptr);
    context->RSSetViewports(1, &viewport);

	D3D11_RECT rect;
	rect.left = rect.top = 0;
	rect.right = (UINT)viewport.Width;
	rect.bottom = (UINT)viewport.Height;
	context->RSSetScissorRects(1, &rect);

    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
    context->PSSetConstantBuffers(0, 1, m_pLuminanceBuffer.GetAddressOf());
    context->PSSetShaderResources(0, 1, &sourceTexture);
    context->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());
    
    context->Draw(4, 0);

    ID3D11ShaderResourceView* nullsrv[] = { nullptr };
    context->PSSetShaderResources(0, 1, nullsrv);
}

ToneMapPostProcess::~ToneMapPostProcess()
{}
