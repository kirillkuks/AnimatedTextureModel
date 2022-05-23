TextureCube cubeTexture : register(t0);

SamplerState MinMagMipLinear : register(s0);

cbuffer Transformation: register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
	float4 CameraPos;
}

struct VS_INPUT
{
    float3 Normal : NORMAL;
    float3 Pos : POSITION;
    float2 Tex : TEXCOORD_0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Tex : TEXCOORD_0;
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(float4(input.Pos, 1.0f), World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Pos;
    return output;
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    return cubeTexture.Sample(MinMagMipLinear, input.Tex);
}
