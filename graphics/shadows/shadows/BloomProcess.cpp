#include "pch.h"

#include "BloomProcess.h"
#include "ShaderStructures.h"
#include "Utils.h"

BloomProcess::BloomProcess()
{};

HRESULT BloomProcess::CreateDeviceDependentResources(ID3D11Device* device)
{
    HRESULT hr = S_OK;

    m_pBloomTexture = std::unique_ptr<RenderTexture>(new RenderTexture(DXGI_FORMAT_R32G32B32A32_FLOAT));
    m_pMaskTextures[0] = std::unique_ptr<RenderTexture>(new RenderTexture(DXGI_FORMAT_R32G32B32A32_FLOAT));
    m_pMaskTextures[1] = std::unique_ptr<RenderTexture>(new RenderTexture(DXGI_FORMAT_R32G32B32A32_FLOAT));
    m_pResultTexture = std::unique_ptr<RenderTexture>(new RenderTexture(DXGI_FORMAT_R32G32B32A32_FLOAT));

    std::vector<BYTE> bytes;

    hr = CreateComputeShader(device, L"BlurComputeShader.cso", bytes, &m_pBlurComputeShader);
    if (FAILED(hr))
        return hr;

    hr = CreateComputeShader(device, L"BloomAddComputeShader.cso", bytes, &m_pAddComputeShader);
    if (FAILED(hr))
        return hr;
    
    CD3D11_BUFFER_DESC cb(sizeof(BlurConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    hr = device->CreateBuffer(&cb, nullptr, &m_pConstantBuffer);

    return hr;
}

HRESULT BloomProcess::CreateWindowSizeDependentResources(ID3D11Device* device, UINT width, UINT height)
{
    HRESULT hr = S_OK;

    hr = m_pBloomTexture->CreateResources(device, width, height);
    if (FAILED(hr))
        return hr;

    hr = m_pMaskTextures[0]->CreateResources(device, width, height);
    if (FAILED(hr))
        return hr;

    hr = m_pMaskTextures[1]->CreateResources(device, width, height);
    if (FAILED(hr))
        return hr;

    hr = m_pResultTexture->CreateResources(device, width, height);

    return hr;
}

void BloomProcess::Process(ID3D11DeviceContext* context, RenderTexture* sourceTexture, D3D11_VIEWPORT viewport)
{
    float blackColour[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context->ClearRenderTargetView(m_pMaskTextures[0]->GetRenderTargetView(), blackColour);
    context->ClearRenderTargetView(m_pMaskTextures[1]->GetRenderTargetView(), blackColour);
    context->ClearRenderTargetView(m_pResultTexture->GetRenderTargetView(), blackColour);

    context->CopyResource(m_pMaskTextures[0]->GetRenderTarget(), m_pBloomTexture->GetRenderTarget());

    BlurConstantBuffer blurData;
    blurData.ImageSize = DirectX::XMUINT2(static_cast<UINT>(viewport.Width), static_cast<UINT>(viewport.Height));

    context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &blurData, 0, 0);

    ID3D11UnorderedAccessView* nulluav[1] = { nullptr };
    ID3D11ShaderResourceView* nullsrv[1] = { nullptr };

    UINT steps = 3;
    UINT maskInd = 0;
    for (UINT step = 0; step < steps; ++step, maskInd = 1 - maskInd)
    {
        ID3D11ShaderResourceView* srv = m_pMaskTextures[maskInd]->GetShaderResourceView();
        ID3D11UnorderedAccessView* uav = m_pMaskTextures[1 - maskInd]->GetUnorderedAccessView();

        context->CSSetShader(m_pBlurComputeShader.Get(), nullptr, 0);
        context->CSSetShaderResources(0, 1, &srv);
        context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
        context->CSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

        context->Dispatch((blurData.ImageSize.x + 64 - 1) / 64, blurData.ImageSize.y, 1);

        context->CSSetShader(nullptr, nullptr, 0);
        context->CSSetUnorderedAccessViews(0, 1, nulluav, nullptr);
        context->CSSetShaderResources(0, 1, nullsrv);
    }

    ID3D11ShaderResourceView* srv0 = m_pMaskTextures[maskInd]->GetShaderResourceView();
    ID3D11ShaderResourceView* srv1 = sourceTexture->GetShaderResourceView();
    ID3D11UnorderedAccessView* uav = m_pResultTexture->GetUnorderedAccessView();

    context->CSSetShader(m_pAddComputeShader.Get(), nullptr, 0);
    context->CSSetShaderResources(0, 1, &srv0);
    context->CSSetShaderResources(1, 1, &srv1);
    context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

    context->Dispatch((blurData.ImageSize.x + 64 - 1) / 64, blurData.ImageSize.y, 1);

    context->CSSetShader(nullptr, nullptr, 0);
    context->CSSetUnorderedAccessViews(0, 1, nulluav, nullptr);
    context->CSSetShaderResources(0, 1, nullsrv);
    context->CSSetShaderResources(1, 1, nullsrv);

    context->CopyResource(sourceTexture->GetRenderTarget(), m_pResultTexture->GetRenderTarget());
}

BloomProcess::~BloomProcess()
{}
