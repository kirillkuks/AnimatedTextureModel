#pragma once

#include "DeviceResources.h"

class RenderTexture
{
public:
    RenderTexture(DXGI_FORMAT format);
    ~RenderTexture();

    HRESULT CreateResources(ID3D11Device* device, UINT width, UINT height);

    ID3D11Texture2D*           GetRenderTarget() const        { return m_pRenderTarget.Get(); };
    ID3D11RenderTargetView*    GetRenderTargetView() const    { return m_pRenderTargetView.Get(); };
    ID3D11ShaderResourceView*  GetShaderResourceView() const  { return m_pShaderResourceView.Get(); };
    ID3D11UnorderedAccessView* GetUnorderedAccessView() const { return m_pUnorderedAccessView.Get(); };
    
    D3D11_VIEWPORT GetViewPort() const { return m_viewport; };

private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D>           m_pRenderTarget;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>    m_pRenderTargetView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  m_pShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_pUnorderedAccessView;

    DXGI_FORMAT    m_format;
    D3D11_VIEWPORT m_viewport;
};
