#pragma once

#include "pch.h"

#include "ShaderStructures.h"
#include "ModelShaders.h"
#include "../../tiny_gltf.h"

const std::string modelsPath = srcPath + "../../models/";

class Model
{
public:
    struct ShadersSlots
    {
        UINT baseColorTextureSlot;
        UINT metallicRoughnessTextureSlot;
        UINT normalTextureSlot;
        UINT samplerStateSlot;
        UINT transformationConstantBufferSlot;
        UINT materialConstantBufferSlot;
    };

    Model(const char* modelPath, const std::shared_ptr<ModelShaders>& modelShaders, DirectX::XMMATRIX globalWorldMatrix = DirectX::XMMatrixIdentity());
    ~Model();

    HRESULT CreateDeviceDependentResources(ID3D11Device* device);

    virtual void Render(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots, bool emissive = false, bool usePS = true);
    void RenderTransparent(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots, DirectX::XMVECTOR cameraDir, bool emissive = false, bool usePS = true);

    DirectX::XMVECTOR GetMaximumPosition() const { return m_max; };
    DirectX::XMVECTOR GetMinimumPosition() const { return m_min; };

protected:
    struct Material
    {
        std::string name;
        bool blend;
        Microsoft::WRL::ComPtr<ID3D11BlendState> pBlendState;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> pRasterizerState;
        MaterialConstantBuffer materialBufferData;
        int baseColorTexture;
        int metallicRoughnessTexture;
        int normalTexture;
        int emissiveTexture;
        UINT pixelShaderDefinesFlags;
    };

    struct Attribute
    {
        UINT byteStride;
        Microsoft::WRL::ComPtr<ID3D11Buffer> pVertexBuffer;
    };

    struct Primitive
    {
        std::vector<Attribute> attributes;
        UINT vertexCount;
        DirectX::XMVECTOR max;
        DirectX::XMVECTOR min;
        D3D11_PRIMITIVE_TOPOLOGY primitiveTopology;
        DXGI_FORMAT indexFormat;
        Microsoft::WRL::ComPtr<ID3D11Buffer> pIndexBuffer;
        UINT indexCount;
        UINT material;
        UINT matrix;
    };

    HRESULT CreateTexture(ID3D11Device* device, tinygltf::Model& model, size_t imageIdx, bool useSRGB = false);
    HRESULT CreateSamplerState(ID3D11Device* device, tinygltf::Model& model);
    HRESULT CreateMaterials(ID3D11Device* device, tinygltf::Model& model);
    HRESULT CreatePrimitives(ID3D11Device* device, tinygltf::Model& model);
    HRESULT ProcessNode(ID3D11Device* device, tinygltf::Model& model, int node, DirectX::XMMATRIX worldMatrix);
    
    virtual HRESULT CreatePrimitive(ID3D11Device* device, tinygltf::Model& model, tinygltf::Primitive& gltfPrimitive, UINT matrix);
    
    virtual void RenderPrimitive(Primitive& primitive,
        ID3D11DeviceContext* context,
        WorldViewProjectionConstantBuffer& transformationData,
        ID3D11Buffer* transformationConstantBuffer,
        ID3D11Buffer* materialConstantBuffer,
        ShadersSlots& slots,
        bool emissive = false,
        bool usePS = true);

    std::string m_modelPath;

    std::shared_ptr<ModelShaders> m_pModelShaders;

    std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_pShaderResourceViews;
    
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerState;

    std::vector<Material> m_materials;

    std::vector<DirectX::XMMATRIX> m_worldMatricies;
    
    std::vector<Primitive> m_primitives;
    std::vector<Primitive> m_transparentPrimitives;
    std::vector<Primitive> m_emissivePrimitives;
    std::vector<Primitive> m_emissiveTransparentPrimitives;

    DirectX::XMMATRIX m_globalWorldMatrix;

    DirectX::XMVECTOR m_max;
    DirectX::XMVECTOR m_min;
};
