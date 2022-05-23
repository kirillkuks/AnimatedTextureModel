#pragma once

#include "DeviceResources.h"
#include "RenderTexture.h"
#include "ShaderStructures.h"
#include "ToneMapPostProcess.h"
#include "Camera.h"
#include "BloomProcess.h"
#include "Settings.h"
#include "Model.h"
#include "AnimatedTexture.h"

class Renderer
{
public:
    Renderer(const std::shared_ptr<DeviceResources>& deviceResources, const std::shared_ptr<Camera>& camera, const std::shared_ptr<Settings>& settings);
    ~Renderer();

    HRESULT CreateDeviceDependentResources();
    HRESULT CreateWindowSizeDependentResources();
    
    HRESULT OnResize();

    HRESULT Update();

    void Render();

private:
    HRESULT CreateShaders();
    HRESULT CreateSphere();
    HRESULT CreatePlane();
    HRESULT CreateLights();
    HRESULT CreateTexture();
    HRESULT CreateCubeTexture();
    HRESULT CreateIrradianceTexture();
    HRESULT CreatePrefilteredColorTexture();
    HRESULT CreatePreintegratedBRDFTexture();
    HRESULT CreateCubeTextureFromResource(UINT size, ID3D11Texture2D* dst, ID3D11ShaderResourceView* src, ID3D11VertexShader* vs, ID3D11PixelShader* ps, UINT mipSlice = 0);
    HRESULT CreateModels();
    HRESULT CreateShadows();

    void UpdatePerspective();

    void Clear();
    void RenderSphere(WorldViewProjectionConstantBuffer& transformationData, bool usePS = true);
    void RenderModels();
    void RenderEnvironment();
    void RenderPlane();
    void RenderSimpleShadow();
    void RenderPSSM();
    void PostProcessTexture();

    std::unique_ptr<RenderTexture>      m_pRenderTexture;
    std::unique_ptr<ToneMapPostProcess> m_pToneMap;
    std::unique_ptr<BloomProcess>       m_pBloom;
    std::shared_ptr<Camera>             m_pCamera;
    std::shared_ptr<DeviceResources>    m_pDeviceResources;
    std::shared_ptr<Settings>           m_pSettings;
    std::shared_ptr<ModelShaders>       m_pModelShaders;

    Microsoft::WRL::ComPtr<ID3D11InputLayout>        m_pInputLayout;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>        m_pIBLInputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pEnvironmentVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pPlaneVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pIndexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pPlaneIndexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pEnvironmentTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pEnvironmentShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pEnvironmentCubeTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pEnvironmentCubeShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pIrradianceTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pIrradianceShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pPrefilteredColorTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pPrefilteredColorShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pPreintegratedBRDFTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pPreintegratedBRDFShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pPlaneShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pSimpleShadowMapTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   m_pSimpleShadowMapDepthStencilView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pSimpleShadowMapShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pPSSMTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pPSSMShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>    m_pSimpleShadowMapRasterizerState;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>       m_pPBRVertexShader;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>       m_pEnvironmentVertexShader;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>       m_pIBLVertexShader;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>       m_pEnvironmentCubeVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pPlanePixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pNDPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pGPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pFPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pEnvironmentPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pIrradiancePixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pPrefilteredColorPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pPreintegratedBRDFPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pEnvironmentCubePixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pLightBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pMaterialBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pShadowBuffer;

    std::vector<Microsoft::WRL::ComPtr<ID3D11SamplerState>>       m_pSamplerStates;
    std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>>   m_pPSSMDepthStencilViews;

    std::vector<std::unique_ptr<Model>> m_pModels;

    WorldViewProjectionConstantBuffer m_constantBufferData;
    LightConstantBuffer               m_lightBufferData;
    MaterialConstantBuffer            m_materialBufferData;
    ShadowConstantBuffer              m_shadowBufferData;
    
    UINT32 m_indexCount;
    UINT32 m_planeIndexCount;
    UINT32 m_frameCount;

    DirectX::XMVECTOR m_sceneCenter;
    FLOAT m_sceneRadius;

    size_t m_usec = 0;

    std::shared_ptr<AnimatedTexture> m_pAnimatedTexture;

    DirectX::XMVECTOR m_targers[6] = {
        DirectX::XMVectorSet(1, 0, 0, 0),
        DirectX::XMVectorSet(-1, 0, 0, 0),
        DirectX::XMVectorSet(0, 1, 0, 0),
        DirectX::XMVectorSet(0, -1, 0, 0),
        DirectX::XMVectorSet(0, 0, 1, 0),
        DirectX::XMVectorSet(0, 0, -1, 0)
    };

    DirectX::XMVECTOR m_ups[6] = {
        DirectX::XMVectorSet(0, 1, 0, 0),
        DirectX::XMVectorSet(0, 1, 0, 0),
        DirectX::XMVectorSet(0, 0, -1, 0),
        DirectX::XMVectorSet(0, 0, 1, 0),
        DirectX::XMVectorSet(0, 1, 0, 0),
        DirectX::XMVectorSet(0, 1, 0, 0)
    };

    DirectX::XMFLOAT3 m_squareLeftBottomAngles[6] = {
        DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f),
        DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f),
        DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f),
        DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f),
        DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f),
        DirectX::XMFLOAT3(0.5f, -0.5f, -0.5f)
    };

    DirectX::XMFLOAT3 m_squareRightTopAngles[6] = {
        DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f),
        DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f),
        DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f),
        DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f),
        DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f),
        DirectX::XMFLOAT3(-0.5f, 0.5f, -0.5f)
    };
};
