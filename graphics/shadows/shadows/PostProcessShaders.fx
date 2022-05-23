Texture2D<float4> sourceTexture : register(t0);

SamplerState MinMagMipLinear : register(s0);

static const float EXPOSURE_RATIO = 7.0f;

cbuffer AverageLuminanceBuffer : register(b0)
{
    float AverageLuminance;
}

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

PS_INPUT vs_copy_main(uint input : SV_VERTEXID)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Tex = float2(input & 1, input >> 1);
    output.Pos = float4((output.Tex.x - 0.5f) * 2, -(output.Tex.y - 0.5f) * 2, 0, 1);
    return output;
}

float4 ps_copy_main(PS_INPUT input) : SV_TARGET
{
    return sourceTexture.Sample(MinMagMipLinear, input.Tex);
}

float4 ps_luminance_main(PS_INPUT input) : SV_TARGET
{
    float4 color = sourceTexture.Sample(MinMagMipLinear, input.Tex);
    float l = 0.2126f * color.r + 0.7151f * color.g + 0.0722f * color.b;
    return log(l + 1);
}

float Exposure()
{
    float luminance = AverageLuminance;
    float keyValue = 1.03 - 2 / (2 + log10(luminance + 1));
    return keyValue / luminance;
}

float3 Uncharted2Tonemap(float3 x)
{
    static const float a = 0.1;  // Shoulder Strength
    static const float b = 0.50; // Linear Strength
    static const float c = 0.1;  // Linear Angle
    static const float d = 0.20; // Toe Strength
    static const float e = 0.02; // Toe Numerator
    static const float f = 0.30; // Toe Denominator
                                 // Note: e/f = Toe Angle

    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

float3 TonemapFilmic(float3 color)
{
    static const float W = 11.2; // Linear White Point Value

    float e = Exposure() * EXPOSURE_RATIO;
    float3 curr = Uncharted2Tonemap(e * color);
    float3 whiteScale = 1.0f / Uncharted2Tonemap(W);
    return curr * whiteScale;
}

float3 LinearToSRGB(float3 color)
{
    return pow(abs(color), 1 / 2.2f);
}

float4 ps_tonemap_main(PS_INPUT input) : SV_TARGET
{
    float4 color = sourceTexture.Sample(MinMagMipLinear, input.Tex);
    return float4(LinearToSRGB(TonemapFilmic(color.xyz)), color.a);
}
