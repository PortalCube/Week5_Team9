#include "FCulling.h"
#include "Runtime/Geometry/FFrustum.h"

uint32 FFlatFrustumCuller::Cull(const FFrustum& Frustum, const TArray<FAxisAlignedBoundingBox>& CullDataList, TArray<uint8>& OutVisibleFlags)
{
	const size_t Count = CullDataList.size();
	OutVisibleFlags.resize(Count);

	constexpr int32 PlaneCount = FFrustum::PlaneCount;

	float Nx[PlaneCount];
	float Ny[PlaneCount];
	float Nz[PlaneCount];
	float Nd[PlaneCount];
	float Ax[PlaneCount];
	float Ay[PlaneCount];
	float Az[PlaneCount];

	for (int32 p = 0; p < PlaneCount; p++)
	{
		const FPlane& Plane = Frustum.Planes[p];

		Nx[p] = Plane.Normal.X;
		Ny[p] = Plane.Normal.Y;
		Nz[p] = Plane.Normal.Z;
		Nd[p] = Plane.Dist;
		Ax[p] = std::fabs(Plane.Normal.X);
		Ay[p] = std::fabs(Plane.Normal.Y);
		Az[p] = std::fabs(Plane.Normal.Z);
	}

	const FAxisAlignedBoundingBox* Data = CullDataList.data();
	uint8* Out = OutVisibleFlags.data();
	uint32 VisibleCount = 0;

	//전부 순회하며 컬링
	for (size_t i = 0; i < Count; i++)
	{
		const FVector& C = Data[i].Center;
		const FVector& E = Data[i].Extent;

		uint8 bVisible = 1;
		for (int32 p = 0; p < PlaneCount; p++){
			const float Dist = Nx[p] * C.X + Ny[p] * C.Y + C.Z * Nz[p] + Nd[p];
			const float Radius = Ax[p] * E.X + Ay[p] * E.Y + Az[p] * E.Z;

			if (Dist < -Radius)
			{
				bVisible = 0;
				break;
			}
		}

		Out[i] = bVisible;
		VisibleCount += bVisible;
	}

	return VisibleCount;
}
