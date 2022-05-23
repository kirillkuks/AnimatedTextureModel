TextureCube cubeTexture : register(t0);

SamplerState MinMagMipLinear : register(s0);

static const float PI = 3.14159265358979323846f;
static const int N1 = 200;
static const int N2 = 50;
static const uint PREFILTERED_COLOR_SAMPLE_COUNT = 1024u;
static const uint PREINTEGRATED_BRDF_SAMPLE_COUNT = 1024u;
static const uint RESOLUTION = 512u;

cbuffer Transformation : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 CameraPos;
}

cbuffer Material : register(b1)
{
    float4 Albedo;
    float Roughness;
    float Metalness;
}

struct VS_INPUT
{
    float3 Pos : POSITION;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Tex : POSITION;
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.Pos = float4(input.Pos, 1.0f);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Pos;
    return output;
}

float3 Irradiance(float3 normal)
{
    float3 dir = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(dir, normal));
    float3 bitangent = cross(normal, tangent);

    float3 irradiance = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < N1; i++)
    {
        for (int j = 0; j < N2; j++)
        {
            float phi = i * (2 * PI / N1);
            float theta = j * (PI / 2 / N2);
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sampleVec = tangentSample.x * tangent + tangentSample.y * bitangent + tangentSample.z * normal;
            irradiance += cubeTexture.SampleLevel(MinMagMipLinear, sampleVec, 0).xyz * cos(theta) * sin(theta);
        }
    }
    irradiance = PI * irradiance / (N1 * N2);
    return irradiance;
}

float4 ips_main(PS_INPUT input) : SV_TARGET
{
    return float4(Irradiance(normalize(input.Tex)), 1.0f);
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 norm, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.z = sin(phi) * sinTheta;
    H.y = cosTheta;

    float3 up = abs(norm.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, norm));
    float3 bitangent = cross(norm, tangent);
    float3 sampleVec = tangent * H.x + bitangent * H.z + norm * H.y;

    return normalize(sampleVec);
}

float DistributionGGX(float3 n, float3 h, float roughness)
{
    float roughnessSqr = pow(max(roughness, 0.01f), 2);
    return roughnessSqr / (PI * pow(pow(max(dot(n, h), 0), 2) * (roughnessSqr - 1) + 1, 2));
}

float3 PrefilteredColor(float3 norm)
{
    float3 view = norm;
    float totalWeight = 0.0;
    float3 prefilteredColor = float3(0, 0, 0);
    for (uint i = 0u; i < PREFILTERED_COLOR_SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, PREFILTERED_COLOR_SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, norm, Roughness);
        float3 L = normalize(2.0 * dot(view, H) * H - view);
        float ndotl = max(dot(norm, L), 0.0);
        float ndoth = max(dot(norm, H), 0.0);
        float hdotv = max(dot(H, view), 0.0);
        float D = DistributionGGX(norm, H, Roughness);
        float pdf = (D * ndoth / (4.0 * hdotv)) + 0.0001;
        float saTexel = 4.0 * PI / (6.0 * RESOLUTION * RESOLUTION);
        float saSample = 1.0 / (float(PREFILTERED_COLOR_SAMPLE_COUNT) * pdf + 0.0001);
        float mipLevel = Roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);
        if (ndotl > 0.0)
        {
            prefilteredColor += cubeTexture.SampleLevel(MinMagMipLinear, L, mipLevel).rgb * ndotl;
            totalWeight += ndotl;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;
    return prefilteredColor;
}

float4 cps_main(PS_INPUT input) : SV_TARGET
{
    return float4(PrefilteredColor(normalize(input.Tex)), 1.0f);
}

float SchlickGGX(float3 n, float3 v, float k)
{
    float value = max(dot(n, v), 0);
    return value / (value * (1 - k) + k);
}

float GeometrySmith(float3 n, float3 v, float3 l, float roughness)
{
    float k = pow(roughness, 2) / 2;
    return SchlickGGX(n, v, k) * SchlickGGX(n, l, k);
}

float2 IntegrateBRDF(float NdotV, float roughness)
{
    float3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.z = 0.0;
    V.y = NdotV;
    float A = 0.0;
    float B = 0.0;
    float3 N = float3(0.0, 1.0, 0.0);
    for (uint i = 0u; i < PREINTEGRATED_BRDF_SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, PREINTEGRATED_BRDF_SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);

        float3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(L.y, 0.0);
        float NdotH = max(H.y, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        if (NdotL > 0.0)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(PREINTEGRATED_BRDF_SAMPLE_COUNT);
    B /= float(PREINTEGRATED_BRDF_SAMPLE_COUNT);
    return float2(A, B);
}

float4 brdfps_main(PS_INPUT input) : SV_TARGET
{
    return float4(IntegrateBRDF(input.Tex.x, 1 - input.Tex.y), 0.0f, 1.0f);
}
