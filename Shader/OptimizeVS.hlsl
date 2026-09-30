#include "Constants.hlsli"

struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
};

PS_INPUT MainVS(VS_INPUT Input)
{
    PS_INPUT Output;
    Output.Position = mul(float4(Input.Position, 1.0f), mul(World, mul(View, Projection)));
    Output.Color = Input.Color;
    Output.UV = Input.UV * UVScale + UVOffset;
    return Output;
}
