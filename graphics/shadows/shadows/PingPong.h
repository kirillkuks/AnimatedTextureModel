#pragma once

#include <d3d11.h>
#include <dxgi.h>

class PingPong
{
public:
	enum class ResourceType
	{
		SOURCE = 0,
		TARGET = 1
	};

	PingPong();
	~PingPong();

	HRESULT SetupResources(ResourceType type, ID3D11Texture2D* texture, ID3D11RenderTargetView* textureRTV, ID3D11ShaderResourceView* textureSRV);

	ID3D11Texture2D* SourceTexture() const;
	ID3D11Texture2D* TargetTexture() const;

	ID3D11RenderTargetView* RenderTargetView() const;

	ID3D11ShaderResourceView* SourceSRV() const;
	ID3D11ShaderResourceView* TargetSRV() const;

	void Swap();

private:
	struct DXResources
	{
		DXResources();
		~DXResources();

		ID3D11Texture2D* m_pTexture;
		ID3D11RenderTargetView* m_pTextureRTV;
		ID3D11ShaderResourceView* m_pTextureSRV;
	};

	DXResources* m_pSource;
	DXResources* m_pTarget;

};
