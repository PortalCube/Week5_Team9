// 외곽선 후처리 픽셀 셰이더
Texture2D<float4> SceneTexture : register(t0);
// 2컴포넌트 포맷(X24_G8)에 맞게 uint2로 선언
Texture2D<uint2> StencilTexture : register(t1);

struct PS_IN
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

// 스텐실 값을 안전하게 읽는 함수 (x든 y든 1이 있으면 검출)
uint SampleStencil(int2 coord)
{
    uint2 s = StencilTexture.Load(int3(coord, 0));
    return max(s.x, s.y);
}

float4 MainPS(PS_IN input) : SV_Target
{
    const int2 pixelCoord = int2(input.Pos.xy);
    uint width, height;
    StencilTexture.GetDimensions(width, height);

    // 현재 픽셀의 스텐실 값 확인
    const uint currentStencil = SampleStencil(pixelCoord);
    const float4 sceneColor = SceneTexture.Load(int3(pixelCoord, 0));

    // 본체 영역은 원래 색상 유지
    if (currentStencil == 1)
    {
        return sceneColor;
    }

    // 8방향 주변 스텐실 검사
    const int thickness = 2;
    const int2 offsets[8] =
    {
        int2(thickness, 0), int2(-thickness, 0),
        int2(0, thickness), int2(0, -thickness),
        int2(thickness, thickness), int2(-thickness, thickness),
        int2(thickness, -thickness), int2(-thickness, -thickness)
    };

    uint neighborStencil = 0;
    for (int i = 0; i < 8; ++i)
    {
        const int2 neighbor = pixelCoord + offsets[i];
        if (neighbor.x >= 0 && neighbor.y >= 0 &&
            neighbor.x < int(width) && neighbor.y < int(height))
        {
            neighborStencil |= SampleStencil(neighbor);
        }
    }

    // 주변에 본체가 닿아 있으면 주황색 외곽선 출력
    if (neighborStencil > 0)
    {
        return float4(1.0f, 0.55f, 0.0f, 1.0f);
    }

    return sceneColor;
}
