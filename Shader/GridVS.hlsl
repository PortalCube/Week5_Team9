cbuffer GridLineConstants : register(b2)
{
    row_major float4x4 MVP;
    float3 CameraPosition;
    float FadeStartDistance;
    float FadeEndDistance;
    float3 Padding;
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL;
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float3 WorldPosition : TEXCOORD0;
};

PS_INPUT MainVS(VS_INPUT Input)
{
    PS_INPUT Output;
    Output.Position = mul(float4(Input.Position, 1.0f), MVP);
    Output.Color = Input.Color;
    Output.WorldPosition = Input.Position;
    return Output;
}
