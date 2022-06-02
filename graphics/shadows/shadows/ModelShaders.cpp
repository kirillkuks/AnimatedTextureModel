#include "pch.h"

#include "ModelShaders.h"

#include "Utils.h"

ModelShaders::ModelShaders()
{};

HRESULT ModelShaders::CreateDeviceDependentResources(ID3D11Device* device)
{
    HRESULT hr = S_OK;

    m_pPixelShaders.resize(32);

    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    std::vector<D3D_SHADER_MACRO> defines;
    defines.push_back({ "HAS_TANGENT", "1" });
    defines.push_back({ nullptr, nullptr });

    hr = CompileShaderFromFile((wsrcPath + L"PBRShaders.fx").c_str(), "vs_main", "vs_5_0", &blob, defines.data());
    if (FAILED(hr))
        return hr;

    hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_pVertexShader);
    if (FAILED(hr))
        return hr;

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD_", 0, DXGI_FORMAT_R32G32_FLOAT, 3, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), blob->GetBufferPointer(), blob->GetBufferSize(), &m_pInputLayout);
    if (FAILED(hr))
        return hr;

    defines.resize(1);
    defines.push_back({ "HAS_EMISSIVE", "1" });
    defines.push_back({ nullptr, nullptr });

    hr = CompileShaderFromFile((wsrcPath + L"PBRShaders.fx").c_str(), "ps_main", "ps_5_0", &blob, defines.data());
    if (FAILED(hr))
        return hr;

    hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_pEmissivePixelShader);
    if (FAILED(hr))
        return hr;

    defines.resize(1);
    defines.push_back({ "HAS_EMISSIVE", "1" });
    defines.push_back({ "HAS_ANIMATED_TEXTURE", "1" });
    defines.push_back({ "HAS_COLOR_TEXTURE", "1" });
    defines.push_back({ nullptr, nullptr });

    hr = CompileShaderFromFile((wsrcPath + L"PBRShaders.fx").c_str(), "ps_main", "ps_5_0", &blob, defines.data());
    if (FAILED(hr))
        return hr;

    hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_pAnimatedEmissivePixelShader);
    if (FAILED(hr))
        return hr;

    return hr;
}

HRESULT ModelShaders::CreatePixelShader(ID3D11Device* device, UINT definesFlags)
{
    HRESULT hr = S_OK;

    if (m_pPixelShaders[definesFlags])
        return hr;

    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    std::vector<D3D_SHADER_MACRO> defines;
    defines.push_back({ "HAS_TANGENT", "1" });

    if (definesFlags & MATERIAL_HAS_COLOR_TEXTURE)
        defines.push_back({ "HAS_COLOR_TEXTURE", "1" });

    if (definesFlags & MATERIAL_HAS_METAL_ROUGH_TEXTURE)
        defines.push_back({ "HAS_METAL_ROUGH_TEXTURE", "1" });

    if (definesFlags & MATERIAL_HAS_NORMAL_TEXTURE)
        defines.push_back({ "HAS_NORMAL_TEXTURE", "1" });

    if (definesFlags & MATERIAL_HAS_OCCLUSION_TEXTURE)
        defines.push_back({ "HAS_OCCLUSION_TEXTURE", "1" });

    if (definesFlags & MODEL_HAS_ANIMATED_TEXTURE)
        defines.push_back({ "HAS_ANIMATED_TEXTURE", "1" });

    defines.push_back({ nullptr, nullptr });

    hr = CompileShaderFromFile((wsrcPath + L"PBRShaders.fx").c_str(), "ps_main", "ps_5_0", &blob, defines.data());
    if (FAILED(hr))
        return hr;

    hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_pPixelShaders[definesFlags]);

    return hr;
}

ModelShaders::~ModelShaders()
{}
