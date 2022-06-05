#define NUM_LIGHTS 1

TextureCube irradianceTexture : register(t0);
TextureCube prefilteredColorTexture : register(t1);
Texture2D<float4> preintegratedBRDFTexture : register(t2);

Texture2D<float4> diffuseTexture : register(t3);
Texture2D<float4> metallicRoughnessTexture : register(t4);
Texture2D<float4> normalTexture : register(t5);

Texture2D simpleShadowMapTexture : register(t6);
Texture2DArray PSSMTexture : register(t7);

Texture2D<float4> layer1Texture : register(t8);
Texture2D<float4> field1Texture : register(t9);
Texture2D<float4> layer2Texture : register(t10);
Texture2D<float4> field2Texture : register(t11);

SamplerState MinMagMipLinear : register(s0);
SamplerState MinMagLinearMipPointClamp : register(s1);
SamplerState ModelSampler : register(s2);
SamplerState MinMagMipLinearBorder : register(s3);
SamplerComparisonState MinMagMipLinearBorderLess : register(s4);

SamplerState FieldSampler : register(s5);
SamplerState TextureSampler : register(s6);

static const float PI = 3.14159265358979323846f;
static const float MAX_REFLECTION_LOD = 4.0f;

cbuffer Transformation: register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
	float4 CameraPos;
    float4 CameraDir;
}

cbuffer Lights : register(b1)
{
    float4 LightPositions[NUM_LIGHTS];
    float4 LightColors[NUM_LIGHTS];
    float4 LightAttenuations[NUM_LIGHTS];
}

cbuffer Material : register(b2)
{
    float4 Albedo;
	float Roughness;
	float Metalness;
}

cbuffer Shadows : register(b3)
{
    matrix SimpleShadowTransform;
    matrix PSSMTransform[4];
    float4 PSSMBorders;
    bool UseShadowPCF;
    bool UseShadowPSSM;
    bool ShowPSSMSplits;
}

cbuffer AnimatedStuff : register(b4)
{
    float4 AnimatedTextureInfo; // x - scale factor, y - width
}


struct VS_INPUT
{
    float3 Normal : NORMAL;
    float3 Pos : POSITION;
#ifdef HAS_TANGENT
    float4 Tangent : TANGENT;
#endif
    float2 Tex : TEXCOORD_0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD_0;
    float3 Normal : NORMAL;
	float4 WorldPos : POSITION;
#ifdef HAS_TANGENT
    float3 Tangent : TANGENT;
#endif
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(float4(input.Pos, 1.0f), World);
	output.WorldPos = output.Pos;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Tex = input.Tex;
    output.Normal = normalize(mul(input.Normal, (float3x3)World));
#ifdef HAS_TANGENT
    output.Tangent = input.Tangent.xyz;
    if (length(input.Tangent.xyz) > 0)
        output.Tangent = normalize(mul(input.Tangent.xyz, (float3x3)World));
#endif
    return output;
}

float3 h(float3 v, float3 l) 
{
    return normalize(v + l);
}

float normalDistribution(float3 n, float3 v, float3 l, float roughness)
{
    float roughnessSqr = pow(max(roughness, 0.01f), 2);
	return roughnessSqr / (PI * pow(pow(max(dot(n, h(v, l)), 0), 2) * (roughnessSqr - 1) + 1, 2));
}

float4 ndps_main(PS_INPUT input) : SV_TARGET
{
    return normalDistribution(normalize(input.Normal), normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0].xyz), Roughness);
}

float SchlickGGX(float3 n, float3 v, float k)
{
	float value = max(dot(n, v), 0);
	return value / (value * (1 - k) + k);
}

float geometry(float3 n, float3 v, float3 l, float roughness)
{
	float k = pow(roughness + 1, 2) / 8;
	return SchlickGGX(n, v, k) * SchlickGGX(n, l, k);
}

float4 gps_main(PS_INPUT input) : SV_TARGET
{
    return geometry(normalize(input.Normal), normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0].xyz), Roughness);
}

float3 fresnel(float3 n, float3 v, float3 l, float3 albedo, float metalness)
{
	float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);
    return (F0 + (1 - F0) * pow(1 - max(dot(h(v, l), v), 0), 5)) * sign(max(dot(l, n), 0));
}

float4 fps_main(PS_INPUT input) : SV_TARGET
{
    return float4(fresnel(normalize(input.Normal), normalize(CameraPos.xyz - input.WorldPos.xyz), normalize(LightPositions[0].xyz), Albedo.xyz, Metalness), 1.0f);
}

float3 BRDF(float3 p, float3 n, float3 v, float3 l, float3 albedo, float metalness, float roughness)
{
	float D = normalDistribution(n, v, l, roughness);
	float G = geometry(n, v, l, roughness);
	float3 F = fresnel(n, v, l, albedo, metalness);

    return (1 - F) * albedo / PI * (1 - metalness) + D * F * G / (0.001f + 4 * (max(dot(l, n), 0) * max(dot(v, n), 0)));
}

float Attenuation(float3 lightDir, float2 attenuation)
{
	float d = length(lightDir);
    float factor = attenuation.x + attenuation.y * d * d;
    return 1 / max(factor, 1e-9);
}

float GetShadowFactor(matrix transformation, float3 pos)
{
    float4 proj = mul(float4(pos, 1.0f), transformation);
    if (UseShadowPCF)
        return simpleShadowMapTexture.SampleCmpLevelZero(MinMagMipLinearBorderLess, proj.xy, proj.z).r;
    else
        return (simpleShadowMapTexture.Sample(MinMagMipLinearBorder, proj.xy).r > proj.z) ? 1 : 0;
}

float GetShadowFactorArray(matrix transformation, float3 pos, int idx)
{
    float4 proj = mul(float4(pos, 1.0f), transformation);
    if (UseShadowPCF)
        return PSSMTexture.SampleCmpLevelZero(MinMagMipLinearBorderLess, float3(proj.xy, idx), proj.z).r;
    else
        return (PSSMTexture.Sample(MinMagMipLinearBorder, float3(proj.xy, idx)).r > proj.z) ? 1 : 0;
}

float GetShadowPSSMFactor(float3 pos)
{
    float dist = dot(pos - CameraPos.xyz, CameraDir.xyz);
    if (dist < PSSMBorders.x)
        return GetShadowFactorArray(PSSMTransform[0], pos, 0);
    else if (dist < PSSMBorders.y)
        return GetShadowFactorArray(PSSMTransform[1], pos, 1);
    else if (dist < PSSMBorders.z)
        return GetShadowFactorArray(PSSMTransform[2], pos, 2);
    else if (dist < PSSMBorders.w)
        return GetShadowFactorArray(PSSMTransform[3], pos, 3);
    else
        return 1.0f;
}

float3 LO_i(float3 p, float3 n, float3 v, uint lightIndex, float3 pos, float3 albedo, float metalness, float roughness)
{
    float3 lightDir = LightPositions[lightIndex].xyz;
    float4 lightColor = LightColors[lightIndex];
    float atten = Attenuation(lightDir, LightAttenuations[lightIndex].xy);
	float3 l = normalize(lightDir);
    float shadowFactor = 1;
    if (UseShadowPSSM)
        shadowFactor = GetShadowPSSMFactor(pos);
    else
        shadowFactor = GetShadowFactor(SimpleShadowTransform, pos);
    return BRDF(p, n, v, l, albedo, metalness, roughness) * lightColor.rgb * atten * max(dot(l, n), 0) * lightColor.a * shadowFactor;
}

float3 FresnelSchlickRoughnessFunction(float3 F0, float3 n, float3 v, float roughness)
{
    return F0 + (max(1 - roughness, F0) - F0) * pow(1 - max(dot(n, v), 0), 5);
}

float3 Ambient(float3 n, float3 v, float3 albedo, float metalness, float roughness)
{
    float3 r = normalize(reflect(-v, n));
    float3 prefilteredColor = prefilteredColorTexture.SampleLevel(MinMagMipLinear, r, roughness * MAX_REFLECTION_LOD).rgb;
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metalness);
    float2 envBRDF = preintegratedBRDFTexture.Sample(MinMagLinearMipPointClamp, float2(dot(n, v), roughness)).xy;
    float3 specular = prefilteredColor * (F0 * envBRDF.x + envBRDF.y);

    float3 irradiance = irradianceTexture.Sample(MinMagMipLinear, n).rgb;
    float3 F = FresnelSchlickRoughnessFunction(F0, n, v, roughness);
    return (1 - F) * irradiance * albedo * (1 - metalness) + specular;
}

float4 GetAlbedo(float2 uv)
{
    float4 albedo = Albedo;
#ifdef HAS_COLOR_TEXTURE
    albedo *= diffuseTexture.Sample(ModelSampler, uv);
#else
    albedo = pow(albedo, 2.2f);
#endif
#ifdef HAS_OCCLUSION_TEXTURE
    albedo.xyz *= metallicRoughnessTexture.Sample(ModelSampler, uv).r;
#endif
    return albedo;
}

float2 GetAnimatedTextureCoords(in float2 uv)
{
    float dx = field1Texture.Sample(FieldSampler, uv).r;
    float dy = field1Texture.Sample(FieldSampler, uv).g;

    float2 dstPixCoords = uv;
    float2 srcPixCoords = dstPixCoords + float2(dx, dy);

    float2 t = AnimatedTextureInfo.x;

    float2 newCoords = uv + t * float2(dx, dy);

    float transformdx = field1Texture.Sample(FieldSampler, uv).b;
    float transformdy = field1Texture.Sample(FieldSampler, uv).a;

    if (transformdx * transformdx + transformdy * transformdy > 0)
    {
        newCoords = uv + float2(dx, dy)
            + (1 - t) * float2(transformdx, transformdy);
    }

    return newCoords;
}

float4 GetAnimatedTextureAlbedo(float2 uv, unsigned int ind)
{
    Texture2D textures[] = { layer1Texture, layer2Texture };
    float4 albedo = Albedo;

    float2 animatedCoords = GetAnimatedTextureCoords(uv);

#ifdef HAS_COLOR_TEXTURE
    albedo *= textures[ind].Sample(TextureSampler, animatedCoords);
#else
    albedo = pow(albedo, 2.2f);
#endif
#ifdef HAS_OCCLUSSION_TEXTURE
    albedo.xyz *= metallicRoughnessTexture.Sample(ModelSampler, animatedCoords).r;
#endif
    return albedo;
}

float4 GetAlbedoByPixel(float2 uv)
{
    float2 animatedCoords = GetAnimatedTextureCoords(uv);

    float4 pix = layer1Texture.Sample(TextureSampler, animatedCoords);

    // Color interpolation
    /* {
        float3 srcColor = layer1Texture.Sample(ModelSampler, input.Tex + float2(dx, dy)).rgb;
        float3 dstColor = layer1Texture.Sample(ModelSampler, input.Tex).rgb;
        pix.r = srcColor.r * t + dstColor.r * (1 - t);
        pix.g = srcColor.g * t + dstColor.g * (1 - t);
        pix.b = srcColor.b * t + dstColor.b * (1 - t);
    } */

    float4 albedo = Albedo;
#ifdef HAS_COLOR_TEXTURE
    albedo *= pix;
#else
    albedo = pow(albedo, 2.2f);
#endif
#ifdef HAS_OCCLUSSION_TEXTURE
    albedo.xyz *= metallicRoughnessTexture.Sample(ModelSampler, animatedCoords).r;
#endif
    return albedo;
}

float2 GetMetalnessRoughness(float2 uv)
{
    float2 material = float2(Metalness, Roughness);
#ifdef HAS_METAL_ROUGH_TEXTURE
    material *= metallicRoughnessTexture.Sample(ModelSampler, uv).bg;
#endif
    return material.xy;
}

float3 GetPSSMSplitColor(float3 pos)
{
    float dist = dot(pos - CameraPos.xyz, CameraDir.xyz);
    if (dist < PSSMBorders.x)
        return float3(0, 0, 1);
    else if (dist < PSSMBorders.y)
        return float3(0, 1, 0);
    else if (dist < PSSMBorders.z)
        return float3(1, 1, 1);
    else if (dist < PSSMBorders.w)
        return float3(1, 0, 0);
    else
        return float3(0.5f, 0.5f, 0.5f);
}

float4 ps_main(PS_INPUT input) : SV_TARGET
{
    float3 color1, color2, color3;
    float3 v = normalize(CameraPos.xyz - input.WorldPos.xyz);
    float3 n = normalize(input.Normal);

#ifdef HAS_EMISSIVE
#ifdef HAS_ANIMATED_TEXTURE
    {
        float4 emissive = GetAnimatedTextureAlbedo(input.Tex, 0);
        emissive.xyz *= 5.0;   // AAV Small hack - make it shine! =)
        return emissive;
    }
#else
    return diffuseTexture.Sample(ModelSampler, input.Tex);
#endif // !HAS_ANIMATED_TEXTURE
#else
#ifdef HAS_NORMAL_TEXTURE
    float3 nm = (normalTexture.Sample(ModelSampler, input.Tex) * 2.0f - 1.0f).xyz;
    float3 tangent = input.Tangent;
    if (length(input.Tangent) > 0)
        tangent = normalize(input.Tangent);
    float3 binormal = cross(n, tangent);
    n = normalize(nm.x * tangent + nm.y * binormal + n);
#endif

    float2 material = GetMetalnessRoughness(input.Tex);
    float metalness = material.x;
    float roughness = material.y;
    float4 albedo = GetAlbedo(input.Tex);

    float3 additionalL = float3(0,0,0);
#ifdef HAS_ANIMATED_TEXTURE
    {
        // Try additional Luminance, instead of albedo color
        additionalL = GetAnimatedTextureAlbedo(input.Tex, 0).xyz * 3.0;    // Coords interpolation

        //albedo = GetAnimatedTextureAlbedo(input.Tex, 0);    // Coords interpolation

        // albedo = GetAlbedoByPixel(pix, input.Tex);    // Color interpolation
    }
#endif // HAS_ANIMATED_TEXTURE

    float3 color = 0;
    [unroll]
    for (uint i = 0; i < NUM_LIGHTS; ++i)
        color += LO_i(input.WorldPos.xyz, n, v, i, input.WorldPos.xyz, albedo.rgb, metalness, roughness);
    color += additionalL;

    float3 ambient = Ambient(n, v, albedo.rgb, metalness, roughness);

    float4 result = float4(color + ambient, albedo.a);
    
    if (ShowPSSMSplits)
    {
        float3 splitColor = GetPSSMSplitColor(input.WorldPos.xyz);
        result.xyz = result.xyz * 0.9f + splitColor * 0.1f;
    }

    return result;
#endif
}
