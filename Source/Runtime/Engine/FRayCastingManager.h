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

    // 피킹 한 번의 작업량과 구간 시간. 피킹 직전에 Reset하고, 끝나면 로그로 출력한다.
    struct FPickProfile
    {
        uint32 SceneNodes = 0;      // 씬 BVH에서 방문한 노드 수
        uint32 ObjectBoxTests = 0;  // 월드 AABB를 검사한 오브젝트 수
        uint32 MeshTests = 0;       // 메시(삼각형) 검사까지 간 오브젝트 수
        uint32 MeshNodes = 0;       // 메시 BVH에서 방문한 노드 수
        uint32 Triangles = 0;       // 검사한 삼각형 수
        double MeshMs = 0.0;        // 메시 검사(광선 로컬 변환 포함)에 쓴 시간의 합

        void Reset() { *this = FPickProfile{}; }
    };
    inline FPickProfile PickProfile;

    // 마지막으로 클릭한 피킹 광선. 같은 광선으로 반복 측정(벤치마크)할 때 쓴다.
    inline FRay LastPickRay{};
    inline bool bHasLastPickRay = false;

    // 마우스 화면 좌표를 카메라 기준 월드 공간 광선(시작점, 단위 방향)으로 바꾼다.
    FRay CreateRayFromScreenPosition(const FCamera& Camera, const FVector2& MousePosition, const FVector2& ViewportSize);

    // BVH 없이 모든 컴포넌트를 선형 검사해 가장 가까운 컴포넌트와 교차점을 찾는다. (비교용 선형 경로)
    bool RayIntersectsMeshes(const FRay& Ray, const FCamera& Camera, const TArray<UPrimitiveComponent*>& Components, UPrimitiveComponent*& HitComponent, FVector& OutImpactPoint);
    
    // 광선과 AABB의 교차 검사(나눗셈 방식). 맞으면 박스 진입 거리를 OutTNear에 담는다.
    bool RayIntersectsAABB(const FRay& Ray, const FAxisAlignedBoundingBox& AABB, float &OutTNear);
    
    // 광선 방향의 역수. 순회 전에 한 번 구해 박스 검사마다 나눗셈 대신 곱셈을 쓴다.
    FVector MakeInvDir(const FVector& Direction);
    
    // MakeInvDir로 구한 역방향을 쓰는 AABB 교차 검사(곱셈만 사용). 씬/메시 BVH 순회용.
    bool RayIntersectsBoundsInv(const FVector& Origin, const FVector& InvDir, const FVector& Min, const FVector& Max, float& OutTNear);
    
    // 메시 로컬 BVH를 가까운 노드부터 순회해 OutClosestHit보다 가까운 삼각형 교차를 찾는다. ObjectRay는 로컬 공간 광선.
    bool IntersectMeshBVH(const FRay& ObjectRay, const FMesh& Mesh, float& OutClosestHit);
    
    // 월드 행렬의 역행렬을 계산한 뒤 RayIntersectsMeshWithInversedModel로 검사한다. (기즈모처럼 행렬을 매번 조합하는 경우용)
    bool RayIntersectsMesh(const FRay& Ray, const FMesh& Mesh, const FMatrix& ModelMatrix, float& OutDistance, FVector& OutImpactPoint, float& ClosestHit, bool bBVH = false);
    
    // 역행렬로 광선을 로컬로 옮겨 메시를 검사한다. ClosestHit보다 가까운 교차가 있으면 갱신하고 true. bBVH면 로컬 AABB 사전 검사를 생략.
    bool RayIntersectsMeshWithInversedModel(const FRay& Ray, const FMesh& Mesh, const FMatrix& InversedModelMatrix, float& OutDistance, FVector& OutImpactPoint, float& ClosestHit, bool bBVH = false);
    
    // 광선과 삼각형 ABC의 교차 검사(Möller–Trumbore). 맞으면 광선 거리 t를 OutT에 담는다.
    bool RayIntersectsTriangle(const FRay& Ray, const FVector& A, const FVector& B, const FVector& C, float& OutT);
};
