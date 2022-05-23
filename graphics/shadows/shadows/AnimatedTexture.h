#pragma once

#include <d3d11.h>
#include <dxgi.h>

#include <vector>
#include <string>

#include "Texture.h"
#include "FieldSwapper.h"
#include "PingPong.h"
#include "VectorField.h"


class AnimatedTexture : public Texture
{
private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pContext;

	std::vector<PingPong*> m_aLayerTextures;
	std::vector<FieldSwapper*> m_aFieldSwappers;

	// Render stuff
	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;
	ID3D11InputLayout* m_pInputLayout;

	ID3D11VertexShader* m_pVertexShader;
	ID3D11PixelShader* m_pPixelShader;

	ID3D11DepthStencilView* m_pDepthStencilView;

	ID3D11Buffer* m_pConstantBuffer;
	ID3D11Buffer* m_pInterpolateBuffer;

	ID3D11SamplerState* m_pFieldSamplerState;
	ID3D11SamplerState* m_pTextureSamplerState;

	std::vector<ID3D11SamplerState*> samplers;

	size_t m_iInc;

public:
	struct CBuffer
	{
		DirectX::XMVECTORI32 secs; // x - scale factor
		DirectX::XMVECTORI32 stepsNum;
	};

	struct InterpolateBuffer
	{
		DirectX::XMVECTORF32 info;
	};

	static float constexpr expectedFrameTime = 1.0f / 1.0f;

public:
	AnimatedTexture(ID3D11Device* device, ID3D11DeviceContext* context, UINT width, UINT height);
	AnimatedTexture(ID3D11Device* device, ID3D11DeviceContext* context, std::string const& filename);
	~AnimatedTexture();

	HRESULT CreateAnimationTextureResources(std::string const& vertexShader, std::string const& pixelShader);

	void UpdateConstantBuffer(CBuffer const* buffer);
	void UpdateInterpolateBuffer(InterpolateBuffer const* buffer);

	ID3D11Buffer* const* GetInterpolateBufferAdress() const;

	ID3D11ShaderResourceView* GetBackgroundTextureSRV() const;
	ID3D11Texture2D* GetBackgroundTexture() const;

	ID3D11SamplerState* const* GetSamplerAdress() const;

	size_t GetLayersNum() const;

	std::vector<ID3D11ShaderResourceView*> GetLayersSourceTexturesSRV() const;
	std::vector<ID3D11ShaderResourceView*> GetLayersTargetTexturesSRV() const override;
	std::vector<ID3D11RenderTargetView*> GetLayersTexturesRTV() const;
	std::vector<ID3D11Texture2D*> GetLayersTextures() const;

	HRESULT CreateVectorFieldTexture(VectorField const* vectorField, FieldSwapper* swapper) const;

	std::vector<FieldSwapper*> GetFields() const override;

	void AddBackground(ID3D11Texture2D* texture, ID3D11ShaderResourceView* textureSRV);
	void AddLayer(ID3D11Texture2D* texture, ID3D11ShaderResourceView* textureSRV, ID3D11RenderTargetView* textureRTV);

	void SetUpFields(std::vector<FieldSwapper*> const& fields);

	HRESULT AddBackgroundByName(std::string const& filename);
	HRESULT AddLayerByName(std::string const& filename);

	void Swap();

	void SaveIncrement(size_t inc);
	void IncrementSaved();

	void IncrementStep(size_t incSize);

	void Render(ID3D11RasterizerState* pRasterizerState,
		ID3D11SamplerState* pSamplerState,
		ID3D11Buffer* pConstantBuffer);

private:
	void RenderTexture(size_t textureNum,
		ID3D11RasterizerState* pRasterizerState,
		ID3D11SamplerState* pSamplerState,
		ID3D11Buffer* pConstantBuffer,
		FieldSwapper* pFieldSwapper);

	ID3D11VertexShader* CreateVertexShader(LPCTSTR shaderSource, ID3DBlob** ppBlob);
	ID3D11PixelShader* CreatePixelShader(LPCTSTR shaderSource);
};
