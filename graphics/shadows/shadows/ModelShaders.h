#pragma once

#include <vector>

class ModelShaders
{
public:
    enum MODEL_PIXEL_SHADER_DEFINES
    {
        MATERIAL_HAS_COLOR_TEXTURE = 0x1,
        MATERIAL_HAS_METAL_ROUGH_TEXTURE = 0x2,
        MATERIAL_HAS_NORMAL_TEXTURE = 0x4,
        MATERIAL_HAS_OCCLUSION_TEXTURE = 0x8,
        
        MODEL_HAS_ANIMATED_TEXTURE = 0x16
    } MODEL_PIXEL_SHADER_DEFINES;

    ModelShaders();
    ~ModelShaders();

    HRESULT CreateDeviceDependentResources(ID3D11Device* device);

    HRESULT CreatePixelShader(ID3D11Device* device, UINT definesFlags);

    ID3D11InputLayout* GetInputLayout() const { return m_pInputLayout.Get(); };
    ID3D11VertexShader* GetVertexShader() const { return m_pVertexShader.Get(); };
    ID3D11PixelShader* GetEmissivePixelShader() const { return m_pEmissivePixelShader.Get(); };
    ID3D11PixelShader* GetPixelShader(UINT definesFlags) const { return m_pPixelShaders[definesFlags].Get(); };

private:
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_pInputLayout;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pEmissivePixelShader;

    std::vector<Microsoft::WRL::ComPtr<ID3D11PixelShader>> m_pPixelShaders;
};
