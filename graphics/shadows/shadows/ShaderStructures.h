#pragma once

#define NUM_LIGHTS 1

struct WorldViewProjectionConstantBuffer
{
	DirectX::XMMATRIX World;
	DirectX::XMMATRIX View;
	DirectX::XMMATRIX Projection;
	DirectX::XMFLOAT4 CameraPos;
	DirectX::XMFLOAT4 CameraDir;
};

struct VertexData
{
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 Tex;
};

struct VertexPosData
{
	DirectX::XMFLOAT3 Pos;
};

struct LightConstantBuffer
{
	DirectX::XMFLOAT4 LightPosition[NUM_LIGHTS];
	DirectX::XMFLOAT4 LightColor[NUM_LIGHTS];
	DirectX::XMFLOAT4 LightAttenuation[NUM_LIGHTS];
};

__declspec(align(16))
struct LuminanceConstantBuffer
{
	float AverageLuminance;
};

__declspec(align(16))
struct MaterialConstantBuffer
{
	DirectX::XMFLOAT4 Albedo;
	float Roughness;
	float Metalness;
};

__declspec(align(16))
struct BlurConstantBuffer
{
	DirectX::XMUINT2 ImageSize;
};

struct ShadowConstantBuffer
{
	DirectX::XMMATRIX SimpleShadowTransform;
	DirectX::XMMATRIX PSSMTransform[4];
	DirectX::XMFLOAT4 PSSMBorders;
	BOOL UseShadowPCF;
	BOOL UseShadowPSSM;
	BOOL ShowPSSMSplits;
};
