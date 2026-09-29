#include "FCulling.h"
#include "Runtime/Geometry/FFrustum.h"

namespace
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
}

uint32 FFlatFrustumCuller::Cull(const FFrustum& Frustum, const TArray<FAxisAlignedBoundingBox>& CullDataList, TArray<uint8>& OutVisibleFlags)
{
	OutVisibleFlags.resize(CullDataList.size());

	// |n|은 평면마다 고정이므로 오브젝트 루프 밖에서 한 번만 계산
	FVector AbsNormals[FFrustum::PlaneCount];
	for (int32 p = 0; p < FFrustum::PlaneCount; ++p)
	{
		AbsNormals[p] = AbsVector(Frustum.Planes[p].Normal);
	}

	uint32 VisibleCount = 0;
	for (size_t i = 0; i < CullDataList.size(); ++i)
	{
		const bool bVisible = IsVisible(Frustum, AbsNormals, CullDataList[i]);
		OutVisibleFlags[i] = bVisible ? 1 : 0;
		VisibleCount += bVisible ? 1 : 0;
	}

	return VisibleCount;
}
