Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

float4 MainPS(PS_INPUT Input) : SV_Target
{
    return DiffuseTexture.Sample(DiffuseSampler, Input.UV);
}
