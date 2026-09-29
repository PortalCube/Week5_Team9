#pragma once
#include "Core.h"

struct FMatrix;

//1개 평면
struct FPlane
{
	FVector Normal{ 0.f, 0.f, 0.f };
	float D = 0.f;

	//점까지의 부호있는 거리. 안쪽 >= 0, 바깥쪽 < 0
	float Distance(const FVector& Point) const { return Normal.Dot(Point) + D; }
};

//6개 평면으로 이루어진 Frustum
struct FFrustum
{
	enum EPlane : int32
	{
		Left = 0,
		Right,
		Bottom,
		Top,
		Near,
		Far,
		PlaneCount
	};

	FPlane Planes[PlaneCount];

	//ViewProj 행렬에서 Frustum을 추출합니다.
	static FFrustum FromViewProjection(const FMatrix& ViewProj);
};