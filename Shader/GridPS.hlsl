cbuffer GridLineConstants : register(b2)
{
    row_major float4x4 MVP;
    float3 CameraPosition;
    float FadeStartDistance;
    float FadeEndDistance;
    float3 Padding;
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float3 WorldPosition : TEXCOORD0;
};

float4 MainPS(PS_INPUT Input) : SV_Target
{
    const float DistanceToCamera = distance(Input.WorldPosition, CameraPosition);
    const float Fade = 1.0f - smoothstep(
        FadeStartDistance, FadeEndDistance, DistanceToCamera);
    
    return float4(Input.Color.rgb, Input.Color.a * Fade);
}
