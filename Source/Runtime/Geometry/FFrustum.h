#pragma once
#include "Core.h"

struct FMatrix;

//1개 평면
struct FPlane
{
	FVector Normal{ 0.f, 0.f, 0.f };
	float Dist = 0.f;

	//안쪽 >= 0
	float Distance(const FVector& Point) const { return Normal.Dot(Point) + Dist; }
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

	//
	static FFrustum FromViewProjection(const FMatrix& ViewProj);
};