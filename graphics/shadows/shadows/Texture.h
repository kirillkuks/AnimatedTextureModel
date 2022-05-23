#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <DirectXMath.h>

#include <string>
#include <vector>

#include "FieldSwapper.h"

class Texture
{
protected:
	struct TextureResources
	{
	public:
		ID3D11Texture2D* m_pTexture;
		ID3D11ShaderResourceView* m_pTextureSRV;

	public:
		TextureResources() = default;
		~TextureResources();
	};

protected:
	UINT m_iWidth;
	UINT m_iHeight;

	TextureResources* m_pBackgroundTexture;

public:
	Texture(ID3D11Device* device, ID3D11DeviceContext* context, UINT width, UINT height);
	Texture(ID3D11Device* device, ID3D11DeviceContext* context, std::string const& filename);
	~Texture();

	ID3D11Texture2D* const GetBackgroundTexture() const;
	ID3D11ShaderResourceView* const GetBackGroundTextureSRV() const;

	UINT GetWidth() const;
	UINT GetHeight() const;

	virtual std::vector<ID3D11ShaderResourceView*> GetLayersTargetTexturesSRV() const;
	virtual std::vector<FieldSwapper*> GetFields() const;
};
