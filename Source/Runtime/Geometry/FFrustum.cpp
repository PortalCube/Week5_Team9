#include "FFrustum.h"

namespace
{
    // 엔진 VP → D3D 클립 순서 VP (FRenderer::UpdateBuffer와 같은 변환)
    FMatrix ToD3DClip(const FMatrix& ViewProj)
    {
        FMatrix Copy = ViewProj;
        return Copy.ToD3DMatrix();
    }

    // 행벡터 규약: Clip = (x, y, z, 1) * M 이므로 클립 성분 j = P · col_j
    // col_j = (M[0][j], M[1][j], M[2][j], M[3][j])
    // Out[p] = (a, b, c, d), 정규화 전
    void ExtractRawPlanes(const FMatrix& ClipVP, float Out[FFrustum::PlaneCount][4])
    {
        for (int32 r = 0; r < 4; ++r)
        {
            const float C0 = ClipVP.M[r][0];
            const float C1 = ClipVP.M[r][1];
            const float C2 = ClipVP.M[r][2];
            const float C3 = ClipVP.M[r][3];

            Out[FFrustum::Left][r] = C3 + C0;   // -w <= x
            Out[FFrustum::Right][r] = C3 - C0;   //  x <= w
            Out[FFrustum::Bottom][r] = C3 + C1;   // -w <= y
            Out[FFrustum::Top][r] = C3 - C1;   //  y <= w
            Out[FFrustum::Near][r] = C2;        //  0 <= z (D3D 깊이 0~1)
            Out[FFrustum::Far][r] = C3 - C2;   //  z <= w
        }
    }
}

FFrustum FFrustum::FromViewProjection(const FMatrix& ViewProj)
{
    float Raw[PlaneCount][4];
    ExtractRawPlanes(ToD3DClip(ViewProj), Raw);

    FFrustum Result;
    for (int32 p = 0; p < PlaneCount; p++)
    {
        const float A = Raw[p][0];
        const float B = Raw[p][1];
        const float C = Raw[p][2];
        const float Length = std::sqrt(A * A + B * B + C * C);

        const float InvLength = (Length > 1e-10f) ? 1.f / Length : 0.f;

        Result.Planes[p].Normal = FVector{ A * InvLength, B * InvLength, C * InvLength };
        Result.Planes[p].Dist = Raw[p][3] * InvLength;
    }

    return Result;
}
