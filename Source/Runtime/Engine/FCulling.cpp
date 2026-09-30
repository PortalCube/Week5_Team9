#include "FCulling.h"

/*namespace FrustumUtils
{
	// 법선의 각 성분을 절댓값으로. r = |n|·e 계산용
	FVector AbsVector(const FVector& V)
	{
		return FVector{ std::fabs(V.X), std::fabs(V.Y), std::fabs(V.Z) };
	}

	// 상자가 평면 하나의 완전히 바깥인가
	//   CenterDist : 상자 중심의 부호 있는 거리 = n·c + D
	//   Radius     : 중심에서 상자 안의 점으로 이동할 때 n·P가 늘어날 수 있는 최대량 = |n|·e
	//   상자에서 가장 안쪽인 점의 거리 = CenterDist + Radius  →  이것도 음수면 전부 바깥
	bool IsOutsidePlane(const FPlane& Plane, const FVector& AbsNormal, const FAxisAlignedBoundingBox& Box)
	{
		const float CenterDist = Plane.Distance(Box.Center);
		const float Radius = AbsNormal.Dot(Box.Extent);
		return CenterDist + Radius < 0.0f;
	}

	// 6개 평면 중 하나라도 완전히 바깥이면 컬링. 걸치면 가시(보수적)
	bool IsVisible(const FFrustum& Frustum, const FVector(&AbsNormals)[FFrustum::PlaneCount], const FAxisAlignedBoundingBox& Box)
	{
		for (int32 p = 0; p < FFrustum::PlaneCount; ++p)
		{
			if (IsOutsidePlane(Frustum.Planes[p], AbsNormals[p], Box))
			{
				return false;
			}
		}
		return true;
	}
}*/

uint32 FFlatFrustumCuller::Cull(const FFrustum& Frustum, const TArray<FAxisAlignedBoundingBox>& CullDataList, TArray<uint8>& OutVisibleFlags)
{
	OutVisibleFlags.resize(CullDataList.size());

	// |n|은 평면마다 고정이므로 오브젝트 루프 밖에서 한 번만 계산
	FVector AbsNormals[FFrustum::PlaneCount];
	for (int32 p = 0; p < FFrustum::PlaneCount; ++p)
	{
		AbsNormals[p] = FrustumUtils::AbsVector(Frustum.Planes[p].Normal);
	}

	uint32 VisibleCount = 0;
	for (size_t i = 0; i < CullDataList.size(); ++i)
	{
		const bool bVisible = FrustumUtils::IsVisible(Frustum, AbsNormals, CullDataList[i]);
		OutVisibleFlags[i] = bVisible ? 1 : 0;
		VisibleCount += bVisible ? 1 : 0;
	}

	return VisibleCount;
}

uint32 FFlatFrustumCuller::Cull_SIMD(const FFrustum& Frustum, const FAxisAlignedBoundingBox* Boxes, uint32 Count, uint8* OutVisibleFlags)
{
	uint32 VisibleCount = 0;

	FMathSSE::VectorRegister4Float PlaneX[FFrustum::PlaneCount], PlaneY[FFrustum::PlaneCount], PlaneZ[FFrustum::PlaneCount], PlaneW[FFrustum::PlaneCount];
	FMathSSE::VectorRegister4Float AbsNormalX[FFrustum::PlaneCount], AbsNormalY[FFrustum::PlaneCount], AbsNormalZ[FFrustum::PlaneCount];

	for (int32 p = 0; p < FFrustum::PlaneCount; ++p)
	{
		const FPlane& Plane = Frustum.Planes[p];

		PlaneX[p] = FMathSSE::VectorSetFloat1(Plane.Normal.X);
		PlaneY[p] = FMathSSE::VectorSetFloat1(Plane.Normal.Y);
		PlaneZ[p] = FMathSSE::VectorSetFloat1(Plane.Normal.Z);
		PlaneW[p] = FMathSSE::VectorSetFloat1(Plane.D);

		AbsNormalX[p] = FMathSSE::VectorAbs(PlaneX[p]);
		AbsNormalY[p] = FMathSSE::VectorAbs(PlaneY[p]);
		AbsNormalZ[p] = FMathSSE::VectorAbs(PlaneZ[p]);
	}

	const uint32 AlignedCount = Count & ~3u;

	for (uint32 i = 0; i < AlignedCount; i += 4)
	{
		FMathSSE::VectorRegister4Float BoxCenterX = FMathSSE::VectorSet(Boxes[i + 0].Center.X, Boxes[i + 1].Center.X, Boxes[i + 2].Center.X, Boxes[i + 3].Center.X);
		FMathSSE::VectorRegister4Float BoxCenterY = FMathSSE::VectorSet(Boxes[i + 0].Center.Y, Boxes[i + 1].Center.Y, Boxes[i + 2].Center.Y, Boxes[i + 3].Center.Y);
		FMathSSE::VectorRegister4Float BoxCenterZ = FMathSSE::VectorSet(Boxes[i + 0].Center.Z, Boxes[i + 1].Center.Z, Boxes[i + 2].Center.Z, Boxes[i + 3].Center.Z);

		FMathSSE::VectorRegister4Float BoxExtentX = FMathSSE::VectorSet(Boxes[i + 0].Extent.X, Boxes[i + 1].Extent.X, Boxes[i + 2].Extent.X, Boxes[i + 3].Extent.X);
		FMathSSE::VectorRegister4Float BoxExtentY = FMathSSE::VectorSet(Boxes[i + 0].Extent.Y, Boxes[i + 1].Extent.Y, Boxes[i + 2].Extent.Y, Boxes[i + 3].Extent.Y);
		FMathSSE::VectorRegister4Float BoxExtentZ = FMathSSE::VectorSet(Boxes[i + 0].Extent.Z, Boxes[i + 1].Extent.Z, Boxes[i + 2].Extent.Z, Boxes[i + 3].Extent.Z);

		FMathSSE::VectorRegister4Float OutsideMask = FMathSSE::VectorZero();

		for (int p = 0; p < FFrustum::PlaneCount; ++p)
		{
			FMathSSE::VectorRegister4Float CenterDist = FMathSSE::VectorAdd(FMathSSE::VectorMul(BoxCenterX, PlaneX[p]), PlaneW[p]);
			CenterDist = FMathSSE::VectorAdd(CenterDist, FMathSSE::VectorMul(BoxCenterY, PlaneY[p]));
			CenterDist = FMathSSE::VectorAdd(CenterDist, FMathSSE::VectorMul(BoxCenterZ, PlaneZ[p]));

			FMathSSE::VectorRegister4Float ExtentRadius = FMathSSE::VectorMul(BoxExtentX, AbsNormalX[p]);
			ExtentRadius = FMathSSE::VectorAdd(ExtentRadius, FMathSSE::VectorMul(BoxExtentY, AbsNormalY[p]));
			ExtentRadius = FMathSSE::VectorAdd(ExtentRadius, FMathSSE::VectorMul(BoxExtentZ, AbsNormalZ[p]));

			FMathSSE::VectorRegister4Float Outside = FMathSSE::VectorCompareLT(FMathSSE::VectorAdd(CenterDist, ExtentRadius), FMathSSE::VectorZero());
			OutsideMask = FMathSSE::VectorOr(OutsideMask, Outside);
		}

		int mask = FMathSSE::VectorMaskBits(OutsideMask);

		OutVisibleFlags[i + 0] = !(mask & 1);
		OutVisibleFlags[i + 1] = !((mask >> 1) & 1);
		OutVisibleFlags[i + 2] = !((mask >> 2) & 1);
		OutVisibleFlags[i + 3] = !((mask >> 3) & 1);

		VisibleCount += OutVisibleFlags[i + 0] + OutVisibleFlags[i + 1] + OutVisibleFlags[i + 2] + OutVisibleFlags[i + 3];
	}

	FVector AbsNormals[FFrustum::PlaneCount];
	for (int32 p = 0; p < FFrustum::PlaneCount; ++p)
	{
		AbsNormals[p] = FrustumUtils::AbsVector(Frustum.Planes[p].Normal);
	}

	for(int32 i = AlignedCount; i < Count; ++i)
	{
		OutVisibleFlags[i] = FrustumUtils::IsVisible(Frustum, AbsNormals, Boxes[i]) ? 1 : 0;
		VisibleCount += OutVisibleFlags[i];	
	}

	return VisibleCount;
}