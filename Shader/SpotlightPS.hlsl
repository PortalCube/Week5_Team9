#include "Constants.hlsli"

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 ClipNormal : NORMAL;
};

float4 MainPS(PS_INPUT Input) : SV_Target
{
    // UV.y: apex=0, base=1
    float T = saturate(Input.UV.y);

    // 거리 감쇠 - apex에서 밝고 base로 갈수록 어두워짐
    float DistanceFactor = 1.0f - T;
    float DistanceFalloff = pow(DistanceFactor, 2.0f);

    // 기본 색상
    float3 BaseColor = lerp(Input.Color.rgb, ColorOverride, ColorOverrideAmount);

    // 중심부 발광 (apex 근처)
    float CoreGlow = pow(DistanceFactor, 5.0f);
    float3 CoreColor = lerp(BaseColor, float3(1.0f, 1.0f, 1.0f), CoreGlow * 0.5f);

    // 빛의 세기
    float LightIntensity = DistanceFalloff;
    float GlowIntensity = CoreGlow * 0.8f;

    // 합성
    float3 FinalColor = CoreColor * LightIntensity;
    FinalColor += float3(1.0f, 1.0f, 1.0f) * GlowIntensity;

    // 최종 투명도
    float FinalAlpha = saturate(LightIntensity * 0.65f + GlowIntensity * 0.35f);

    return float4(FinalColor, FinalAlpha);
}

