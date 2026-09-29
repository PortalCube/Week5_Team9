#pragma once
#include "Core.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"

struct FFrustum;

//항상 보이게 된다. 아주 큰 Extent를 가진 AABB를 사용하므로 어떤 평면에서도 d < -r이 되지 않는다.
inline FAxisAlignedBoundingBox MakeAlwaysVisibleCullData()
{
	constexpr float Huge = 1.0e30f;
	return FAxisAlignedBoundingBox{ FVector{ 0.f, 0.f, 0.f }, FVector{ Huge, Huge, Huge } };
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