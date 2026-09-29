#include "Constants.hlsli"

struct VS_INPUT
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

PS_INPUT MainVS(VS_INPUT Input)
{
    PS_INPUT Output;
    Output.Position = mul(float4(Input.Position, 1.0f), mul(World, mul(View, Projection)));
    Output.UV = Input.UV * UVScale + UVOffset;
    return Output;
}
