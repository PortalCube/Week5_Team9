#include "pch.h"
#include "FSceneBVH.h"

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

        const FAxisAlignedBoundingBox &Local = C->GetLocalBounds();

        //빈 박스는 BVH에서 제외한다.
        if (!Local.IsValid()) { continue; }

        const FMatrix World = C->GetGlobalTransform().GetMatrix();
        FAxisAlignedBoundingBox WorldBox(Local, World);

        //{AABB, 중심점, 컴포넌트}
        Prims.push_back({ WorldBox, (WorldBox.Min + WorldBox.Max) * 0.5f, C });
    }

    //단 한 개도 없으면 안만듬
    if (Prims.empty()) return;

    //BVH 최대 크기는 컴포넌트 수*2 를 넘지 않음(리프가 컴포넌트 수 + 부모 노드 수가 그보단 작음)
    Nodes.reserve(2 * Prims.size());
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

    const uint32 Mid = Start + Count / 2;
    std::nth_element(
        Prims.begin() + Start,
        Prims.begin() + Mid,
        Prims.begin() + Start + Count,
        [Axis](const FPrimRef& A, const FPrimRef& B) { return A.Centroid[Axis] < B.Centroid[Axis]; });

    const uint32 LeftIdx = (uint32)Nodes.size();
    Nodes.push_back({});
    Nodes.push_back({});

    //리프 노드가 아니면 ObjCount = 0
    Nodes[NodeIdx].Left = LeftIdx;
    Nodes[NodeIdx].ObjCount = 0;

    BuildRecursive(LeftIdx, Start, Mid - Start, NodeIdx);
    BuildRecursive(LeftIdx + 1, Mid, Start + Count - Mid, NodeIdx);
}

void FSceneBVH::RefitObject(UPrimitiveComponent* Moved)
{
    if (!Moved) return;

    const int32 ObjectIndex = Moved->GetBVHIndex();
    if (ObjectIndex < 0 || static_cast<uint32>(ObjectIndex) >= Objects.size()) return;

    const FAxisAlignedBoundingBox Local = Moved->GetLocalBounds();
    if (!Local.IsValid()) { return; }


    //변경된 Transform으로 AABB 다시 넣기
    ObjectBounds[ObjectIndex] = FAxisAlignedBoundingBox(Local, Moved->GetGlobalTransform().GetMatrix());

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

        const FAxisAlignedBoundingBox Merged =
            FAxisAlignedBoundingBox::Union(Nodes[N.Left].Bounds, Nodes[N.Left + 1].Bounds);

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

//void FSceneBVH::QueryFrustum(const FFrustum & Frustum, float MinScreenPixels, TArray<UPrimitiveComponent*>&OutVisible) const
//{
//}

bool FSceneBVH::QueryRay(const FRay &Ray, UPrimitiveComponent*& OutHit, FVector &OutImpact) const
{
    OutImpact = FVector{};
    float Closest = (std::numeric_limits<float>::max)();

    //1) 트리 순회
    if (!Nodes.empty())
    {
        TraverseRay(0, Ray, Closest, OutHit, OutImpact);
    }

    //2) 아직 트리에 흡수되지 않은 대기열. 빠뜨리면 최근 스폰분이 조용히 누락된다
    for (UPrimitiveComponent* C : PendingObjects)
    {
        if (!C) { continue; }

        const FAxisAlignedBoundingBox Local = C->GetLocalBounds();
        if (!Local.IsValid()) { continue; }

        //대기열은 바운드 캐시가 없으므로 즉석 계산
        const FAxisAlignedBoundingBox World(Local, C->GetGlobalTransform().GetMatrix());
        TestObjectRay(C, World, Ray, Closest, OutHit, OutImpact);
    }

    return OutHit != nullptr;
}

void FSceneBVH::TraverseRay(uint32 NodeIdx, const FRay& Ray, float& Closest, UPrimitiveComponent*& OutHit, FVector& OutImpact) const
{
    const FSceneBVHNode& N = Nodes[NodeIdx];

    //삭제로 비어버린 가지
    if (!N.Bounds.IsValid()) { return; }

    //리프노드이면
    if (N.bLeafNode)
    {
        for (uint32 i = N.ObjStart; i < N.ObjStart + N.ObjCount; ++i)
        {
            //FSceneBVH::RemoveObject에서 삭제된 UPrimComp는 nullptr로 되어있다
            //Buil되기 전에는 빈 공간을 남아있으므로 Ray 검사중엔 건너뛴다.
            if (!Objects[i]) { continue; }

            TestObjectRay(Objects[i], ObjectBounds[i], Ray, Closest, OutHit, OutImpact);
        }
        return;
    }

    //리프가 아니면, 왼쪽 자식 오른쪽 자식 순회 준비
    const uint32 L = N.Left;
    const uint32 R = N.Left + 1;

    float tL = 0.0f, tR = 0.0f;
    const bool bL = Nodes[L].Bounds.IsValid()
        && FRayCastingManager::RayIntersectsAABB(Ray, Nodes[L].Bounds, tL);
    const bool bR = Nodes[R].Bounds.IsValid()
        && FRayCastingManager::RayIntersectsAABB(Ray, Nodes[R].Bounds, tR);

    //둘 다 Ray가 맞았으면
    if (bL && bR)
    {
        //가까운 쪽부터 들어가야 Closest 가 일찍 작아진다
        uint32 Near = L, Far = R;
        float  tFar = tR;
        if (tR < tL) { Near = R; Far = L; tFar = tL; }

        TraverseRay(Near, Ray, Closest, OutHit, OutImpact);

        //먼 쪽 박스 진입점이 이미 찾은 히트보다 뒤면 서브트리 전체를 버린다
        if (tFar < Closest) { TraverseRay(Far, Ray, Closest, OutHit, OutImpact); }
    }

    //왼쪽 혹은 오른쪽만 맞았으면 안맞은 서브트리는 버린다.
    else if (bL) { if (tL < Closest) { TraverseRay(L, Ray, Closest, OutHit, OutImpact); } }
    else if (bR) { if (tR < Closest) { TraverseRay(R, Ray, Closest, OutHit, OutImpact); } }
}

//AABB -> 뮐러 트럼보어
void FSceneBVH::TestObjectRay(UPrimitiveComponent* C, const FAxisAlignedBoundingBox& WorldBox, const FRay& Ray, float& Closest, UPrimitiveComponent*& OutHit, FVector& OutImpact) const
{
    float tNear = 0.0f;
    if (!FRayCastingManager::RayIntersectsAABB(Ray, WorldBox, tNear)) { return; }
    if (tNear >= Closest) { return; }           //이미 더 가까운 히트가 있으면 삼각형 검사 생략

    const UStaticMesh* Asset = C->GetMeshAsset();
    const FMesh* Mesh = Asset ? Asset->Get() : nullptr;
    if (!Mesh) { return; }

    float Dist = 0.0f;
    FVector Impact{};
    if (FRayCastingManager::RayIntersectsMesh(Ray, *Mesh, C->GetGlobalTransform().GetMatrix(), Dist, Impact)
        && Dist < Closest)
    {
        Closest = Dist;
        OutHit = C;
        OutImpact = Impact;
    }
}

bool FSceneBVH::Validate() const
{
    for (uint32 n = 0; n < Nodes.size(); ++n)
    {
        const FSceneBVHNode& N = Nodes[n];

        if (N.ObjCount > 0) // 리프: 구간의 모든 박스를 품어야 함
        {
            for (uint32 i = N.ObjStart; i < N.ObjStart + N.ObjCount; ++i)
            {
                for (int a = 0; a < 3; ++a)
                {
                    if (ObjectBounds[i].Min[a] < N.Bounds.Min[a]) { return false; }
                    if (ObjectBounds[i].Max[a] > N.Bounds.Max[a]) { return false; }
                }
            }
        }
        else  // 내부: 자식 둘의 합집합과 같아야 함
        {
            const FAxisAlignedBoundingBox M = FAxisAlignedBoundingBox::Union(Nodes[N.Left].Bounds, Nodes[N.Left + 1].Bounds);
            if (!(M == N.Bounds)) { return false; }
            if (Nodes[N.Left].Parent != n || Nodes[N.Left + 1].Parent != n) { return false; }
        }
    }
    return true;
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
