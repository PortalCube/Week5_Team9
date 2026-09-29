#pragma once

#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Actors/AActor.h"
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
    //void QueryFrustum(const FFrustum& Frustum, float MinScreenPixels, TArray<UPrimitiveComponent*>& OutVisible) const;
    bool QueryRay(const FRay& Ray, UPrimitiveComponent*& OutHit, FVector& OutImpact) const;

    //BVH Edit
    void AddObject(UPrimitiveComponent* C);
    void RemoveObject(UPrimitiveComponent* C);

private:
    void BuildRecursive(uint32 NodeIdx, uint32 Start, uint32 Count, uint32 ParentIdx);
    void TraverseRay(uint32 NodeIdx, const FRay& Ray, float& Closest, UPrimitiveComponent*& OutHit, FVector& OutImpact) const;
    void TestObjectRay(UPrimitiveComponent* C, const FAxisAlignedBoundingBox& WorldBox, const FRay& Ray, float& Closest, UPrimitiveComponent*& OutHit, FVector& OutImpact) const;

    //해당 LeafNode에 영향 받는 BVHNode 모두 갱신
    void RefitFromLeaf(uint32 LeafNodeIndex);

    TArray<FSceneBVHNode> Nodes;                    //BVH Node
    TArray<UPrimitiveComponent*> Objects;           //BVH에 포함된 UPrimitiveComponent 배열
    TArray<FAxisAlignedBoundingBox> ObjectBounds;   //Objects의 index에 해당하는 prim의 BoundingBox
    TArray<uint32> LeafOfObject;                    //여려개의 BVHIndex -> 하나의 Leaf BVHNode 맵핑

    /*
    LeafSize 비교
    LeafSize = 4  2.54 3.87 4.55 2.19 2.32 (min = 2.19, max = 4.55, avg = 3.09)
    LeafSize = 8  3.23 2.19 2.06 2.48 2.78 (min = 2.19, max = 3.23, avg = 2.55)
    LeafSize =16  2.64 3.16 5.40 3.88 3.07 (min = 2.64, max = 5.40, avg = 3.63)
    */
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