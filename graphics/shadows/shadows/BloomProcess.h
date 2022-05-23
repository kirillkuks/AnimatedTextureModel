#pragma once

#include "DeviceResources.h"
#include "RenderTexture.h"

class BloomProcess
{
public:
    BloomProcess();
    ~BloomProcess();

    HRESULT CreateDeviceDependentResources(ID3D11Device* device);
    HRESULT CreateWindowSizeDependentResources(ID3D11Device* device, UINT width, UINT height);

    ID3D11RenderTargetView* GetBloomRenderTargetView() const { return m_pBloomTexture->GetRenderTargetView(); };

    void Process(ID3D11DeviceContext* context, RenderTexture* sourceTexture, D3D11_VIEWPORT viewport);

private:
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_pBlurComputeShader;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_pAddComputeShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_pConstantBuffer;

    std::unique_ptr<RenderTexture> m_pBloomTexture;
    std::unique_ptr<RenderTexture> m_pMaskTextures[2];
    std::unique_ptr<RenderTexture> m_pResultTexture;
};
