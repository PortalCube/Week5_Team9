#include "pch.h"
#include "FSceneBVH.h"
#include "Runtime/Math/FMathSSE.h"
#include <smmintrin.h>
#include <algorithm>

void FSceneBVH::Split2Way(uint32 Start, uint32 Count, FSubRange& OutLeft, FSubRange& OutRight)
{
    if (Count <= 1)
    {
        OutLeft = { Start, Count };
        OutRight = { 0, 0 };
        return;
    }

    FAxisAlignedBoundingBox CentroidBounds;
    for (uint32 i = Start; i < Start + Count; ++i)
    {
        const FPrimRef& P = Prims[i];
        for (int a = 0; a < 3; ++a)
        {
            CentroidBounds.Min[a] = std::min(CentroidBounds.Min[a], P.Centroid[a]);
            CentroidBounds.Max[a] = std::max(CentroidBounds.Max[a], P.Centroid[a]);
        }
    }

    const FVector Extent = CentroidBounds.Max - CentroidBounds.Min;
    int Axis = 0;
    if (Extent.Y > Extent[Axis]) Axis = 1;
    if (Extent.Z > Extent[Axis]) Axis = 2;

    const uint32 Mid = Start + Count / 2;
    std::nth_element(
        Prims.begin() + Start,
        Prims.begin() + Mid,
        Prims.begin() + Start + Count,
        [Axis](const FPrimRef& A, const FPrimRef& B) { return A.Centroid[Axis] < B.Centroid[Axis]; });

    OutLeft = { Start, Mid - Start };
    OutRight = { Mid, Start + Count - Mid };
}

uint32 FSceneBVH::Split4Way(uint32 Start, uint32 Count, FSubRange OutRanges[4])
{
    FSubRange Left, Right;
    Split2Way(Start, Count, Left, Right);

    uint32 RangeCount = 0;

    if (Left.Count > LeafSize)
    {
        FSubRange L0, L1;
        Split2Way(Left.Start, Left.Count, L0, L1);
        if (L0.Count > 0) OutRanges[RangeCount++] = L0;
        if (L1.Count > 0) OutRanges[RangeCount++] = L1;
    }
    else if (Left.Count > 0)
    {
        OutRanges[RangeCount++] = Left;
    }

    if (Right.Count > LeafSize)
    {
        FSubRange R0, R1;
        Split2Way(Right.Start, Right.Count, R0, R1);
        if (R0.Count > 0) OutRanges[RangeCount++] = R0;
        if (R1.Count > 0) OutRanges[RangeCount++] = R1;
    }
    else if (Right.Count > 0)
    {
        OutRanges[RangeCount++] = Right;
    }

    return RangeCount;
}

void FSceneBVH::Build(const TArray<UPrimitiveComponent*>& Components)
{
    for (UPrimitiveComponent* C : Objects) { if (C) { C->SetBVHIndex(-1); } }
    for (UPrimitiveComponent* C : PendingObjects) { if (C) { C->SetBVHIndex(-1); } }

    PendingObjects.clear();
    RemovedCount = 0;

    Nodes.clear(); Objects.clear(); Prims.clear(); ObjectBounds.clear(); LeafOfObject.clear();
    Prims.reserve(Components.size());

    for (UPrimitiveComponent* C : Components)
    {
        if (!C) continue;

        const FAxisAlignedBoundingBox& Local = C->GetLocalBounds();

        //빈 박스는 BVH에서 제외한다.
        if (!Local.IsValid()) { continue; }

        const FMatrix World = C->GetGlobalTransformMatrix();
        FAxisAlignedBoundingBox WorldBox(Local, World);

        //{AABB, 중심점, 컴포넌트}
        Prims.push_back({ WorldBox, (WorldBox.Min + WorldBox.Max) * 0.5f, C });
    }

    //단 한 개도 없으면 안만듬
    if (Prims.empty()) return;

    //BVH 최대 크기는 컴포넌트 수*2 를 넘지 않음(리프가 컴포넌트 수 + 부모 노드 수가 그보단 작음)
    Nodes.reserve(Prims.size());
    Nodes.push_back({});

    //재귀 돌면 BVH 구성
    BuildRecursive(0, 0, (uint32)Prims.size(), UINT32_MAX);

    //사용된 UPrimitiveComp와 LeafBounds 저장
    Objects.reserve(Prims.size());
    ObjectBounds.reserve(Prims.size());
    for (const FPrimRef& P : Prims)
    {
        Objects.push_back(P.Comp);
        ObjectBounds.push_back(P.WorldBox);
    }

    Prims.clear();
    Prims.shrink_to_fit();

    // Component -> BVHNode 질의를 위한 자료구조 구성
    LeafOfObject.resize(Objects.size());
    for (uint32 n = 0; n < Nodes.size(); ++n)
    {
        const FSceneBVHNode& Node = Nodes[n];
        if (Node.ObjCount == 0) { continue; }    // 내부 노드는 건너뜀
        for (uint32 k = 0; k < Node.ObjCount; ++k)
        {
            LeafOfObject[Node.ObjStart + k] = n;
        }
    }

    //후에 질의를 위한 BVH Index 저장
    for (uint32 i = 0; i < Objects.size(); ++i)
    {
        if (Objects[i])
            Objects[i]->SetBVHIndex(static_cast<int32>(i));
    }
}

void FSceneBVH::BuildRecursive(uint32 NodeIdx, uint32 Start, uint32 Count, uint32 ParentIdx)
{
    FAxisAlignedBoundingBox Bounds;
    FAxisAlignedBoundingBox CentroidBounds;

    for (uint32 i = Start; i < Start + Count; ++i)
    {
        const FPrimRef& P = Prims[i];
        for (int a = 0; a < 3; ++a)
        {
            Bounds.Min[a] = std::min(Bounds.Min[a], P.WorldBox.Min[a]);
            Bounds.Max[a] = std::max(Bounds.Max[a], P.WorldBox.Max[a]);
            CentroidBounds.Min[a] = std::min(CentroidBounds.Min[a], P.Centroid[a]);
            CentroidBounds.Max[a] = std::max(CentroidBounds.Max[a], P.Centroid[a]);
        }
    }
    Bounds.Center = (Bounds.Min + Bounds.Max) * 0.5f;
    Bounds.Extent = (Bounds.Max - Bounds.Min) * 0.5f;

    Nodes[NodeIdx].Bounds = Bounds;
    Nodes[NodeIdx].Parent = ParentIdx;

    const FVector Extent = CentroidBounds.Max - CentroidBounds.Min;
    int Axis = 0;
    if (Extent.Y > Extent[Axis]) Axis = 1;
    if (Extent.Z > Extent[Axis]) Axis = 2;

    const bool bDegenerate = Extent[Axis] < 1e-6f;
    if (Count <= LeafSize || (bDegenerate && Count <= LeafSize * 4))
    {
        Nodes[NodeIdx].ObjStart = Start;
        Nodes[NodeIdx].ObjCount = Count;
        Nodes[NodeIdx].bLeafNode = true;
        return;
    }

    FSubRange Ranges[4];
    const uint32 NumChildren = Split4Way(Start, Count, Ranges);

    if (NumChildren <= 1)
    {
        Nodes[NodeIdx].ObjStart = Start;
        Nodes[NodeIdx].ObjCount = Count;
        Nodes[NodeIdx].bLeafNode = true;
        return;
    }

    const uint32 ChildBase = static_cast<uint32>(Nodes.size());
    Nodes.resize(ChildBase + NumChildren);

    //리프 노드가 아니면 ObjCount = 0
    Nodes[NodeIdx].ChildCount = static_cast<uint8>(NumChildren);
    Nodes[NodeIdx].ObjCount = 0;
    Nodes[NodeIdx].bLeafNode = false;

    for (uint32 i = 0; i < NumChildren; ++i)
    {
        Nodes[NodeIdx].Children[i] = ChildBase + i;
        BuildRecursive(ChildBase + i, Ranges[i].Start, Ranges[i].Count, NodeIdx);
    }

    for (uint32 i = 0; i < NumChildren; ++i)
    {
        const uint32 ChildIdx = ChildBase + i;
        const FAxisAlignedBoundingBox& CB = Nodes[ChildIdx].Bounds;
        Nodes[NodeIdx].ChildCenterX[i] = CB.Center.X;
        Nodes[NodeIdx].ChildCenterY[i] = CB.Center.Y;
        Nodes[NodeIdx].ChildCenterZ[i] = CB.Center.Z;
        Nodes[NodeIdx].ChildExtentX[i] = CB.Extent.X;
        Nodes[NodeIdx].ChildExtentY[i] = CB.Extent.Y;
        Nodes[NodeIdx].ChildExtentZ[i] = CB.Extent.Z;
    }

    for (uint32 i = NumChildren; i < 4; ++i)
    {
        Nodes[NodeIdx].ChildCenterX[i] = 0.0f;
        Nodes[NodeIdx].ChildCenterY[i] = 0.0f;
        Nodes[NodeIdx].ChildCenterZ[i] = 0.0f;
        Nodes[NodeIdx].ChildExtentX[i] = -1.0f;
        Nodes[NodeIdx].ChildExtentY[i] = -1.0f;
        Nodes[NodeIdx].ChildExtentZ[i] = -1.0f;
        Nodes[NodeIdx].Children[i] = UINT32_MAX;
    }
}

void FSceneBVH::RefitObject(UPrimitiveComponent* Moved)
{
    if (!Moved) return;

    const int32 ObjectIndex = Moved->GetBVHIndex();
    if (ObjectIndex < 0 || static_cast<uint32>(ObjectIndex) >= Objects.size()) return;

    const FAxisAlignedBoundingBox Local = Moved->GetLocalBounds();
    if (!Local.IsValid()) { return; }

    //변경된 Transform으로 AABB 다시 넣기
    ObjectBounds[ObjectIndex] = FAxisAlignedBoundingBox(Local, Moved->GetGlobalTransformMatrix());

    RefitFromLeaf(LeafOfObject[ObjectIndex]);
}

void FSceneBVH::RefitFromLeaf(uint32 LeafNodeIndex)
{
    if (LeafNodeIndex >= Nodes.size()) { return; }
    if (Nodes[LeafNodeIndex].ObjCount == 0) { return; }   // 내부 노드면 잘못된 호출

    //Leaf Node의 AABB를 소속 오브젝트 전체로 재계산
    uint32 NodeIdx = LeafNodeIndex;
    FSceneBVHNode& Leaf = Nodes[NodeIdx];

    FAxisAlignedBoundingBox NewBounds;
    for (uint32 i = Leaf.ObjStart; i < Leaf.ObjStart + Leaf.ObjCount; ++i)
    {
        NewBounds = FAxisAlignedBoundingBox::Union(NewBounds, ObjectBounds[i]);
    }

    // 재계산한 AABB가 그대로면 여기서 종료
    if (NewBounds == Leaf.Bounds) { return; }

    Leaf.Bounds = NewBounds;
    NodeIdx = Leaf.Parent;

    //루트 도달 전까지 부모로 거슬러 올라가면 AABB 재계산
    while (NodeIdx != UINT32_MAX)
    {
        FSceneBVHNode& N = Nodes[NodeIdx];

        FAxisAlignedBoundingBox Merged;
        for (uint8 c = 0; c < N.ChildCount; ++c)
        {
            const uint32 ChildIdx = N.Children[c];
            const FAxisAlignedBoundingBox& CB = Nodes[ChildIdx].Bounds;
            Merged = FAxisAlignedBoundingBox::Union(Merged, CB);

            N.ChildCenterX[c] = CB.Center.X;
            N.ChildCenterY[c] = CB.Center.Y;
            N.ChildCenterZ[c] = CB.Center.Z;
            N.ChildExtentX[c] = CB.Extent.X;
            N.ChildExtentY[c] = CB.Extent.Y;
            N.ChildExtentZ[c] = CB.Extent.Z;
        }

        if (Merged == N.Bounds) { break; }
        N.Bounds = Merged;
        NodeIdx = N.Parent;
    }
}

//PendingObjects에 일정 이상 쌓이거나 삭제된 Prim이 일정 이상 쌓이면 트리를 다시 빌드할 필요가 있다.
bool FSceneBVH::ShouldRebuild() const
{
    if (PendingObjects.size() > PendingLimit) { return true; }
    if (!Objects.empty() && RemovedCount * 4 > Objects.size()) { return true; }
    return false;
}

bool FSceneBVH::QueryFrustum(const FFrustum& Frustum, float MinScreenPixels, TArray<UPrimitiveComponent*>& OutVisible) const
{
    OutVisible.clear();
    OutVisible.reserve(Objects.size());

    FVector AbsNormals[FFrustum::PlaneCount];

    // |n|은 평면마다 고정이므로 오브젝트 루프 밖에서 한 번만 계산
    for (int32 p = 0; p < FFrustum::PlaneCount; ++p)
    {
        AbsNormals[p] = FrustumUtils::AbsVector(Frustum.Planes[p].Normal);
    }

    if (!Nodes.empty())
    {
        TraverseFrustum(0, Frustum, AbsNormals, OutVisible);
    }

    for (UPrimitiveComponent* C : PendingObjects)
    {
        if (!C) { continue; }

        const FAxisAlignedBoundingBox Local = C->GetLocalBounds();
        if (!Local.IsValid()) { continue; }

        const FAxisAlignedBoundingBox World(Local, C->GetGlobalTransformMatrix());

        if (FrustumUtils::IsVisible(Frustum, AbsNormals, World))
        {
            OutVisible.push_back(C);
        }
    }

    return !OutVisible.empty();
}

bool FSceneBVH::QueryRay(const FRay& Ray, UPrimitiveComponent*& OutHit, FVector& OutImpact) const
{
    OutImpact = FVector{};
    float Closest = (std::numeric_limits<float>::max)();

    //광선 방향은 순회 내내 같으므로 역수를 한 번만 구한다
    const FVector InvDir = FRayCastingManager::MakeInvDir(Ray.Direction);

    //1) 트리 순회
    if (!Nodes.empty())
    {
        TraverseRay(0, Ray, InvDir, Closest, OutHit, OutImpact);
    }

    //2) 아직 트리에 흡수되지 않은 대기열. 빠뜨리면 최근 스폰분이 조용히 누락된다
    for (UPrimitiveComponent* C : PendingObjects)
    {
        if (!C) { continue; }

        const FAxisAlignedBoundingBox Local = C->GetLocalBounds();
        if (!Local.IsValid()) { continue; }

        //대기열은 바운드 캐시가 없으므로 즉석 계산
        const FAxisAlignedBoundingBox World(Local, C->GetGlobalTransformMatrix());
        TestObjectRay(C, World, Ray, InvDir, Closest, OutHit, OutImpact);
    }

    return OutHit != nullptr;
}

void FSceneBVH::TraverseRay(uint32 RootIdx, const FRay& Ray, const FVector& InvDir, float& Closest, UPrimitiveComponent*& OutHit, FVector& OutImpact) const
{
    //재귀 대신 고정 크기 스택으로 순회한다. 노드와 진입 거리를 함께 쌓아,
    //꺼낼 때 그사이 줄어든 Closest로 다시 가지친다.
    //중앙값 분할로 빌드해 깊이가 log2(개수) 이하이므로 64칸이면 넘치지 않는다.
    struct FStackEntry { uint32 Node; float TNear; };
    FStackEntry Stack[64];
    int32 Sp = 0;
    Stack[Sp++] = { RootIdx, 0.0f };

    const FMathSSE::VectorRegister4Float rayOx = FMathSSE::VectorSetFloat1(Ray.Origin.X);
    const FMathSSE::VectorRegister4Float rayOy = FMathSSE::VectorSetFloat1(Ray.Origin.Y);
    const FMathSSE::VectorRegister4Float rayOz = FMathSSE::VectorSetFloat1(Ray.Origin.Z);

    const FMathSSE::VectorRegister4Float rayIdx = FMathSSE::VectorSetFloat1(InvDir.X);
    const FMathSSE::VectorRegister4Float rayIdy = FMathSSE::VectorSetFloat1(InvDir.Y);
    const FMathSSE::VectorRegister4Float rayIdz = FMathSSE::VectorSetFloat1(InvDir.Z);

    while (Sp > 0)
    {
        const FStackEntry Entry = Stack[--Sp];
        if (Entry.TNear >= Closest) { continue; }

        const FSceneBVHNode& N = Nodes[Entry.Node];

        //삭제로 비어버린 가지
        if (!N.Bounds.IsValid()) { continue; }

        if (N.bLeafNode)
        {
            TestLeafRay(N, Ray, InvDir, Closest, OutHit, OutImpact);
            continue;
        }

        const FMathSSE::VectorRegister4Float bCx = FMathSSE::VectorLoadAligned(N.ChildCenterX);
        const FMathSSE::VectorRegister4Float bCy = FMathSSE::VectorLoadAligned(N.ChildCenterY);
        const FMathSSE::VectorRegister4Float bCz = FMathSSE::VectorLoadAligned(N.ChildCenterZ);
        const FMathSSE::VectorRegister4Float bEx = FMathSSE::VectorLoadAligned(N.ChildExtentX);
        const FMathSSE::VectorRegister4Float bEy = FMathSSE::VectorLoadAligned(N.ChildExtentY);
        const FMathSSE::VectorRegister4Float bEz = FMathSSE::VectorLoadAligned(N.ChildExtentZ);

        const FMathSSE::VectorRegister4Float minX = FMathSSE::VectorSub(bCx, bEx);
        const FMathSSE::VectorRegister4Float maxX = FMathSSE::VectorAdd(bCx, bEx);
        const FMathSSE::VectorRegister4Float minY = FMathSSE::VectorSub(bCy, bEy);
        const FMathSSE::VectorRegister4Float maxY = FMathSSE::VectorAdd(bCy, bEy);
        const FMathSSE::VectorRegister4Float minZ = FMathSSE::VectorSub(bCz, bEz);
        const FMathSSE::VectorRegister4Float maxZ = FMathSSE::VectorAdd(bCz, bEz);

        const FMathSSE::VectorRegister4Float t0x = FMathSSE::VectorMul(FMathSSE::VectorSub(minX, rayOx), rayIdx);
        const FMathSSE::VectorRegister4Float t1x = FMathSSE::VectorMul(FMathSSE::VectorSub(maxX, rayOx), rayIdx);
        const FMathSSE::VectorRegister4Float tminX = FMathSSE::VectorMin(t0x, t1x);
        const FMathSSE::VectorRegister4Float tmaxX = FMathSSE::VectorMax(t0x, t1x);

        const FMathSSE::VectorRegister4Float t0y = FMathSSE::VectorMul(FMathSSE::VectorSub(minY, rayOy), rayIdy);
        const FMathSSE::VectorRegister4Float t1y = FMathSSE::VectorMul(FMathSSE::VectorSub(maxY, rayOy), rayIdy);
        const FMathSSE::VectorRegister4Float tminY = FMathSSE::VectorMin(t0y, t1y);
        const FMathSSE::VectorRegister4Float tmaxY = FMathSSE::VectorMax(t0y, t1y);

        const FMathSSE::VectorRegister4Float t0z = FMathSSE::VectorMul(FMathSSE::VectorSub(minZ, rayOz), rayIdz);
        const FMathSSE::VectorRegister4Float t1z = FMathSSE::VectorMul(FMathSSE::VectorSub(maxZ, rayOz), rayIdz);
        const FMathSSE::VectorRegister4Float tminZ = FMathSSE::VectorMin(t0z, t1z);
        const FMathSSE::VectorRegister4Float tmaxZ = FMathSSE::VectorMax(t0z, t1z);

        const FMathSSE::VectorRegister4Float tNear = FMathSSE::VectorMax(FMathSSE::VectorMax(tminX, tminY), FMathSSE::VectorMax(tminZ, FMathSSE::VectorZero()));
        const FMathSSE::VectorRegister4Float tFar = FMathSSE::VectorMin(FMathSSE::VectorMin(tmaxX, tmaxY), FMathSSE::VectorMin(tmaxZ, FMathSSE::VectorSetFloat1(Closest)));

        const FMathSSE::VectorRegister4Float hitMask = FMathSSE::VectorCompareLE(tNear, tFar);
        int mask = FMathSSE::VectorMaskBits(hitMask) & ((1 << N.ChildCount) - 1);

        if (mask == 0) { continue; }

        alignas(16) float tNearArr[4];
        FMathSSE::VectorStoreAligned(tNear, tNearArr);

        struct FChildHit { uint32 Node; float TNear; };
        FChildHit Hits[4];
        uint32 HitCount = 0;

        for (uint8 c = 0; c < N.ChildCount; ++c)
        {
            if (mask & (1 << c))
            {
                Hits[HitCount++] = { N.Children[c], tNearArr[c] };
            }
        }

        //먼 쪽을 먼저 넣어야 가까운 쪽이 먼저 나온다
        for (uint32 i = 0; i < HitCount; ++i)
        {
            for (uint32 j = i + 1; j < HitCount; ++j)
            {
                if (Hits[i].TNear < Hits[j].TNear)
                {
                    std::swap(Hits[i], Hits[j]);
                }
            }
        }

        for (uint32 i = 0; i < HitCount; ++i)
        {
            Stack[Sp++] = { Hits[i].Node, Hits[i].TNear };
        }
    }
}

void FSceneBVH::TestLeafRay(const FSceneBVHNode& N, const FRay& Ray, const FVector& InvDir, float& Closest, UPrimitiveComponent*& OutHit, FVector& OutImpact) const
{
    //박스에 맞은 오브젝트만 진입 거리(tNear) 순으로 모은 뒤 가까운 것부터 메시를 검사한다.
    //인덱스 순서로 검사하면 뒤쪽 오브젝트의 메시를 먼저 끝까지 도는 낭비가 생긴다.
    struct FCandidate { float TNear; uint32 Index; };
    constexpr uint32 MaxCandidates = 64;
    FCandidate Cands[MaxCandidates];
    uint32 NumCands = 0;

    for (uint32 i = N.ObjStart; i < N.ObjStart + N.ObjCount; ++i)
    {
        //FSceneBVH::RemoveObject에서 삭제된 UPrimComp는 nullptr로 되어있다
        //Buil되기 전에는 빈 공간을 남아있으므로 Ray 검사중엔 건너뛴다.
        if (!Objects[i]) { continue; }

        float tNear = 0.0f;
        if (!FRayCastingManager::RayIntersectsBoundsInv(Ray.Origin, InvDir, ObjectBounds[i].Min, ObjectBounds[i].Max, tNear)) { continue; }
        if (tNear >= Closest) { continue; }

        //후보가 넘치면(퇴화 리프 등) 정렬 없이 바로 검사한다
        if (NumCands == MaxCandidates)
        {
            TestObjectMesh(Objects[i], Ray, Closest, OutHit, OutImpact);
            continue;
        }

        //삽입 정렬: 후보는 보통 1~3개라 std::sort보다 직접 넣는 쪽이 싸다
        uint32 Pos = NumCands++;
        while (Pos > 0 && Cands[Pos - 1].TNear > tNear)
        {
            Cands[Pos] = Cands[Pos - 1];
            --Pos;
        }
        Cands[Pos] = { tNear, i };
    }

    for (uint32 c = 0; c < NumCands; ++c)
    {
        //정렬돼 있으므로 이 후보가 이미 찾은 교차보다 멀면 나머지도 전부 멀다
        if (Cands[c].TNear >= Closest) { break; }
        TestObjectMesh(Objects[Cands[c].Index], Ray, Closest, OutHit, OutImpact);
    }
}

void FSceneBVH::TraverseFrustum(uint32 NodeIdx, const FFrustum& Frustum, const FVector(&AbsNormals)[FFrustum::PlaneCount], TArray<UPrimitiveComponent*>& OutVisible) const
{
    const FSceneBVHNode& N = Nodes[NodeIdx];

    if (!N.Bounds.IsValid()) { return; }

    // 현재 노드가 절두체 밖에 있으면 하위 자식 검사 생략
    if (!FrustumUtils::IsVisible(Frustum, AbsNormals, N.Bounds))
    {
        return;
    }

    // 리프 노드인 경우 소속 오브젝트들을 순회하며 검사
    if (N.bLeafNode)
    {
        for (uint32 i = N.ObjStart; i < N.ObjStart + N.ObjCount; ++i)
        {
            UPrimitiveComponent* C = Objects[i];
            if (!C) { continue; }

            if (FrustumUtils::IsVisible(Frustum, AbsNormals, ObjectBounds[i]))
            {
                OutVisible.push_back(C);
            }
        }
        return;
    }

    // 내부 노드: 유효한 자식들(최대 4개)에 대해 재귀 순회
    for (uint8 c = 0; c < N.ChildCount; ++c)
    {
        const uint32 ChildIdx = N.Children[c];
        if (ChildIdx != UINT32_MAX)
        {
            TraverseFrustum(ChildIdx, Frustum, AbsNormals, OutVisible);
        }
    }
}

//AABB -> 뮐러 트럼보어
void FSceneBVH::TestObjectRay(UPrimitiveComponent* C, const FAxisAlignedBoundingBox& WorldBox, const FRay& Ray, const FVector& InvDir, float& Closest, UPrimitiveComponent*& OutHit, FVector& OutImpact) const
{
    float tNear = 0.0f;
    if (!FRayCastingManager::RayIntersectsBoundsInv(Ray.Origin, InvDir, WorldBox.Min, WorldBox.Max, tNear)) { return; }
    if (tNear >= Closest) { return; }           //이미 더 가까운 히트가 있으면 삼각형 검사 생략

    TestObjectMesh(C, Ray, Closest, OutHit, OutImpact);
}

//월드 AABB를 통과한 오브젝트의 메시(삼각형)를 검사한다
void FSceneBVH::TestObjectMesh(UPrimitiveComponent* C, const FRay& Ray, float& Closest, UPrimitiveComponent*& OutHit, FVector& OutImpact) const
{
    const UStaticMesh* Asset = C->GetMeshAsset();
    const FMesh* Mesh = Asset ? Asset->Get() : nullptr;
    if (!Mesh) { return; }

    float Dist = 0.0f;
    FVector Impact{};

    //역행렬이 없으면(스케일 0 등) 로컬 공간으로 옮길 수 없으니 맞지 않은 것으로 본다
    const FMatrix* InvWorld = C->GetGlobalInverseMatrix();
    if (!InvWorld) { return; }

    if (FRayCastingManager::RayIntersectsMeshWithInversedModel(Ray, *Mesh, *InvWorld, Dist, Impact, Closest, true))
    {
        OutHit = C;
        OutImpact = Impact;
    }
}

void FSceneBVH::AddObject(UPrimitiveComponent* C)
{
    if (!C) { return; }
    if (C->GetBVHIndex() >= 0) { return; }

    //대기열에 이미 있으면 중복 추가 방지(O(n)이지만 길지 않으므로 괜찮)
    if (std::find(PendingObjects.begin(), PendingObjects.end(), C) != PendingObjects.end()) { return; }

    PendingObjects.push_back(C);
}

void FSceneBVH::RemoveObject(UPrimitiveComponent* C)
{
    if (!C) { return; }

    const int32 ObjectIndex = C->GetBVHIndex();      // Index → ObjectIndex

    if (ObjectIndex < 0)
    {
        //트리에 없으면 대기열 소속. 없으면 아무 일도 안 일어남
        std::erase(PendingObjects, C);
        return;
    }

    if (static_cast<uint32>(ObjectIndex) >= Objects.size()) { return; }

    Objects[ObjectIndex] = nullptr;                       // 댕글링 포인터 차단
    ObjectBounds[ObjectIndex] = FAxisAlignedBoundingBox{};     // 뒤집힌 빈 박스 = Union의 항등원
    C->SetBVHIndex(-1);
    ++RemovedCount;

    RefitFromLeaf(LeafOfObject[ObjectIndex]);                  // 박스가 자연스럽게 줄어듦
}