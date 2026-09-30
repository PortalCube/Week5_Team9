#pragma once
#include "Core.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Engine/FSceneBVH.h"
#include "Runtime/Geometry/FFrustum.h"
#include <cstdint>

struct FFrustum;

//항상 보이게 된다. 아주 큰 Extent를 가진 AABB를 사용하므로 어떤 평면에서도 d < -r이 되지 않는다.
inline FAxisAlignedBoundingBox MakeAlwaysVisibleCullData()
{
	constexpr float Huge = 1.0e30f;
	return FAxisAlignedBoundingBox{ FVector{ 0.f, 0.f, 0.f }, FVector{ Huge, Huge, Huge } };
}

namespace FrustumUtils
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

class IPrimitiveCuller
{
public:
	virtual ~IPrimitiveCuller() = default;

	virtual uint32 Cull(const FFrustum& Frustum, const TArray<FAxisAlignedBoundingBox>& CullDataList, TArray<uint8>& OutVisibleFlags) = 0;
};

//공간 분할 없는, SIMD 안쓰는 기본 Frustum Culling.
//추후 공간 분할, SIMD가 추가된다면 늘려나갈것
class FFlatFrustumCuller final : public IPrimitiveCuller
{
public:
	uint32 Cull(const FFrustum& Frustum, const TArray<FAxisAlignedBoundingBox>& CullDataList, TArray<uint8>& OutVisibleFlags) override;
};