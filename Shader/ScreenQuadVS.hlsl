// 화면 전체를 덮는 삼각형 버텍스 셰이더
struct VS_OUT
{
    float4 Pos : SV_POSITION;
    float2 UV  : TEXCOORD0;
};

VS_OUT MainVS(uint id : SV_VertexID)
{
    VS_OUT output;
    output.UV = float2((id << 1) & 2, id & 2);
    output.Pos = float4(output.UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}
