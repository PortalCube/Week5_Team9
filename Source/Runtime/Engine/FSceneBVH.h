#pragma once

#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/Geometry/FFrustum.h"
#include <cstdint>


//임시 FPlane, FFrustum, 조훈님이 만들면 그 자료구조로 교체
//struct FPlane { FVector Normal; float D; };

//struct FFrustum
//{
//    FPlane Planes[6];
//    static FFrustum FromViewProj(const FMatrix& ViewProj);
//};

// ★ bool이 아니라 3-상태여야 함
enum class EIntersection : uint8 { Outside, Intersect, Inside };

//EIntersection TestAABB(const FFrustum& Frustum, const FAxisAlignedBoundingBox& Box);


class FSceneBVH
{
public:
    struct FSceneBVHNode
    {
        FAxisAlignedBoundingBox Bounds;
        uint32 Left;
        uint32 Parent = UINT32_MAX;
        uint32 ObjStart = 0;
        uint32 ObjCount = 0;
        bool bLeafNode = false;
    };
    struct FPrimRef
    {
        FAxisAlignedBoundingBox WorldBox;   //World 기준 AABB 박스
        FVector                 Centroid;   //박스 중심
        UPrimitiveComponent*    Comp;       //컴포넌트 포인터
    };

public:

    //Build
    void Build(const TArray<UPrimitiveComponent*>& Components);
    bool ShouldRebuild() const;

    //Tranform 변경된 PrimitiveComponent의 ObjectBounds 갱신
    void RefitObject(UPrimitiveComponent* Moved);

    //Query
    bool QueryFrustum(const FFrustum& Frustum, float MinScreenPixels, TArray<UPrimitiveComponent*>& OutVisible) const;
    bool QueryRay(const FRay& Ray, UPrimitiveComponent*& OutHit, FVector& OutImpact) const;

    //BVH Edit
    void AddObject(UPrimitiveComponent* C);
    void RemoveObject(UPrimitiveComponent* C);

private:
    void BuildRecursive(uint32 NodeIdx, uint32 Start, uint32 Count, uint32 ParentIdx);
    void TraverseRay(uint32 NodeIdx, const FRay& Ray, const FVector& InvDir, float& Closest, UPrimitiveComponent*& OutHit, FVector& OutImpact) const;
    //월드 AABB 검사 후 통과하면 메시를 검사한다 (대기열 오브젝트용)
    void TestObjectRay(UPrimitiveComponent* C, const FAxisAlignedBoundingBox& WorldBox, const FRay& Ray, const FVector& InvDir, float& Closest, UPrimitiveComponent*& OutHit, FVector& OutImpact) const;
    //월드 AABB를 통과한 오브젝트의 메시(삼각형)를 검사한다
    void TestObjectMesh(UPrimitiveComponent* C, const FRay& Ray, float& Closest, UPrimitiveComponent*& OutHit, FVector& OutImpact) const;
    void TraverseFrustum(uint32 NodeIdx, const FFrustum& Frustum, const FVector(&AbsNormals)[FFrustum::PlaneCount], TArray<UPrimitiveComponent*>& OutVisible) const;

    //해당 LeafNode에 영향 받는 BVHNode 모두 갱신
    void RefitFromLeaf(uint32 LeafNodeIndex);

    TArray<FSceneBVHNode> Nodes;                    //BVH Node
    TArray<UPrimitiveComponent*> Objects;           //BVH에 포함된 UPrimitiveComponent 배열
    TArray<FAxisAlignedBoundingBox> ObjectBounds;   //Objects의 index에 해당하는 prim의 BoundingBox
    TArray<uint32> LeafOfObject;                    //여려개의 BVHIndex -> 하나의 Leaf BVHNode 맵핑

    uint32 LeafSize = 8;

    TArray<UPrimitiveComponent*> PendingObjects;
    uint32 PendingLimit = 256;
    uint32 RemovedCount = 0;

private:
    TArray<FPrimRef> Prims;     // 빌드 중 작업 버퍼
};

inline static void RefitActorInBVH(FSceneBVH& BVH, AActor* Actor)
{
    if (!Actor) { return; }

    if (USceneComponent* Root = Actor->GetRootComponent())
    {
        if (UPrimitiveComponent* P = Root->Cast<UPrimitiveComponent>()) { BVH.RefitObject(P); }
    }

    for (USceneComponent* S : Actor->GetAttachedComponents())
    {
        if (!S) { continue; }
        if (UPrimitiveComponent* P = S->Cast<UPrimitiveComponent>()) { BVH.RefitObject(P); }
    }
}