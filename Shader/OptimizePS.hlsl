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
    float3 Tint = lerp(Input.Color.rgb, ColorOverride, ColorOverrideAmount);
    return float4(Sampled.rgb * Tint, Sampled.a * Input.Color.a);
}
