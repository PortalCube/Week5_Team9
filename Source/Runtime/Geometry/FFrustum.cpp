#include "FFrustum.h"

namespace
{
    FVector4 GetColumn(const FMatrix& Matrix, int32 Col)
    {
        return FVector4{ Matrix.M[0][Col], Matrix.M[1][Col], Matrix.M[2][Col], Matrix.M[3][Col] };
    }

    // 평면 계수 (a, b, c, d) → 법선 길이 1인 평면
    //   a*x + b*y + c*z + d ≥ 0 에서 (a, b, c)가 법선, d가 평면 위치
    //   법선 길이로 나눠야 "n·P + d"가 실제 거리가 된다
    FPlane MakeNormalizedPlane(const FVector4& Coeff)
    {
        //
        const FVector Normal{ Coeff.X, Coeff.Y, Coeff.Z };
        const float Length = Normal.Size();

        // 퇴화 평면: (0,0,0,0)으로 두면 어떤 점도 바깥으로 판정되지 않는다 (보수적)
        if (Length < 1e-10f)
        {
            return FPlane{};
        }

        const float InvLength = 1.0f / Length;
        //Normal과 D를 Length로 Normalize
        return FPlane{ Normal * InvLength, Coeff.W * InvLength };
    }
}

FFrustum FFrustum::FromViewProjection(const FMatrix& ViewProj)
{
    FMatrix ClipVP = ViewProj;
    ClipVP = ClipVP.ToD3DMatrix();

    //Clip Pos = P dot VP
    //(x_c, y_c, z_c, w) = (x, y, z, 1)[VP]
    //x_c = P dot VP.col0
    //y_c = P dot VP.col1
    //z_c = P dot VP.col2
    //w = P dot VP.col3

    // -w <= x_c <= w   Left, Right
    // -w <= y_c <= w   Bottom, Top
    // 0 <= z_c <= w    Near, Far (D3D 깊이 0~1)

    //col 0 ~ 3 각 열 성분
    FVector4 Col0 = GetColumn(ClipVP, 0);
    FVector4 Col1 = GetColumn(ClipVP, 1);
    FVector4 Col2 = GetColumn(ClipVP, 2);
    FVector4 Col3 = GetColumn(ClipVP, 3);

	//Left      col3 + col0    -w <= x      w + x >= 0
	//Right     col3 - col0     x <= w      w - x >= 0
	//Bottom    col3 + col1    -w <= y      w + y >= 0
	//Top       col3 - col1     y <= w      w - y >= 0
	//Near      col2            0 <= z      z >= 0
	//Far       col3 - col2     z <= w      w - z >= 0

    // 3) 클립 공간 부등식을 "계수 · P ≥ 0" 형태로 옮긴다
    FFrustum Result;
    Result.Planes[Left] = MakeNormalizedPlane(Col3 + Col0);
    Result.Planes[Right] = MakeNormalizedPlane(Col3 - Col0);
    Result.Planes[Bottom] = MakeNormalizedPlane(Col3 + Col1);
    Result.Planes[Top] = MakeNormalizedPlane(Col3 - Col1);
    Result.Planes[Near] = MakeNormalizedPlane(Col2);
    Result.Planes[Far] = MakeNormalizedPlane(Col3 - Col2);

    return Result;
}
