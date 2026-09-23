cbuffer FrameConstants : register(b0)
{
    float Time;
    float DeltaTime;
    float2 FramePadding;
}

cbuffer ViewConstants : register(b1)
{
    row_major float4x4 VP;
    float2 ViewportSize;
    float2 ViewPadding;
}

cbuffer ObjectConstants : register(b2)
{
    row_major float4x4 MVP;
    float3 ColorOverride;
    float ColorOverrideAmount;
    float2 UVScale;
    float2 UVOffset;
    row_major float4x4 World;
    float DisableShading;
    float3 ObjectPadding;
}

cbuffer LightConstants : register(b4)
{
    float3 LightDirection;
    float Intensity;
    float3 LightColor;
    float AmbientIntensity;
};
