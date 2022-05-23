#include <cassert>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <tchar.h>

#include "AnimatedTexture.h"
#include "../../WICTextureLoader.h"
#include "../../stb_image.h"
#include "../../stb_image_write.h"

#define SAFE_RELEASE(p) \
if (p != NULL) { \
	p->Release(); \
	p = NULL;\
}

using namespace DirectX;

struct TextureVertex
{
	XMVECTORF32 pos;
	XMFLOAT2 uv;
};

AnimatedTexture::TextureResources::~TextureResources()
{
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pTextureSRV);
}

AnimatedTexture::AnimatedTexture(ID3D11Device* device, ID3D11DeviceContext* context, UINT width, UINT height)
	: Texture(device, context, width, height),
	m_pDevice{ device }, m_pContext{ context },
	m_pVertexBuffer{ nullptr }, m_pIndexBuffer{ nullptr }, m_pInputLayout{ nullptr },
	m_pVertexShader{ nullptr }, m_pPixelShader{ nullptr }, m_pDepthStencilView{ nullptr },
	m_pConstantBuffer{ nullptr }, m_pInterpolateBuffer{ nullptr },
	m_pFieldSamplerState{ nullptr }, m_pTextureSamplerState{ nullptr }, m_iInc{ 0 }
{

}

AnimatedTexture::~AnimatedTexture()
{
	for (auto& pingPing : m_aLayerTextures)
	{
		if (pingPing)
		{
			delete pingPing;
		}
	}

	for (auto& field : m_aFieldSwappers)
	{
		if (field)
		{
			delete field;
		}
	}

	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pInputLayout);

	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);

	SAFE_RELEASE(m_pDepthStencilView);

	SAFE_RELEASE(m_pConstantBuffer);
	SAFE_RELEASE(m_pInterpolateBuffer);

	SAFE_RELEASE(m_pFieldSamplerState);
	SAFE_RELEASE(m_pTextureSamplerState);
}

HRESULT AnimatedTexture::CreateAnimationTextureResources(std::string const& vertexShader, std::string const& pixelShader)
{
	static const TextureVertex vertices[4] = {
		{ {-1, -1, 0, 1}, {0, 1} },
		{ {-1, 1, 0, 1}, {0, 0} },
		{ { 1, 1, 0, 1}, {1, 0} },
		{ {1, -1, 0, 1}, {1, 1} }
	};
	static const UINT16 indices[6] = {
		0, 1, 2, 0, 2, 3
	};

	D3D11_BUFFER_DESC vertexBufferDesc{ 0 };
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(vertices);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData{ 0 };
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	HRESULT result = m_pDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &m_pVertexBuffer);

	if (SUCCEEDED(result))
	{
		D3D11_BUFFER_DESC indexBufferDesc{ 0 };
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(indices);
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA indexData{ 0 };
		indexData.pSysMem = indices;
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		result = m_pDevice->CreateBuffer(&indexBufferDesc, &indexData, &m_pIndexBuffer);
	}

	if (SUCCEEDED(result))
	{
		ID3DBlob* pBlob = nullptr;

		m_pVertexShader = CreateVertexShader(std::string(vertexShader.begin(), vertexShader.end()).c_str(), &pBlob);
		m_pPixelShader = CreatePixelShader(std::string(pixelShader.begin(), pixelShader.end()).c_str());

		assert(m_pVertexShader != nullptr && m_pPixelShader != nullptr);

		if (m_pVertexShader == nullptr || m_pPixelShader == nullptr)
		{
			return E_FAIL;
		}

		if (SUCCEEDED(result))
		{
			D3D11_INPUT_ELEMENT_DESC inputDesc[2] = {
				D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0 , D3D11_INPUT_PER_VERTEX_DATA, 0},
				D3D11_INPUT_ELEMENT_DESC{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMVECTORF32), D3D11_INPUT_PER_VERTEX_DATA, 0}
			};

			result = m_pDevice->CreateInputLayout(inputDesc, 2, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &m_pInputLayout);
		}

		SAFE_RELEASE(pBlob);
	}

	if (SUCCEEDED(result))
	{
		ID3D11Texture2D* pTexture = nullptr;

		D3D11_TEXTURE2D_DESC depthDesc = {};
		depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthDesc.ArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Usage = D3D11_USAGE_DEFAULT;
		depthDesc.Height = m_iHeight;
		depthDesc.Width = m_iWidth;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthDesc.CPUAccessFlags = 0;
		depthDesc.MiscFlags = 0;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.SampleDesc.Quality = 0;

		HRESULT result = m_pDevice->CreateTexture2D(&depthDesc, nullptr, &pTexture);

		if (SUCCEEDED(result))
		{
			result = m_pDevice->CreateDepthStencilView(pTexture, nullptr, &m_pDepthStencilView);
		}

		SAFE_RELEASE(pTexture);
	}

	if (SUCCEEDED(result))
	{
		D3D11_BUFFER_DESC desc = { 0 };
		desc.ByteWidth = sizeof(AnimatedTexture::CBuffer);
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;

		result = m_pDevice->CreateBuffer(&desc, nullptr, &m_pConstantBuffer);

		assert(SUCCEEDED(result));

		if (FAILED(result))
		{
			return result;
		}

		desc.ByteWidth = sizeof(AnimatedTexture::InterpolateBuffer);

		result = m_pDevice->CreateBuffer(&desc, nullptr, &m_pInterpolateBuffer);
		assert(SUCCEEDED(result));
	}
	
	if (SUCCEEDED(result)) {
		D3D11_SAMPLER_DESC sd = {};
		ZeroMemory(&sd, sizeof(sd));

		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.MinLOD = 0;
		sd.MaxAnisotropy = 16;
		sd.MaxLOD = D3D11_FLOAT32_MAX;

		result = m_pDevice->CreateSamplerState(&sd, &m_pFieldSamplerState);
		assert(SUCCEEDED(result));

		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

		result = m_pDevice->CreateSamplerState(&sd, &m_pTextureSamplerState);
		assert(SUCCEEDED(result));

		samplers.push_back(m_pFieldSamplerState);
		samplers.push_back(m_pTextureSamplerState);
	}

	return result;
}

void AnimatedTexture::UpdateConstantBuffer(CBuffer const* buffer)
{
	m_pContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, buffer, 0, 0);
}

void AnimatedTexture::UpdateInterpolateBuffer(InterpolateBuffer const* buffer)
{
	m_pContext->UpdateSubresource(m_pInterpolateBuffer, 0, nullptr, buffer, 0, 0);
}

ID3D11Buffer* const* AnimatedTexture::GetInterpolateBufferAdress() const
{
	return &m_pInterpolateBuffer;
}

ID3D11ShaderResourceView* AnimatedTexture::GetBackgroundTextureSRV() const
{
	return m_pBackgroundTexture->m_pTextureSRV;
}

ID3D11Texture2D* AnimatedTexture::GetBackgroundTexture() const
{
	return m_pBackgroundTexture->m_pTexture;
}

ID3D11SamplerState* const* AnimatedTexture::GetSamplerAdress() const
{
	return samplers.data();
}

size_t AnimatedTexture::GetLayersNum() const
{
	return m_aLayerTextures.size();
}

std::vector<ID3D11ShaderResourceView*> AnimatedTexture::GetLayersSourceTexturesSRV() const
{
	std::vector<ID3D11ShaderResourceView*> layers(m_aLayerTextures.size());

	for (size_t i = 0; i < layers.size(); ++i)
	{
		layers[i] = m_aLayerTextures[i]->SourceSRV();
	}

	return layers;
}

std::vector<ID3D11ShaderResourceView*> AnimatedTexture::GetLayersTargetTexturesSRV() const
{
	std::vector<ID3D11ShaderResourceView*> layers(m_aLayerTextures.size());

	for (size_t i = 0; i < layers.size(); ++i)
	{
		layers[i] = m_aLayerTextures[i]->TargetSRV();
	}

	return layers;
}

std::vector<ID3D11RenderTargetView*> AnimatedTexture::GetLayersTexturesRTV() const
{
	std::vector<ID3D11RenderTargetView*> layers(m_aLayerTextures.size());

	for (size_t i = 0; i < layers.size(); ++i)
	{
		layers[i] = m_aLayerTextures[i]->RenderTargetView();
	}

	return layers;
}

std::vector<ID3D11Texture2D*> AnimatedTexture::GetLayersTextures() const
{
	std::vector<ID3D11Texture2D*> layers(m_aLayerTextures.size());

	for (size_t i = 0; i < layers.size(); ++i)
	{
		layers[i] = m_aLayerTextures[i]->TargetTexture();
	}

	return layers;
}

HRESULT AnimatedTexture::CreateVectorFieldTexture(VectorField const* field, FieldSwapper* swapper) const
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.ArraySize = 1;
	desc.MipLevels = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Height = m_iHeight;
	desc.Width = m_iWidth;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = field->raw_data();
	initData.SysMemPitch = 4u * m_iHeight * sizeof(float);
	initData.SysMemSlicePitch = 0;

	ID3D11Texture2D* animatedTextureVectorField = nullptr;
	ID3D11ShaderResourceView* animatedTextureVectorFieldSRV = nullptr;

	HRESULT result = m_pDevice->CreateTexture2D(&desc, &initData, &animatedTextureVectorField);
	assert(SUCCEEDED(result));

	if (SUCCEEDED(result))
	{
		result = m_pDevice->CreateShaderResourceView(animatedTextureVectorField, NULL, &animatedTextureVectorFieldSRV);
	}

	assert(SUCCEEDED(result));

	swapper->AddField(animatedTextureVectorField, animatedTextureVectorFieldSRV);

	return result;
}

std::vector<FieldSwapper*> AnimatedTexture::GetFields() const
{
	return m_aFieldSwappers;
}

void AnimatedTexture::AddBackground(ID3D11Texture2D* texture, ID3D11ShaderResourceView* textureSRV)
{
	assert(texture != nullptr && textureSRV != nullptr);

	if (m_pBackgroundTexture != nullptr)
	{
		delete m_pBackgroundTexture;
	}

	m_pBackgroundTexture = new TextureResources();
	assert(m_pBackgroundTexture);

	m_pBackgroundTexture->m_pTexture = texture;
	m_pBackgroundTexture->m_pTextureSRV = textureSRV;
}

void AnimatedTexture::AddLayer(ID3D11Texture2D* texture, ID3D11ShaderResourceView* textureSRV, ID3D11RenderTargetView* textureRTV)
{
	assert(texture != nullptr && textureSRV != nullptr && textureRTV != nullptr);

	PingPong* pingPong = new PingPong();
	assert(pingPong != nullptr);

	pingPong->SetupResources(PingPong::ResourceType::SOURCE, texture, textureRTV, textureSRV);

	{
		D3D11_TEXTURE2D_DESC depthDesc = {};
		depthDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		depthDesc.ArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Usage = D3D11_USAGE_DEFAULT;
		depthDesc.Height = m_iHeight;
		depthDesc.Width = m_iWidth;
		depthDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		depthDesc.CPUAccessFlags = 0;
		depthDesc.MiscFlags = 0;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.SampleDesc.Quality = 0;

		ID3D11Texture2D* pTextureRenderTarget(nullptr);
		ID3D11RenderTargetView* pTextureRenderTargetRTV(nullptr);
		ID3D11ShaderResourceView* pTextureRenderTargetSRV(nullptr);

		HRESULT result = m_pDevice->CreateTexture2D(&depthDesc, NULL, &pTextureRenderTarget);

		if (SUCCEEDED(result))
		{
			result = m_pDevice->CreateRenderTargetView(pTextureRenderTarget, NULL, &pTextureRenderTargetRTV);
		}
		if (SUCCEEDED(result))
		{
			result = m_pDevice->CreateShaderResourceView(pTextureRenderTarget, NULL, &pTextureRenderTargetSRV);
		}

		assert(SUCCEEDED(result));

		pingPong->SetupResources(PingPong::ResourceType::TARGET, pTextureRenderTarget, pTextureRenderTargetRTV, pTextureRenderTargetSRV);
	}

	m_aLayerTextures.push_back(pingPong);
}

void AnimatedTexture::SetUpFields(std::vector<FieldSwapper*> const& fields)
{
	m_aFieldSwappers = fields;
}

void AnimatedTexture::Swap()
{
	for (auto& pingPong : m_aLayerTextures)
	{
		pingPong->Swap();
	}
}

void AnimatedTexture::SaveIncrement(size_t inc)
{
	m_iInc = inc;
}

void AnimatedTexture::IncrementSaved()
{
	IncrementStep(m_iInc);
}

HRESULT AnimatedTexture::AddBackgroundByName(std::string const& filename)
{
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* textureSRV = nullptr;

	HRESULT result = CreateWICTextureFromFile(
		m_pDevice,
		m_pContext,
		std::wstring(filename.begin(), filename.end()).c_str(),
		(ID3D11Resource**)&texture,
		&textureSRV
	);

	if (SUCCEEDED(result))
	{
		AddBackground(texture, textureSRV);
	}

	return result;
}

HRESULT AnimatedTexture::AddLayerByName(std::string const& filename)
{
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* textureSRV = nullptr;
	ID3D11RenderTargetView* textureRTV = nullptr;

	int x, y, n;
	unsigned char* pixels = stbi_load(filename.c_str(), &x, &y, &n, 0);
	assert(pixels != nullptr);

	HRESULT result = CreateWICTextureFromFile(
		m_pDevice,
		m_pContext,
		std::wstring(filename.begin(), filename.end()).c_str(),
		(ID3D11Resource**)&texture,
		&textureSRV
	);

	delete pixels;

	if (SUCCEEDED(result))
	{
		result = m_pDevice->CreateRenderTargetView(texture, nullptr, &textureRTV);
	}

	if (SUCCEEDED(result))
	{
		AddLayer(texture, textureSRV, textureRTV);
	}

	return result;
}

void AnimatedTexture::Render(ID3D11RasterizerState* pRasterizerState,
	ID3D11SamplerState* pSamplerState,
	ID3D11Buffer* pConstantBuffer)
{
	for (size_t i = 0; i < m_aLayerTextures.size(); ++i)
	{
		m_pContext->ClearState();

		ID3D11RenderTargetView* views[] = { m_aLayerTextures[i]->RenderTargetView() };
		m_pContext->OMSetRenderTargets(1, views, m_pDepthStencilView);

		static const FLOAT blackColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_pContext->ClearRenderTargetView(m_aLayerTextures[i]->RenderTargetView(), blackColor);
		m_pContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		D3D11_VIEWPORT viewport{ 0, 0, (float)m_iWidth, (float)m_iHeight, 0.0f, 1.0f };
		m_pContext->RSSetViewports(1, &viewport);
		D3D11_RECT rect{ 0, 0, (LONG)m_iWidth, (LONG)m_iHeight };
		m_pContext->RSSetScissorRects(1, &rect);

		RenderTexture(i, pRasterizerState, pSamplerState, m_pConstantBuffer, m_aFieldSwappers[i]);
	}
}

void AnimatedTexture::IncrementStep(size_t incSize)
{
	for (auto& swapper : m_aFieldSwappers)
	{
		swapper->IncStep(incSize);
	}
}

void AnimatedTexture::RenderTexture(size_t textureNum,
	ID3D11RasterizerState* pRasterizerState,
	ID3D11SamplerState* pSamplerState,
	ID3D11Buffer* pConstantBuffer,
	FieldSwapper* pFieldSwapper)
{
	PingPong* curTexture = m_aLayerTextures[textureNum];

	ID3D11Buffer* vertexBuffer[] = { m_pVertexBuffer };
	UINT stride = sizeof(TextureVertex);
	UINT offset = 0;

	m_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	m_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	m_pContext->IASetInputLayout(m_pInputLayout);

	m_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pContext->PSSetShader(m_pPixelShader, nullptr, 0);

	m_pContext->RSSetState(pRasterizerState);
	m_pContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11ShaderResourceView* textures[] = {
		m_aLayerTextures[textureNum]->SourceSRV(),
		m_aFieldSwappers[textureNum]->CurrentVectorFieldSRV(),
		nullptr
	};
	m_pContext->PSSetShaderResources(0, 3, textures);

	ID3D11SamplerState* samplers[] = { pSamplerState, m_pFieldSamplerState };
	m_pContext->PSSetSamplers(0, 2, samplers);

	ID3D11Buffer* buffers[] = { pConstantBuffer };
	m_pContext->PSSetConstantBuffers(0, 1, buffers);

	m_pContext->DrawIndexed(6, 0, 0);
}

ID3D11VertexShader* AnimatedTexture::CreateVertexShader(LPCTSTR shaderSource, ID3DBlob** ppBlob)
{
	ID3D11VertexShader* pVertexShader = nullptr;

	FILE* pFile = nullptr;

	_tfopen_s(&pFile, shaderSource, _T("rb"));
	if (pFile != nullptr)
	{
		fseek(pFile, 0, SEEK_END);
		int size = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		char* pSourceCode = (char*)malloc((size_t)size + 1);
		if (pSourceCode == nullptr)
		{
			return nullptr;
		}

		fread(pSourceCode, size, 1, pFile);
		pSourceCode[size] = 0;

		fclose(pFile);

		ID3DBlob* pError = nullptr;
		HRESULT result = D3DCompile(pSourceCode, size, "", nullptr, nullptr, "VS", "vs_5_0", D3DCOMPILE_SKIP_OPTIMIZATION, 0, ppBlob, &pError);
		assert(SUCCEEDED(result));
		if (!SUCCEEDED(result))
		{
			const char* pMsg = (const char*)pError->GetBufferPointer();
			OutputDebugStringA(pMsg);
		}
		else
		{
			result = m_pDevice->CreateVertexShader((*ppBlob)->GetBufferPointer(), (*ppBlob)->GetBufferSize(), nullptr, &pVertexShader);
		}

		SAFE_RELEASE(pError);
	}

	assert(pVertexShader);
	return pVertexShader;
}

ID3D11PixelShader* AnimatedTexture::CreatePixelShader(LPCTSTR shaderSource)
{
	ID3D11PixelShader* pPixelShader = nullptr;

	FILE* pFile = nullptr;

	_tfopen_s(&pFile, shaderSource, _T("rb"));
	if (pFile != nullptr)
	{
		fseek(pFile, 0, SEEK_END);
		int size = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		char* pSourceCode = (char*)malloc((size_t)size + 1);
		if (pSourceCode == nullptr)
		{
			return nullptr;
		}

		fread(pSourceCode, size, 1, pFile);
		pSourceCode[size] = 0;

		fclose(pFile);

		ID3DBlob* pBlob = nullptr;
		ID3DBlob* pError = nullptr;
		HRESULT result = D3DCompile(pSourceCode, size, "", nullptr, nullptr, "PS", "ps_5_0", D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pBlob, &pError);
		assert(SUCCEEDED(result));
		if (!SUCCEEDED(result))
		{
			const char* pMsg = (const char*)pError->GetBufferPointer();
			OutputDebugStringA(pMsg);
		}
		else
		{
			result = m_pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader);
			assert(SUCCEEDED(result));
		}

		SAFE_RELEASE(pError);
		SAFE_RELEASE(pBlob);
	}

	assert(pPixelShader);
	return pPixelShader;
}
