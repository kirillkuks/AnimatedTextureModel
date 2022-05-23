#pragma once

#include "DeviceResources.h"
#include "AverageLuminanceProcess.h"

class ToneMapPostProcess
{
public:
    ToneMapPostProcess();
    ~ToneMapPostProcess();

    HRESULT CreateDeviceDependentResources(ID3D11Device* device);
    HRESULT CreateWindowSizeDependentResources(ID3D11Device* device, UINT width, UINT height);

    void Process(ID3D11DeviceContext* context, ID3D11ShaderResourceView* sourceTexture, ID3D11RenderTargetView* renderTarget, D3D11_VIEWPORT viewport);

private:
    std::unique_ptr<AverageLuminanceProcess> m_pAverageLuminance;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pPixelShader;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerState;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       m_pLuminanceBuffer;
};
