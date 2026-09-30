#pragma once

#include "Runtime/Math/FVector.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Engine/FCamera.h"

class UPrimitiveComponent;
class FMesh;
struct FAxisAlignedBoundingBox;

struct FRay
{
	FVector Origin;
	FVector Direction;
};

namespace FRayCastingManager
{
    // true면 미리 펼친 삼각형 배열(TriangleVertices)로, false면 기존 인덱스 방식으로 검사한다. 성능 비교용.
    inline bool bUseFlattenedTriangles = true;

    // true면 미리 빌드된 Local Mesh BVH를 사용하여 Ray검사
    inline bool bUseMeshBVH = true;

    FRay CreateRayFromScreenPosition(const FCamera& Camera, const FVector2& MousePosition, const FVector2& ViewportSize);
    bool RayIntersectsMeshes(const FRay& Ray, const FCamera& Camera, const TArray<UPrimitiveComponent*>& Components, UPrimitiveComponent*& HitComponent, FVector& OutImpactPoint);
    bool RayIntersectsAABB(const FRay& Ray, const FAxisAlignedBoundingBox& AABB, float &OutTNear);
    // 광선 방향의 역수. 순회 전에 한 번 구해 박스 검사마다 나눗셈 대신 곱셈을 쓴다.
    FVector MakeInvDir(const FVector& Direction);
    bool RayIntersectsBoundsInv(const FVector& Origin, const FVector& InvDir, const FVector& Min, const FVector& Max, float& OutTNear);
    bool IntersectMeshBVH(const FRay& ObjectRay, const FMesh& Mesh, float& OutClosestHit);
    bool RayIntersectsMesh(const FRay& Ray, const FMesh& Mesh, const FMatrix& ModelMatrix, float& OutDistance, FVector& OutImpactPoint, float& ClosestHit, bool bBVH = false);
    bool RayIntersectsMeshWithInversedModel(const FRay& Ray, const FMesh& Mesh, const FMatrix& InversedModelMatrix, float& OutDistance, FVector& OutImpactPoint, float& ClosestHit, bool bBVH = false);
    bool RayIntersectsTriangle(const FRay& Ray, const FVector& A, const FVector& B, const FVector& C, float& OutT);
};
