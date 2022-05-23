#pragma once

class DeviceResources
{
public:
    DeviceResources();
    ~DeviceResources();

    HRESULT CreateDeviceResources();
    HRESULT CreateWindowResources(HWND hWnd);

    HRESULT OnResize();

    float GetAspectRatio();
    UINT  GetWidth();
    UINT  GetHeight();

    ID3D11Device*              GetDevice() const            { return m_pd3dDevice.Get(); };
    ID3D11DeviceContext*       GetDeviceContext() const     { return m_pd3dDeviceContext.Get(); };
    IDXGISwapChain*            GetSwapChain() const         { return m_pSwapChain.Get(); };
    ID3D11RenderTargetView*    GetRenderTarget() const      { return m_pRenderTargetView.Get(); };
    ID3D11DepthStencilView*    GetDepthStencil() const      { return m_pDepthStencilView.Get(); };
    ID3D11DepthStencilState*   GetTransDepthStencil() const { return m_pTransDepthStencilState.Get(); };
    ID3DUserDefinedAnnotation* GetAnnotation() const        { return m_pAnnotation.Get(); };

    D3D11_VIEWPORT GetViewPort() const { return m_viewport; };

    void Present();

private:
    HRESULT ConfigureBackBuffer();

    Microsoft::WRL::ComPtr<ID3D11Device>            m_pd3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>     m_pd3dDeviceContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain>          m_pSwapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_pRenderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_pDepthStencilView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_pTransDepthStencilState;

    Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> m_pAnnotation;

    D3D_FEATURE_LEVEL    m_featureLevel;
    D3D11_TEXTURE2D_DESC m_backBufferDesc;
    D3D11_VIEWPORT       m_viewport;
};
