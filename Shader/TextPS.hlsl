#include "Constants.hlsli"

Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

float4 MainPS(PS_INPUT Input) : SV_Target
{
    float4 Sampled = DiffuseTexture.Sample(DiffuseSampler, Input.UV);
       
    clip(Sampled.r - 0.1f);
    float3 TintedColor = Sampled.rgb * Input.Color.rgb;
    
    float3 FinalColor = lerp(Sampled.rgb, ColorOverride, ColorOverrideAmount);
    return float4(FinalColor, Sampled.a);
}
