#include "Constants.hlsli"

Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL; // 법선
};

float4 MainPS(PS_INPUT Input) : SV_Target
{
    float4 Sampled = DiffuseTexture.Sample(DiffuseSampler, Input.UV);
    clip(Sampled.a - 0.1f);
    
    // 하이라이트 색상 보간
    float3 Tint = lerp(float3(1.0f, 1.0f, 1.0f), ColorOverride, ColorOverrideAmount);
    float3 BaseColor = Sampled.rgb * Tint;

    if (DisableShading > 0.5f)
    {
        return float4(BaseColor, Sampled.a);
    }
    
    // 조명 계산 및 양면 음영 보정
    float3 N = normalize(Input.Normal);
    float NdotL = max(0.0f, dot(N, -normalize(LightDirection)));
    float3 Diffuse = LightColor * (Intensity * NdotL);
    float3 Ambient = LightColor * max(AmbientIntensity, 0.4f);
    float3 DirectionalLight = max(Ambient + Diffuse, 0.5f);

    float3 FinalColor = BaseColor * DirectionalLight;
    return float4(FinalColor, Sampled.a);
}
