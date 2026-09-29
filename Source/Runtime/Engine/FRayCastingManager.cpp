#include "FRayCastingManager.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include <limits>
#include <cmath>
#include <algorithm>

constexpr float Epsilon = 0.000001f;

FRay FRayCastingManager::CreateRayFromScreenPosition(const FCamera& Camera, const FVector2& MousePosition, const FVector2& ViewportSize)
{
	float ViewportWidth = ViewportSize.X;
	float ViewportHeight = ViewportSize.Y;

	FMatrix InvVP;
	Camera.GetViewProjectionMatrix().Inverse(InvVP);

	const float screenNdcX = (MousePosition.X / ViewportWidth) * 2.0f - 1.0f;
	const float screenNdcY = 1.0f - (MousePosition.Y / ViewportHeight) * 2.0f;

	// 이 엔진의 투영 행렬:
	// X = depth, Y = screen horizontal, Z = screen vertical
	FVector NearClip{ 0.0f, screenNdcX, screenNdcY };
	FVector FarClip{ 1.0f, screenNdcX, screenNdcY };

	const FVector NearWorld = InvVP.TransformPointRow(NearClip);
	const FVector FarWorld = InvVP.TransformPointRow(FarClip);

	FRay Ray;
	Ray.Origin = NearWorld;
	FVector dir = FarWorld - NearWorld;
	Ray.Direction = dir / dir.Size();
	return Ray;
}


bool FRayCastingManager::RayIntersectsMeshes(
	const FRay& Ray,
	const FCamera& Camera,
	const TArray<UPrimitiveComponent*>& Components,
	UPrimitiveComponent*& HitComponent,
	FVector& OutImpactPoint)
{
	HitComponent = nullptr;

	float ClosestHit = (std::numeric_limits<float>::max)();
	UPrimitiveComponent* ClosestComponent = nullptr;
	FVector ClosestImpactPoint;

	for (UPrimitiveComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		const UStaticMesh* MeshAsset = Component->GetRenderData(Camera).Mesh;
		const FMesh* Mesh = MeshAsset ? MeshAsset->Get() : nullptr;
		if (!Mesh)
		{
			continue;
		}

		FMatrix World = Component->GetRenderMatrix(Camera);

		float HitDistance;
		FVector ImpactPoint;
		if (RayIntersectsMesh(Ray, *Mesh, World, HitDistance, ImpactPoint) &&
			HitDistance < ClosestHit)
		{
			ClosestHit = HitDistance;
			ClosestComponent = Component;
			ClosestImpactPoint = ImpactPoint;
		}
	}
	
	HitComponent = ClosestComponent;
	OutImpactPoint = ClosestImpactPoint;

	return ClosestComponent != nullptr;
}

bool FRayCastingManager::RayIntersectsAABB(const FRay& Ray, const FAxisAlignedBoundingBox& AABB, float& OutTNear)
{
	// AABB 판별
	// Source: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes//ray-box-intersection.html
	// Source: https://gist.github.com/DomNomNom/46bb1ce47f68d255fd5d

	FVector TMin;
	FVector TMax;
	for (int i = 0; i < 3; ++i)
	{
		if (std::abs(Ray.Direction[i]) < Epsilon)
		{
			if (Ray.Origin[i] < AABB.Min[i] || Ray.Origin[i] > AABB.Max[i])
			{
				return false;
			}
			else
			{
				//Orthographic모드 피킹버그 추가분
				TMin[i] = -std::numeric_limits<float>::infinity(); 
				TMax[i] = std::numeric_limits<float>::infinity(); 
				continue;
			}
		}
		else
		{
			TMin[i] = (AABB.Min[i] - Ray.Origin[i]) / Ray.Direction[i];
			TMax[i] = (AABB.Max[i] - Ray.Origin[i]) / Ray.Direction[i];
		}
	}

	float TNear = 0.0f;
	float TFar = std::numeric_limits<float>::max();
	for (int i = 0; i < 3; ++i)
	{
		TNear = std::max(TNear, std::min(TMin[i], TMax[i]));
		TFar = std::min(TFar, std::max(TMin[i], TMax[i]));
	}

	OutTNear = TNear;
	return TNear <= TFar;
}

// 펼친 삼각형 배열을 순서대로 검사한다.
static bool IntersectFlattenedTriangles(const FRay& ObjectRay, const FMesh& Mesh, float& OutClosestHit)
{
	const TArray<FVector>& Tri = Mesh.GetTriangleVertices();

	bool bHit = false;
	for (uint32 i = 0; i + 2 < Tri.size(); i += 3)
	{
		float HitT = 0.0f;
		if (FRayCastingManager::RayIntersectsTriangle(ObjectRay, Tri[i], Tri[i+1], Tri[i+2], HitT) &&
			HitT < OutClosestHit)
		{
			OutClosestHit = HitT;
			bHit = true;
		}
	}
	return bHit;
}

// 기존 방식: 매 삼각형마다 인덱스로 정점을 찾아간다.
static bool IntersectIndexedTriangles(const FRay& ObjectRay, const FMesh& Mesh, float& OutClosestHit)
{
	const auto& Positions = Mesh.GetPositions();
	const auto& Indices = Mesh.GetIndices();

	const uint32 elementCount = Mesh.HasIndices()
		? static_cast<uint32>(Indices.size())
		: static_cast<uint32>(Positions.size());

	bool bHit = false;
	for (uint32 i = 0; i + 2 < elementCount; i += 3)
	{
		const uint32 i0 = Mesh.HasIndices() ? Indices[i] : i;
		const uint32 i1 = Mesh.HasIndices() ? Indices[i + 1] : i + 1;
		const uint32 i2 = Mesh.HasIndices() ? Indices[i + 2] : i + 2;

		// 잘못된 인덱스 방어
		if (i0 >= Positions.size() ||
			i1 >= Positions.size() ||
			i2 >= Positions.size())
		{
			continue;
		}

		FVector A = Positions[i0];
		FVector B = Positions[i1];
		FVector C = Positions[i2];

		float HitT = 0.0f;
		if (FRayCastingManager::RayIntersectsTriangle(ObjectRay, A, B, C, HitT) &&
			HitT < OutClosestHit)
		{
			OutClosestHit = HitT;
			bHit = true;
		}
	}
	return bHit;
}

// 역방향 벡터를 쓰는 slab 검사. 나눗셈 대신 곱셈만 한다.
// 박스에 맞으면 진입 거리(음수면 0)를 OutTNear에 담는다.
static bool RayIntersectsBoundsInv(const FVector& Origin, const FVector& InvDir, const FVector& Min, const FVector& Max, float& OutTNear)
{
	float TNear = 0.0f;
	float TFar = (std::numeric_limits<float>::max)();
	for (int a = 0; a < 3; ++a)
	{
		const float T0 = (Min[a] - Origin[a]) * InvDir[a];
		const float T1 = (Max[a] - Origin[a]) * InvDir[a];
		TNear = std::max(TNear, std::min(T0, T1));
		TFar = std::min(TFar, std::max(T0, T1));
	}
	OutTNear = TNear;
	return TNear <= TFar;
}

// 메시 로컬 BVH를 가까운 노드부터 순회한다. ObjectRay는 메시 로컬 공간의 광선이다.
static bool IntersectMeshBVH(const FRay& ObjectRay, const FMesh& Mesh, float& OutClosestHit)
{
	const TArray<FMesh::FMeshBVHNode>& Nodes = Mesh.GetMeshBVHNodes();
	const FVector* Verts = Mesh.GetTriangleVertices().data();

	// 방향 성분이 0이면 1/0 = inf가 되고, 0 * inf = NaN이 slab 검사를 망가뜨린다.
	// 아주 작은 값으로 바꿔 유한한 큰 수가 나오게 한다.
	FVector InvDir;
	for (int a = 0; a < 3; ++a)
	{
		const float D = ObjectRay.Direction[a];
		InvDir[a] = 1.0f / (std::fabs(D) > 1e-12f ? D : std::copysign(1e-12f, D));
	}

	float RootNear = 0.0f;
	if (!RayIntersectsBoundsInv(ObjectRay.Origin, InvDir, Nodes[0].BoundsMin, Nodes[0].BoundsMax, RootNear))
	{
		return false;
	}

	// 노드 번호와 진입 거리를 같이 쌓아, 꺼낼 때 그사이 줄어든 Closest로 다시 가지친다.
	struct FStackEntry { uint32 Node; float TNear; };
	FStackEntry Stack[64];
	int32 Sp = 0;
	Stack[Sp++] = { 0, RootNear };

	bool bHit = false;
	while (Sp > 0)
	{
		const FStackEntry Entry = Stack[--Sp];
		if (Entry.TNear >= OutClosestHit) { continue; }

		const FMesh::FMeshBVHNode& N = Nodes[Entry.Node];

		// 리프: 이 노드에 속한 삼각형만 검사한다. LeftOrFirst는 삼각형 번호다.
		if (N.TriCount > 0)
		{
			const uint32 End = N.LeftOrFirst + N.TriCount;
			for (uint32 t = N.LeftOrFirst; t < End; ++t)
			{
				const FVector* V = Verts + static_cast<size_t>(t) * 3;
				float HitT = 0.0f;
				if (FRayCastingManager::RayIntersectsTriangle(ObjectRay, V[0], V[1], V[2], HitT) &&
					HitT < OutClosestHit)
				{
					OutClosestHit = HitT;
					bHit = true;
				}
			}
			continue;
		}

		// 내부 노드: 자식 둘은 항상 연속해 있다.
		const uint32 L = N.LeftOrFirst;
		const uint32 R = L + 1;

		float TL = 0.0f, TR = 0.0f;
		const bool bL = RayIntersectsBoundsInv(ObjectRay.Origin, InvDir, Nodes[L].BoundsMin, Nodes[L].BoundsMax, TL) && TL < OutClosestHit;
		const bool bR = RayIntersectsBoundsInv(ObjectRay.Origin, InvDir, Nodes[R].BoundsMin, Nodes[R].BoundsMax, TR) && TR < OutClosestHit;

		// 먼 쪽을 먼저 넣어야 가까운 쪽이 먼저 나온다.
		if (bL && bR)
		{
			if (TL <= TR)
			{
				Stack[Sp++] = { R, TR };
				Stack[Sp++] = { L, TL };
			}
			else
			{
				Stack[Sp++] = { L, TL };
				Stack[Sp++] = { R, TR };
			}
		}
		else if (bL) { Stack[Sp++] = { L, TL }; }
		else if (bR) { Stack[Sp++] = { R, TR }; }
	}
	return bHit;
}

bool FRayCastingManager::RayIntersectsMesh(const FRay& Ray, const FMesh& Mesh, const FMatrix& ModelMatrix, float& OutDistance, FVector& OutImpactPoint)
{
	const size_t VertexCount = bUseFlattenedTriangles
		? Mesh.GetTriangleVertices().size()
		: Mesh.GetPositions().size();
	if (VertexCount < 3)
	{
		return false;
	}

	// Ray를 Object 좌표계로 변환
	FMatrix InvM;
	if (!ModelMatrix.Inverse(InvM))
	{
		return false;
	}

	const FVector ObjectOrigin = InvM.TransformPointRow(Ray.Origin);
	const FVector ObjectDirection = InvM.TransformPointRow(Ray.Direction, 0.0f); // 1.0은 점을 나타내므로 0.0으로 하여 벡터로 유지
	const FRay ObjectRay{ ObjectOrigin, ObjectDirection };

	float DummyNear;
	FAxisAlignedBoundingBox AABB = Mesh.GetLocalBounds();
	if (!RayIntersectsAABB(ObjectRay, AABB, DummyNear))
	{
		return false;
	}

	float ClosestHit = (std::numeric_limits<float>::max)();
	bool bHit = false;
	if (bUseMeshBVH && !Mesh.GetMeshBVHNodes().empty())
	{
		bHit = IntersectMeshBVH(ObjectRay, Mesh, ClosestHit);
	}
	else
	{
		bHit = bUseFlattenedTriangles
			? IntersectFlattenedTriangles(ObjectRay, Mesh, ClosestHit)
			: IntersectIndexedTriangles(ObjectRay, Mesh, ClosestHit);
	}

	OutDistance = ClosestHit;
	if (bHit)
	{
		OutImpactPoint = Ray.Origin + Ray.Direction * ClosestHit;
	}

	return bHit;
}

bool FRayCastingManager::RayIntersectsTriangle(
	const FRay& Ray,
	const FVector& A,
	const FVector& B,
	const FVector& C,
	float& OutT)
{

	const FVector edge1 = B - A;
	const FVector edge2 = C - A;

	// Ray direction × triangle edge
	const FVector pVector = Ray.Direction.Cross(edge2);
	const float determinant = edge1.Dot(pVector);

	// 레이와 삼각형 평면이 평행함
	if (std::fabs(determinant) < Epsilon)
	{
		return false;
	}

	const float inverseDeterminant = 1.0f / determinant;

	// Barycentric u 계산
	const FVector tVector = Ray.Origin - A;
	const float u = tVector.Dot(pVector) * inverseDeterminant;

	if (u < 0.0f || u > 1.0f)
	{
		return false;
	}

	// Barycentric v 계산
	const FVector qVector = tVector.Cross(edge1);
	const float v = Ray.Direction.Dot(qVector) * inverseDeterminant;

	if (v < 0.0f || u + v > 1.0f)
	{
		return false;
	}

	// 레이 시작점으로부터 교점까지의 거리
	OutT = edge2.Dot(qVector) * inverseDeterminant;

	// t가 음수면 카메라/레이 시작점 뒤에 있는 삼각형
	return OutT > Epsilon;
}
