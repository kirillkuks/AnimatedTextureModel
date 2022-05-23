cbuffer ConstantBuffer : register(b0)
{
	int4 secs; // x - scale factor
	int4 framesPerTex;
}

texture2D Texture : register(t0);

texture2D VectorField1 : register(t1);
texture2D VectorField2 : register(t2);

SamplerState Sampler : register(s0);

struct VSInput
{
	float4 pos : POSITION;
	float2 uv : TEXCOORD;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD;
};

VSOutput VS(in VSInput vertex)
{
	VSOutput output;

	output.pos = vertex.pos;
	output.uv = vertex.uv;

	return output;
}

float4 PS(in VSOutput input) : SV_Target0
{
	float4 color = float4(0, 0, 0, 1);
	
	// float ddx = VectorField1.Sample(Sampler, input.uv).r;
	// float ddy = VectorField1.Sample(Sampler, input.uv).g;

	float d = 1 / 2048.0;
	int scale = secs.x;
	// int curIndex = secs.y;
	// int curFrameCounter = secs.z;
	// int totalFieldNum = secs.w;

	float2 vec = input.uv;

	float ddx = VectorField1.Sample(Sampler, vec).r;
	float ddy = VectorField1.Sample(Sampler, vec).g;

	/*for (int i = 0; i < scale; ++i)
	{
		float ddx = VectorField1.Sample(Sampler, vec).r;
		float ddy = VectorField1.Sample(Sampler, vec).g;

		if (curIndex == 0)
		{
			ddx = VectorField1.Sample(Sampler, vec).r;
			ddy = VectorField1.Sample(Sampler, vec).g;
		}
		else
		{
			ddx = VectorField2.Sample(Sampler, vec).r;
			ddy = VectorField2.Sample(Sampler, vec).g;
		}

		vec = vec + float2(ddy, ddx);

		++curFrameCounter;
		if (curFrameCounter >= framesPerTex.y)
		{
			curFrameCounter = 0;
			curIndex = (curIndex + 1) % totalFieldNum;
		}
	}*/

	color.rgb = Texture.Sample(Sampler, vec + scale * float2(ddx, ddy)).rgb;

	/*float3 I_i_j = Texture.Sample(Sampler, vec).rgb;
	
	float3 I_i1_j = Texture.Sample(Sampler, vec + float2(d, 0)).rgb;
	float3 I_1i_j = Texture.Sample(Sampler, vec + float2(-d, 0)).rgb;

	float3 I_i_j1 = Texture.Sample(Sampler, vec + float2(0, d)).rgb;
	float3 I_i_1j = Texture.Sample(Sampler, vec + float2(0, -d)).rgb;

	color.rgb = I_i_j + scale * (ddy * (I_i1_j - I_1i_j) + ddx * (I_i_j1 - I_i_1j));*/

	// color.rgb = Texture.Sample(Sampler, input.uv + scale * float2(VectorField1.Sample(Sampler, input.uv).g, VectorField1.Sample(Sampler, input.uv).r));

	/*color.rgb = (Texture.Sample(Sampler, input.uv) +
		( ddx * 2048.0 * (Texture.Sample(Sampler, input.uv + float2(0, d)) - Texture.Sample(Sampler, input.uv + float2(0, -d))) ) +
		( ddy * 2048.0 * (Texture.Sample(Sampler, input.uv + float2(d, 0)) - Texture.Sample(Sampler, input.uv + float2(-d, 0))) )).rgb;*/



	/*color.rgb = Texture.Sample(Sampler, input.uv) -
		(ddy / 4.0 * (-Texture.Sample(Sampler, input.uv + float2(2 * d, 0)) + 4 * Texture.Sample(Sampler, input.uv + float2(d, 0)) - 4 * Texture.Sample(Sampler, input.uv + float2(-d, 0)) + Texture.Sample(Sampler, input.uv + float2(-2 * d, 0)))) +
		(ddy / 12.0 * (Texture.Sample(Sampler, input.uv + float2(2 * d, 0)) - 4 * Texture.Sample(Sampler, input.uv + float2(d, 0)) + 6 * Texture.Sample(Sampler, input.uv) - 4 * Texture.Sample(Sampler, input.uv + float2(-d, 0)) + Texture.Sample(Sampler, +float2(-2 * d, 0)))) +
		(ddx / 4.0 * (-Texture.Sample(Sampler, input.uv + float2(0, 2 * d)) + 4 * Texture.Sample(Sampler, input.uv + float2(0, d)) - 4 * Texture.Sample(Sampler, input.uv + float2(0, -d)) + Texture.Sample(Sampler, input.uv + float2(0, -2 * d)))) +
		(ddx / 12.0 * (Texture.Sample(Sampler, input.uv + float2(0, 2 * d)) - 4 * Texture.Sample(Sampler, input.uv + float2(0, d)) + 6 * Texture.Sample(Sampler, input.uv) - 4 * Texture.Sample(Sampler, input.uv + float2(0, -d)) + Texture.Sample(Sampler, +float2(0, -2 * d))));*/

	return color;
}
