#include "Constants.hlsli"

Texture2D msdTexture : register(t0);
SamplerState msdSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

// 새 채널의 중간값을 구하는 함수
float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float4 MainPS(PS_INPUT Input) : SV_Target
{
    float3 msd = msdTexture.Sample(msdSampler, Input.UV).rgb;
    
    float sd = median(msd.r, msd.g, msd.b) - 0.5f;
 
    float screenPxDistance = sd / fwidth(sd);
    float opacity = saturate(screenPxDistance + 0.5f);
    
    clip(opacity - 0.1f);
    
    float3 baseColor = float3(1.0f, 1.0f, 1.0f);
    float3 finalColor = lerp(baseColor, ColorOverride, ColorOverrideAmount);
    
    return float4(finalColor, Input.Color.a * opacity);
    //return float4(1.0f, 0.0f, 0.0f, 1.0f); // 강제 빨간색 출력
}
    
   
