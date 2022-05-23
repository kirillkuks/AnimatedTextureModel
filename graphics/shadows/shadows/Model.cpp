#include "pch.h"

#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "Model.h"
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_WRITE_IMPLEMENTATION
#undef TINYGLTF_IMPLEMENTATION

#include "Utils.h"

Model::Model(const char* modelPath, const std::shared_ptr<ModelShaders>& modelShaders, DirectX::XMMATRIX globalWorldMatrix) :
    m_modelPath(modelsPath + modelPath),
    m_globalWorldMatrix(globalWorldMatrix),
    m_pModelShaders(modelShaders),
    m_max(),
    m_min()
{};

HRESULT Model::CreateDeviceDependentResources(ID3D11Device* device)
{
    HRESULT hr = S_OK;

    tinygltf::TinyGLTF loader;

    tinygltf::Model model;

    bool ret = loader.LoadASCIIFromFile(&model, nullptr, nullptr, m_modelPath.c_str());
    if (!ret)
        return E_FAIL;

    m_pShaderResourceViews.resize(model.images.size());

    hr = CreateSamplerState(device, model);
    if (FAILED(hr))
        return hr;

    hr = CreateMaterials(device, model);
    if (FAILED(hr))
        return hr;

    m_max = DirectX::XMVectorSet(-INFINITY, -INFINITY, -INFINITY, 0);
    m_min = DirectX::XMVectorSet(INFINITY, INFINITY, INFINITY, 0);

    hr = CreatePrimitives(device, model);

    return hr;
}

HRESULT Model::CreateTexture(ID3D11Device* device, tinygltf::Model& model, size_t imageIdx, bool useSRGB)
{
    // All images have 8 bits per channel and 4 components
    HRESULT hr = S_OK;

    if (m_pShaderResourceViews[imageIdx])
        return hr;

    tinygltf::Image& gltfImage = model.images[imageIdx];

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    DXGI_FORMAT format = useSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    CD3D11_TEXTURE2D_DESC td(format, gltfImage.width, gltfImage.height, 1, 1, D3D11_BIND_SHADER_RESOURCE);
    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = gltfImage.image.data();
    initData.SysMemPitch = 4 * gltfImage.width;
    hr = device->CreateTexture2D(&td, &initData, &texture);
    if (FAILED(hr))
        return hr;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResource;
    CD3D11_SHADER_RESOURCE_VIEW_DESC srvd(D3D11_SRV_DIMENSION_TEXTURE2D, td.Format);
    hr = device->CreateShaderResourceView(texture.Get(), &srvd, &shaderResource);
    if (FAILED(hr))
        return hr;
    m_pShaderResourceViews[imageIdx] = shaderResource;

    return hr;
}

D3D11_TEXTURE_ADDRESS_MODE GetTextureAddressMode(int wrap)
{
    switch (wrap)
    {
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        return D3D11_TEXTURE_ADDRESS_MIRROR;
    default:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    }
}

HRESULT Model::CreateSamplerState(ID3D11Device* device, tinygltf::Model& model)
{
    // In model only one sampler is used
    HRESULT hr = S_OK;

    tinygltf::Sampler& gltfSampler = model.samplers[0];
    D3D11_SAMPLER_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    
    switch (gltfSampler.minFilter)
    {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        else
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        else
            sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        else
            sd.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        else
            sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        else
            sd.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        else
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    default:
        sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
    }

    sd.AddressU = GetTextureAddressMode(gltfSampler.wrapS);
    sd.AddressV = GetTextureAddressMode(gltfSampler.wrapT);
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    hr = device->CreateSamplerState(&sd, &m_pSamplerState);

    return hr;
}

HRESULT Model::CreateMaterials(ID3D11Device* device, tinygltf::Model& model)
{
    HRESULT hr = S_OK;

    for (tinygltf::Material& gltfMaterial : model.materials)
    {
        Material material = {};
        material.name = gltfMaterial.name;
        material.blend = false;

        D3D11_BLEND_DESC bd = {};
        // All materials have alpha mode "BLEND" or "OPAQUE"
        if (gltfMaterial.alphaMode == "BLEND")
        {
            material.blend = true;
            bd.RenderTarget[0].BlendEnable = true;
            bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            hr = device->CreateBlendState(&bd, &material.pBlendState);
            if (FAILED(hr))
                return hr;
        }

        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        if (gltfMaterial.doubleSided)
            rd.CullMode = D3D11_CULL_NONE;
        else
            rd.CullMode = D3D11_CULL_BACK;
        rd.FrontCounterClockwise = true;
        rd.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
        rd.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
        rd.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        hr = device->CreateRasterizerState(&rd, &material.pRasterizerState);
        if (FAILED(hr))
            return hr;

        float albedo[4];
        for (UINT i = 0; i < 4; ++i)
            albedo[i] = static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor[i]);
        material.materialBufferData.Albedo = DirectX::XMFLOAT4(albedo);
        material.materialBufferData.Metalness = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
        material.materialBufferData.Roughness = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);

        material.pixelShaderDefinesFlags = 0;

        material.baseColorTexture = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
        if (material.baseColorTexture >= 0)
        {
            material.pixelShaderDefinesFlags |= ModelShaders::MATERIAL_HAS_COLOR_TEXTURE;
            hr = CreateTexture(device, model, material.baseColorTexture, true);
            if (FAILED(hr))
                return hr;
        }

        material.metallicRoughnessTexture = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
        if (material.metallicRoughnessTexture >= 0)
        {
            material.pixelShaderDefinesFlags |= ModelShaders::MATERIAL_HAS_METAL_ROUGH_TEXTURE;
            hr = CreateTexture(device, model, material.metallicRoughnessTexture);
            if (FAILED(hr))
                return hr;
        }

        material.normalTexture = gltfMaterial.normalTexture.index;
        if (material.normalTexture >= 0)
        {
            material.pixelShaderDefinesFlags |= ModelShaders::MATERIAL_HAS_NORMAL_TEXTURE;
            hr = CreateTexture(device, model, material.normalTexture);
            if (FAILED(hr))
                return hr;
        }

        if (gltfMaterial.occlusionTexture.index >= 0)
            material.pixelShaderDefinesFlags |= ModelShaders::MATERIAL_HAS_OCCLUSION_TEXTURE;

        hr = m_pModelShaders->CreatePixelShader(device, material.pixelShaderDefinesFlags);
        if (FAILED(hr))
            return hr;

        material.emissiveTexture = gltfMaterial.emissiveTexture.index;
        if (material.emissiveTexture >= 0)
        {
            hr = CreateTexture(device, model, material.emissiveTexture, true);
            if (FAILED(hr))
                return hr;
        }

        m_materials.push_back(material);
    }

    return hr;
}

DirectX::XMMATRIX GetMatrixFromNode(tinygltf::Node& gltfNode)
{
    if (gltfNode.matrix.empty())
    {
        DirectX::XMMATRIX rotation;
        DirectX::XMMATRIX translation;
        DirectX::XMMATRIX scale;
        
        if (gltfNode.rotation.empty())
            rotation = DirectX::XMMatrixIdentity();
        else
        {
            float v[4] = {};
            float* p = v;
            for (double value : gltfNode.rotation)
            {
                *p = static_cast<float>(value);
                ++p;
            }
            DirectX::XMFLOAT4 vector(v);
            rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&vector));
        }

        if (gltfNode.translation.empty())
            translation = DirectX::XMMatrixIdentity();
        else
            translation = DirectX::XMMatrixTranslation(static_cast<float>(gltfNode.translation[0]), static_cast<float>(gltfNode.translation[1]), static_cast<float>(gltfNode.translation[2]));

        if (gltfNode.scale.empty())
            scale = DirectX::XMMatrixIdentity();
        else
            scale = DirectX::XMMatrixScaling(static_cast<float>(gltfNode.scale[0]), static_cast<float>(gltfNode.scale[1]), static_cast<float>(gltfNode.scale[2]));

        //return DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(translation, rotation), scale);
        return DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(scale, rotation), translation);
    }
    else
    {
        float flat[16] = {};
        float* p = flat;
        for (double value : gltfNode.matrix)
        {
            *p = static_cast<float>(value);
            ++p;
        }
        DirectX::XMFLOAT4X4 matrix(flat);
        return DirectX::XMLoadFloat4x4(&matrix);
    }
}

HRESULT Model::CreatePrimitive(ID3D11Device* device, tinygltf::Model& model, tinygltf::Primitive& gltfPrimitive, UINT matrix)
{
    HRESULT hr = S_OK;

    Primitive primitive = {};
    primitive.matrix = matrix;

    for (std::pair<const std::string, int>& item : gltfPrimitive.attributes)
    {
        if (item.first == "TEXCOORD_1") // It isn't used in model
            continue;

        tinygltf::Accessor& gltfAccessor = model.accessors[item.second];
        tinygltf::BufferView& gltfBufferView = model.bufferViews[gltfAccessor.bufferView];
        tinygltf::Buffer& gltfBuffer = model.buffers[gltfBufferView.buffer];

        Attribute attribute = {};
        attribute.byteStride = static_cast<UINT>(gltfAccessor.ByteStride(gltfBufferView));

        CD3D11_BUFFER_DESC vbd(attribute.byteStride * static_cast<UINT>(gltfAccessor.count), D3D11_BIND_VERTEX_BUFFER);
        vbd.StructureByteStride = attribute.byteStride;
        D3D11_SUBRESOURCE_DATA initData;
        ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
        initData.pSysMem = &gltfBuffer.data[gltfBufferView.byteOffset + gltfAccessor.byteOffset];
        hr = device->CreateBuffer(&vbd, &initData, &attribute.pVertexBuffer);
        if (FAILED(hr))
            return hr;

        primitive.attributes.push_back(attribute);

        if (item.first == "POSITION")
        {
            primitive.vertexCount = static_cast<UINT>(gltfAccessor.count);
            
            DirectX::XMFLOAT3 maxPosition(static_cast<float>(gltfAccessor.maxValues[0]), static_cast<float>(gltfAccessor.maxValues[1]), static_cast<float>(gltfAccessor.maxValues[2]));
            DirectX::XMFLOAT3 minPosition(static_cast<float>(gltfAccessor.minValues[0]), static_cast<float>(gltfAccessor.minValues[1]), static_cast<float>(gltfAccessor.minValues[2]));

            DirectX::XMMATRIX world = DirectX::XMMatrixMultiply(m_worldMatricies[primitive.matrix], m_globalWorldMatrix);
            primitive.max = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&maxPosition), world);
            primitive.min = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&minPosition), world);

            for (size_t i = 0; i < 3; ++i)
            {
                m_max.m128_f32[i] = max(m_max.m128_f32[i], max(primitive.max.m128_f32[i], primitive.min.m128_f32[i]));
                m_min.m128_f32[i] = min(m_min.m128_f32[i], min(primitive.max.m128_f32[i], primitive.min.m128_f32[i]));
            }
        }
    }

    switch (gltfPrimitive.mode)
    {
    case TINYGLTF_MODE_POINTS:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        break;
    case TINYGLTF_MODE_LINE:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        break;
    case TINYGLTF_MODE_LINE_STRIP:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        break;
    case TINYGLTF_MODE_TRIANGLES:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        break;
    case TINYGLTF_MODE_TRIANGLE_STRIP:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        break;
    }

    tinygltf::Accessor& gltfAccessor = model.accessors[gltfPrimitive.indices];
    tinygltf::BufferView& gltfBufferView = model.bufferViews[gltfAccessor.bufferView];
    tinygltf::Buffer& gltfBuffer = model.buffers[gltfBufferView.buffer];

    primitive.indexCount = static_cast<uint32_t>(gltfAccessor.count);
    UINT stride = 2;
    switch (gltfAccessor.componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        primitive.indexFormat = DXGI_FORMAT_R8_UINT;
        stride = 1;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        primitive.indexFormat = DXGI_FORMAT_R16_UINT;
        stride = 2;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        primitive.indexFormat = DXGI_FORMAT_R32_UINT;
        stride = 4;
        break;
    }

    CD3D11_BUFFER_DESC ibd(stride * primitive.indexCount, D3D11_BIND_INDEX_BUFFER);
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
    initData.pSysMem = &gltfBuffer.data[gltfBufferView.byteOffset + gltfAccessor.byteOffset];
    hr = device->CreateBuffer(&ibd, &initData, &primitive.pIndexBuffer);
    if (FAILED(hr))
        return hr;

    primitive.material = gltfPrimitive.material;
    if (m_materials[primitive.material].blend)
    {
        m_transparentPrimitives.push_back(primitive);
        if (m_materials[primitive.material].emissiveTexture >= 0)
            m_emissiveTransparentPrimitives.push_back(primitive);
    }
    else
    {
        m_primitives.push_back(primitive);
        if (m_materials[primitive.material].emissiveTexture >= 0)
            m_emissivePrimitives.push_back(primitive);
    }

    return hr;
}

HRESULT Model::ProcessNode(ID3D11Device* device, tinygltf::Model& model, int node, DirectX::XMMATRIX worldMatrix)
{
    HRESULT hr = S_OK;

    tinygltf::Node& gltfNode = model.nodes[node];

    if (gltfNode.mesh >= 0)
    {
        tinygltf::Mesh& gltfMesh = model.meshes[gltfNode.mesh];
        UINT matrixIndex = static_cast<UINT>(m_worldMatricies.size() - 1);

        for (tinygltf::Primitive& gltfPrimitive : gltfMesh.primitives)
        {
            // Node with mesh can has matrix but it isn't our case
            hr = CreatePrimitive(device, model, gltfPrimitive, matrixIndex);
            if (FAILED(hr))
                return hr;
        }
    }
    
    if (gltfNode.children.size() > 0)
    {
        worldMatrix = DirectX::XMMatrixMultiplyTranspose(DirectX::XMMatrixTranspose(worldMatrix), DirectX::XMMatrixTranspose(GetMatrixFromNode(gltfNode)));
        m_worldMatricies.push_back(worldMatrix);
        for (int childNode : gltfNode.children)
        {
            hr = ProcessNode(device, model, childNode, worldMatrix);
            if (FAILED(hr))
                return hr;
        }
    }

    return hr;
}

HRESULT Model::CreatePrimitives(ID3D11Device* device, tinygltf::Model& model)
{
    HRESULT hr = S_OK;

    tinygltf::Scene& gltfScene = model.scenes[model.defaultScene];
    
    m_worldMatricies.push_back(DirectX::XMMatrixIdentity());
    
    for (int node : gltfScene.nodes)
    {
        hr = ProcessNode(device, model, node, m_worldMatricies[0]);
        if (FAILED(hr))
            return hr;
    }

    return hr;
}

void Model::Render(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots, bool emissive, bool usePS)
{
    context->VSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.materialConstantBufferSlot, 1, &materialConstantBuffer);
    context->PSSetSamplers(slots.samplerStateSlot, 1, m_pSamplerState.GetAddressOf());

    transformationData.World = DirectX::XMMatrixIdentity();
    std::vector<Primitive>& primitives = emissive ? m_emissivePrimitives : m_primitives;
    for (Primitive& primitive : primitives)
    {
        RenderPrimitive(primitive, context, transformationData, transformationConstantBuffer, materialConstantBuffer, slots, emissive, usePS);
    }

    // for (size_t i = 0; i < primitives.size(); ++i)
    // {
    //     RenderPrimitive(primitives[i], context, transformationData, transformationConstantBuffer, materialConstantBuffer, slots, emissive, usePS);
    // }

    // if (primitives.size() > 3)
    // {
    //     RenderPrimitive(primitives[3], context, transformationData, transformationConstantBuffer, materialConstantBuffer, slots, emissive, usePS);
    // }
}

bool CompareDistancePairs(const std::pair<float, size_t>& p1, const std::pair<float, size_t>& p2)
{
    return p1.first < p2.first;
}

void Model::RenderTransparent(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots, DirectX::XMVECTOR cameraDir, bool emissive, bool usePS)
{
    context->VSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.materialConstantBufferSlot, 1, &materialConstantBuffer);
    context->PSSetSamplers(slots.samplerStateSlot, 1, m_pSamplerState.GetAddressOf());

    transformationData.World = DirectX::XMMatrixIdentity();

    std::vector<Primitive>& primitives = emissive ? m_emissiveTransparentPrimitives : m_transparentPrimitives;
    
    std::vector<std::pair<float, size_t>> distances;
    float distance;
    DirectX::XMVECTOR center;
    DirectX::XMVECTOR cameraPos = DirectX::XMLoadFloat4(&transformationData.CameraPos);
    for (size_t i = 0; i < primitives.size(); ++i)
    {
        center = DirectX::XMVectorDivide(DirectX::XMVectorAdd(primitives[i].max, primitives[i].min), DirectX::XMVectorReplicate(2));
        distance = DirectX::XMVector3Dot(DirectX::XMVectorSubtract(center, cameraPos), cameraDir).m128_f32[0];
        distances.push_back(std::pair<float, size_t>(distance, i));
    }

    std::sort(distances.begin(), distances.end(), CompareDistancePairs);

    for (auto iter = distances.rbegin(); iter != distances.rend(); ++iter)
        RenderPrimitive(primitives[(*iter).second], context, transformationData, transformationConstantBuffer, materialConstantBuffer, slots, emissive, usePS);
}

void Model::RenderPrimitive(Primitive& primitive, ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer& transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots& slots, bool emissive, bool usePS)
{
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
            materialBufferData.Albedo = DirectX::XMFLOAT4(1, 1, 1, materialBufferData.Albedo.w);
            context->PSSetShaderResources(slots.baseColorTextureSlot, 1, m_pShaderResourceViews[material.emissiveTexture].GetAddressOf());
            context->PSSetShader(m_pModelShaders->GetEmissivePixelShader(), nullptr, 0);
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
        }
        context->RSSetState(material.pRasterizerState.Get());
    }
    else
        context->PSSetShader(nullptr, nullptr, 0);
    context->UpdateSubresource(materialConstantBuffer, 0, NULL, &material.materialBufferData, 0, 0);

    context->DrawIndexed(primitive.indexCount, 0, 0);

    if (material.blend)
        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

Model::~Model()
{}
