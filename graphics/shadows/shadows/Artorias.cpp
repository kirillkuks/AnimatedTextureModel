#include "Artorias.h"

#include <iostream>

Artorias::Artorias(const char* modelPath,
	const std::shared_ptr<ModelShaders>& modelShaders,
	DirectX::XMMATRIX globalWorldMatrix)
	: Model(modelPath, modelShaders, globalWorldMatrix)
{

}

Artorias::~Artorias()
{

}

void Artorias::SetAnimatedTexture(std::shared_ptr<AnimatedTexture>& animatedTexture, UINT primitiveNum)
{
    m_pAnimatedTexture = animatedTexture;
    m_animatedPrimitives.push_back(primitiveNum);

    AddEmission(primitiveNum);
}

void Artorias::Render(ID3D11DeviceContext* context,
    WorldViewProjectionConstantBuffer transformationData,
    ID3D11Buffer* transformationConstantBuffer,
    ID3D11Buffer* materialConstantBuffer,
    ShadersSlots slots,
    bool emissive, bool usePS)
{
    context->VSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.materialConstantBufferSlot, 1, &materialConstantBuffer);
    context->PSSetSamplers(slots.samplerStateSlot, 1, m_pSamplerState.GetAddressOf());

    context->PSSetSamplers(5, 2, m_pAnimatedTexture->GetSamplerAdress());

    transformationData.World = DirectX::XMMatrixIdentity();
    std::vector<Model::Primitive> primitives = emissive ? m_emissivePrimitives : m_primitives;

    for (UINT i = 0; i < primitives.size(); ++i)
    {
        RenderPrimitive(primitives[i], context, transformationData, transformationConstantBuffer, materialConstantBuffer, slots,
            emissive, usePS, std::find(m_animatedPrimitives.begin(), m_animatedPrimitives.end(), i) != m_animatedPrimitives.end());
    }

}

void Artorias::RenderPrimitive(Model::Primitive& primitive,
    ID3D11DeviceContext* context,
    WorldViewProjectionConstantBuffer& transformationData,
    ID3D11Buffer* transformationConstantBuffer,
    ID3D11Buffer* materialConstantBuffer,
    ShadersSlots& slots,
    bool emissive,
    bool usePS,
    bool isAnimated)
{
    // AAV Turn animated texture emission on/off
    if (emissive)
    {
        return;
    }

    std::vector<ID3D11Buffer*> combined;
    std::vector<UINT> offset;
    std::vector<UINT> stride;

    size_t attributesCount = primitive.attributes.size();
    combined.resize(attributesCount);
    offset.resize(attributesCount);
    stride.resize(attributesCount);
    for (size_t i = 0; i < attributesCount; ++i)
    {
        combined[i] = primitive.attributes[i].pVertexBuffer.Get();
        stride[i] = primitive.attributes[i].byteStride;
    }
    context->IASetVertexBuffers(0, static_cast<UINT>(attributesCount), combined.data(), stride.data(), offset.data());

    context->IASetIndexBuffer(primitive.pIndexBuffer.Get(), primitive.indexFormat, 0);
    context->IASetPrimitiveTopology(primitive.primitiveTopology);

    Material& material = m_materials[primitive.material];
    if (material.blend)
        context->OMSetBlendState(material.pBlendState.Get(), nullptr, 0xFFFFFFFF);

    context->IASetInputLayout(m_pModelShaders->GetInputLayout());
    context->VSSetShader(m_pModelShaders->GetVertexShader(), nullptr, 0);

    transformationData.World = DirectX::XMMatrixMultiplyTranspose(m_worldMatricies[primitive.matrix], m_globalWorldMatrix);

    context->UpdateSubresource(transformationConstantBuffer, 0, NULL, &transformationData, 0, 0);

    MaterialConstantBuffer materialBufferData = material.materialBufferData;
    if (usePS)
    {
        if (emissive)
        {
            std::vector<ID3D11ShaderResourceView*> textures = std::vector<ID3D11ShaderResourceView*>{ m_pAnimatedTexture->GetLayersTargetTexturesSRV()[0], m_pAnimatedTexture->GetFields()[0]->CurrentVectorFieldSRV() };

            context->PSSetShaderResources(8, 2 * (UINT)m_pAnimatedTexture->GetLayersNum(), textures.data());

            context->PSSetConstantBuffers(4, 1, m_pAnimatedTexture->GetInterpolateBufferAdress());

            context->PSSetShader(m_pModelShaders->GetAnimatedEmissivePixelShader(), nullptr, 0);
        }
        else
        {
            context->PSSetShader(m_pModelShaders->GetPixelShader(material.pixelShaderDefinesFlags), nullptr, 0);
            if (material.baseColorTexture >= 0)
                context->PSSetShaderResources(slots.baseColorTextureSlot, 1, m_pShaderResourceViews[material.baseColorTexture].GetAddressOf());
            if (material.metallicRoughnessTexture >= 0)
                context->PSSetShaderResources(slots.metallicRoughnessTextureSlot, 1, m_pShaderResourceViews[material.metallicRoughnessTexture].GetAddressOf());
            if (material.normalTexture >= 0)
                context->PSSetShaderResources(slots.normalTextureSlot, 1, m_pShaderResourceViews[material.normalTexture].GetAddressOf());

            std::vector<ID3D11ShaderResourceView*> textures = isAnimated ? 
                std::vector<ID3D11ShaderResourceView*>{ m_pAnimatedTexture->GetLayersTargetTexturesSRV()[0], m_pAnimatedTexture->GetFields()[0]->CurrentVectorFieldSRV()} :
                std::vector<ID3D11ShaderResourceView*>{ nullptr, nullptr };

            context->PSSetShaderResources(8, 2 * (UINT)m_pAnimatedTexture->GetLayersNum(), textures.data());

            context->PSSetConstantBuffers(4, 1, m_pAnimatedTexture->GetInterpolateBufferAdress());
        }
        context->RSSetState(material.pRasterizerState.Get());
    }
    else
    {
        context->PSSetShader(nullptr, nullptr, 0);
    }
    context->UpdateSubresource(materialConstantBuffer, 0, NULL, &material.materialBufferData, 0, 0);

    context->DrawIndexed(primitive.indexCount, 0, 0);

    if (material.blend)
        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

HRESULT Artorias::AddEmission(UINT primitiveNum)
{
    m_emissivePrimitives.push_back(m_primitives[primitiveNum]);
    return S_OK;
}
