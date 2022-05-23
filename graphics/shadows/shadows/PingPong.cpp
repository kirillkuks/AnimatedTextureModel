#include "PingPong.h"
#include <vector>

PingPong::PingPong() : m_pSource{ nullptr }, m_pTarget{ nullptr } {}

PingPong::~PingPong()
{
	if (m_pSource)
	{
		delete m_pSource;
	}

	if (m_pTarget)
	{
		delete m_pTarget;
	}
}

PingPong::DXResources::DXResources() : m_pTexture(nullptr), m_pTextureRTV(nullptr), m_pTextureSRV(nullptr) {}

PingPong::DXResources::~DXResources()
{
	if (m_pTexture)
	{
		m_pTexture->Release();
		m_pTexture = nullptr;
	}

	if (m_pTextureRTV)
	{
		m_pTextureRTV->Release();
		m_pTextureRTV = nullptr;
	}

	if (m_pTextureSRV)
	{
		m_pTextureSRV->Release();
		m_pTextureSRV = nullptr;
	}
}

void PingPong::Swap()
{
	std::swap(m_pSource, m_pTarget);
}

HRESULT PingPong::SetupResources(ResourceType type, ID3D11Texture2D* texture, ID3D11RenderTargetView* textureRTV, ID3D11ShaderResourceView* textureSRV) {
	DXResources* resources = new DXResources();
	resources->m_pTexture = texture;
	resources->m_pTextureRTV = textureRTV;
	resources->m_pTextureSRV = textureSRV;

	if (type == ResourceType::SOURCE)
	{
		if (m_pSource)
		{
			delete m_pSource;
		}

		m_pSource = resources;
	}
	if (type == ResourceType::TARGET)
	{
		if (m_pTarget)
		{
			delete m_pTarget;
		}

		m_pTarget = resources;
	}

	return S_OK;
}

ID3D11Texture2D* PingPong::SourceTexture() const
{
	return m_pSource->m_pTexture;
}

ID3D11Texture2D* PingPong::TargetTexture() const
{
	return m_pTarget->m_pTexture;
}

ID3D11RenderTargetView* PingPong::RenderTargetView() const
{
	return m_pTarget->m_pTextureRTV;
}

ID3D11ShaderResourceView* PingPong::SourceSRV() const
{
	return m_pSource->m_pTextureSRV;
}

ID3D11ShaderResourceView* PingPong::TargetSRV() const
{
	return m_pTarget->m_pTextureSRV;
}
