Texture2D<float4> srcTexture0 : register(t0);
Texture2D<float4> srcTexture1 : register(t1);
RWTexture2D<float4> dstTexture : register(u0);

cbuffer ImageData : register(b0)
{
    int2 ImageSize;
}

static const int BlurSize = 7;
static const int HalfBlurSize = (BlurSize - 1) / 2;

[numthreads(64, 1, 1)]
void bloomcs_main(uint3 groupId : SV_GroupID, uint3 threadId : SV_GroupThreadID)
{
    float4 curSum = float4(0, 0, 0, 0);
    for (int i = -HalfBlurSize; i <= HalfBlurSize; i++)
    {
        int2 pixCoord = clamp(int2(groupId.x * 64 + i, groupId.y * 64 + threadId.x), int2(0, 0), ImageSize);
        curSum += srcTexture0[pixCoord];
    }

    for (int i = 0; i < 64; i++)
    {
        int2 pixCoord = int2(groupId.x * 64 + i, groupId.y * 64 + threadId.x);
        dstTexture[pixCoord] = curSum / BlurSize;
        int2 prevPixCoord = max(int2(groupId.x * 64 + i - HalfBlurSize, groupId.y * 64 + threadId.x), int2(0, 0));
        int2 nextPixCoord = min(int2(groupId.x * 64 + i + HalfBlurSize + 1, groupId.y * 64 + threadId.x), ImageSize);
        curSum = curSum - srcTexture0[prevPixCoord] + srcTexture0[nextPixCoord];
    }
}

[numthreads(64, 1, 1)]
void addcs_main(uint3 threadId : SV_DispatchThreadID)
{
    dstTexture[threadId.xy] = srcTexture0[threadId.xy] + srcTexture1[threadId.xy];
}
