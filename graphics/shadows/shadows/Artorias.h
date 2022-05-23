#pragma once

#include <d3d11_1.h>
#include <DirectXMath.h>

#include "Model.h"
#include "AnimatedTexture.h"



class Artorias : public Model
{
private:
	std::shared_ptr<AnimatedTexture> m_pAnimatedTexture;
	std::vector<UINT> m_animatedPrimitives;

public:
	struct ShaderSlots : public Model::ShadersSlots
	{
		UINT layersStartSlot;
	};


public:
	Artorias(const char* modelPath,
		const std::shared_ptr<ModelShaders>& modelShaders,
		DirectX::XMMATRIX globalWorldMatrix = DirectX::XMMatrixIdentity());
	~Artorias();

	void SetAnimatedTexture(std::shared_ptr<AnimatedTexture>& animatedTexture, UINT primitiveNum);

	void Render(ID3D11DeviceContext* context,
		WorldViewProjectionConstantBuffer transformationData,
		ID3D11Buffer* transformationConstantBuffer,
		ID3D11Buffer* materialConstantBuffer,
		ShadersSlots slots,
		bool emissive = false, bool usePS = true) override;

private:
	// TODO
	// ѕереопределить только анимированный примитив?
	// Ўейдер - интерпол€ци€

	void RenderPrimitive(Model::Primitive& primitive,
		ID3D11DeviceContext* context,
		WorldViewProjectionConstantBuffer& transformationData,
		ID3D11Buffer* transformationConstantBuffer,
		ID3D11Buffer* materialConstantBuffer,
		ShadersSlots& slots,
		bool emissive = false,
		bool usePS = true,
		bool isAnimated = false);

	HRESULT AddEmission(UINT primitiveNum);

};
