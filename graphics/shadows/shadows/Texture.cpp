#include "Texture.h"

#include <assert.h>
#include "../../WICTextureLoader.h"
#include "../../stb_image.h"
#include "../../stb_image_write.h"

Texture::Texture(ID3D11Device* device, ID3D11DeviceContext* context, std::string const& filename) :
	m_pBackgroundTexture{ new TextureResources() },
	m_iWidth{ 0 }, m_iHeight{ 0 }
{


	HRESULT result = CreateWICTextureFromFile(
		device,
		context,
		std::wstring(filename.begin(), filename.end()).c_str(),
		(ID3D11Resource**)&m_pBackgroundTexture->m_pTexture,
		&m_pBackgroundTexture->m_pTextureSRV
	);

	assert(SUCCEEDED(result));
}

Texture::Texture(ID3D11Device* device, ID3D11DeviceContext* context, UINT width, UINT height) :
	m_pBackgroundTexture{ nullptr }, m_iWidth{ width }, m_iHeight{ height }
{

}

Texture::~Texture()
{
	if (m_pBackgroundTexture)
	{
		auto texture = m_pBackgroundTexture->m_pTexture;
		if (texture)
		{
			texture->Release();
			texture = nullptr;
		}

		auto textureSRV = m_pBackgroundTexture->m_pTextureSRV;
		if (textureSRV)
		{
			textureSRV->Release();
			textureSRV = NULL;
		}
	}
}

ID3D11Texture2D* const Texture::GetBackgroundTexture() const
{
	return m_pBackgroundTexture->m_pTexture;
}

ID3D11ShaderResourceView* const Texture::GetBackGroundTextureSRV() const
{
	return m_pBackgroundTexture->m_pTextureSRV;
}

UINT Texture::GetWidth() const
{
	return m_iWidth;
}

UINT Texture::GetHeight() const
{
	return m_iHeight;
}

std::vector<ID3D11ShaderResourceView*> Texture::GetLayersTargetTexturesSRV() const
{
	return {};
}

std::vector<FieldSwapper*> Texture::GetFields() const
{
	return {};
}
