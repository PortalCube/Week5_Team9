#include "FMesh.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <algorithm>
#include <limits>

// 메시 BVH 빌드(Binned SAH)에서만 쓰는 도우미. 이 파일 밖으로 이름이 새지 않게 한다.
namespace
{
constexpr int BinCount = 12;

// SAH 비용 상수. 노드 하나를 더 방문하는 비용(자식 AABB 검사)과 삼각형 하나를 검사하는 비용의 비율.
// TraversalCost가 없으면 분할 비용이 항상 리프 비용 이하가 되어 SAH가 리프에서 멈추지 못한다.
constexpr float TraversalCost = 1.0f;
constexpr float IntersectCost = 1.0f;

struct FBin
{
	FVector Min{ std::numeric_limits<float>::max(),
				 std::numeric_limits<float>::max(),
				 std::numeric_limits<float>::max() };
	FVector Max{ std::numeric_limits<float>::lowest(),
				 std::numeric_limits<float>::lowest(),
				 std::numeric_limits<float>::lowest() };
	uint32 Count = 0;
};

// 박스 표면적. 빈 박스(Min > Max)는 0으로 본다.
float SurfaceArea(const FVector& Min, const FVector& Max)
{
	const FVector D = Max - Min;
	if (D.X < 0.0f || D.Y < 0.0f || D.Z < 0.0f) { return 0.0f; }
	return 2.0f * (D.X * D.Y + D.Y * D.Z + D.Z * D.X);
}

// 박스 A를 B까지 포함하도록 키운다.
void GrowBox(FVector& AMin, FVector& AMax, const FVector& BMin, const FVector& BMax)
{
	for (int a = 0; a < 3; ++a)
	{
		AMin[a] = std::min(AMin[a], BMin[a]);
		AMax[a] = std::max(AMax[a], BMax[a]);
	}
}
} // namespace

FMesh::~FMesh()
{
	if (VertexBuffer)
	{
		D3D11_BUFFER_DESC Desc{};
		VertexBuffer->GetDesc(&Desc);
	}

	if (IndexBuffer)
	{
		D3D11_BUFFER_DESC Desc{};
		IndexBuffer->GetDesc(&Desc);
	}
}

void FMesh::BindResources(ID3D11DeviceContext& Context) const
{
	constexpr UINT Offset = 0;

	Context.IASetPrimitiveTopology(Topology);
	Context.IASetVertexBuffers(0, 1, VertexBuffer.GetAddressOf(), &VertexStride, &Offset);
	Context.IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}

void FMesh::BuildTriangleVertices()
{
	TriangleVertices.clear();
	if (!Indices.empty())
	{
		size_t PositionSize = Positions.size();
		TriangleVertices.reserve(Indices.size());
		for (size_t i = 0; i + 2 < Indices.size(); i += 3)
		{
			if (Indices[i] < PositionSize && Indices[i+1] < PositionSize && Indices[i+2] < PositionSize)
			{
				TriangleVertices.push_back(Positions[Indices[i]]);
				TriangleVertices.push_back(Positions[Indices[i+1]]);
				TriangleVertices.push_back(Positions[Indices[i+2]]);
			}
		}
	}
	else
	{
		TriangleVertices = Positions;
	}

	//BVH Build
	TArray<FTriRef> Tris; BVHNodes.clear();
	Tris.reserve(TriangleVertices.size() / 3);

	for (size_t i = 0; i + 2 < TriangleVertices.size(); i += 3)
	{
		const FVector& A = TriangleVertices[i];
		const FVector& B = TriangleVertices[i + 1];
		const FVector& C = TriangleVertices[i + 2];

		FVector Min, Max;
		for (int a = 0; a < 3; ++a)
		{
			Min[a] = std::min({ A[a], B[a], C[a] });
			Max[a] = std::max({ A[a], B[a], C[a] });
		}

		Tris.emplace_back(Min, Max, (Min + Max) / 2, static_cast<uint32>(i / 3));
	}

	// 삼각형이 없으면 트리를 만들지 않는다. 빈 루트는 내부 노드로 오인될 수 있다.
	if (Tris.empty()) { return; }

	BVHNodes.reserve(Tris.size() * 2 / LeafSize);
	BVHNodes.push_back({});
	BuildRecursive(0, 0, (uint32)Tris.size(), Tris, 0);

	TArray<FVector> Reordered;
	Reordered.resize(Tris.size() * 3);

	for (size_t k = 0; k < Tris.size(); ++k)
	{
		const size_t Src = static_cast<size_t>(Tris[k].TriIndex) * 3;
		const size_t Dst = k * 3;

		Reordered[Dst] = TriangleVertices[Src];
		Reordered[Dst + 1] = TriangleVertices[Src + 1];
		Reordered[Dst + 2] = TriangleVertices[Src + 2];
	}

	TriangleVertices = std::move(Reordered);
}

void FMesh::BuildRecursive(uint32 NodeIdx, uint32 Start, uint32 Count, TArray<FTriRef> &Tris, uint32 Depth)
{
	FAxisAlignedBoundingBox Bounds;
	FAxisAlignedBoundingBox CentroidBounds;

	for (uint32 i = Start; i < Start + Count; ++i)
	{
		const FTriRef& P = Tris[i];
		for (int a = 0; a < 3; ++a)
		{
			Bounds.Min[a] = std::min(Bounds.Min[a], P.Min[a]);
			Bounds.Max[a] = std::max(Bounds.Max[a], P.Max[a]);
			CentroidBounds.Min[a] = std::min(CentroidBounds.Min[a], P.Centroid[a]);
			CentroidBounds.Max[a] = std::max(CentroidBounds.Max[a], P.Centroid[a]);
		}
	}

	BVHNodes[NodeIdx].BoundsMax = Bounds.Max;
	BVHNodes[NodeIdx].BoundsMin = Bounds.Min;

	// 중심점 좌표를 bin 번호로 바꾼다. 채우기와 분할에서 반드시 같은 식을 써야
	// 경계에 걸린 삼각형이 계산한 비용과 다른 쪽으로 가지 않는다.
	auto BinIndex = [&CentroidBounds](int Axis, float Centroid)
	{
		const float CMin = CentroidBounds.Min[Axis];
		const float Scale = BinCount / (CentroidBounds.Max[Axis] - CMin);
		return std::min(BinCount - 1, static_cast<int>((Centroid - CMin) * Scale));
	};

	// 세 축의 bin 경계마다 SAH 비용을 계산해 가장 싼 분할을 찾는다.
	// 분할 비용 = TraversalCost x 부모 표면적 + IntersectCost x (왼쪽 표면적 x 왼쪽 개수 + 오른쪽 표면적 x 오른쪽 개수)
	// (원래 식을 부모 표면적으로 나누지 않은 형태. 리프 비용도 같은 배율로 계산하므로 비교 결과는 같다.)
	const float ParentArea = SurfaceArea(Bounds.Min, Bounds.Max);
	int BestAxis = -1;
	int BestSplit = -1;          // 이 bin까지가 왼쪽
	float BestCost = (std::numeric_limits<float>::max)();

	for (int Axis = 0; Axis < 3; ++Axis)
	{
		if (CentroidBounds.Max[Axis] - CentroidBounds.Min[Axis] < 1e-6f) { continue; }   // 이 축으로는 못 나눔

		FBin Bins[BinCount];
		for (uint32 i = Start; i < Start + Count; ++i)
		{
			const FTriRef& T = Tris[i];
			FBin& B = Bins[BinIndex(Axis, T.Centroid[Axis])];
			GrowBox(B.Min, B.Max, T.Min, T.Max);
			++B.Count;
		}

		// 왼쪽에서 누적: 경계 i의 왼쪽 = bin 0..i
		float LeftArea[BinCount - 1];
		uint32 LeftCount[BinCount - 1];
		FBin Acc;
		for (int i = 0; i < BinCount - 1; ++i)
		{
			GrowBox(Acc.Min, Acc.Max, Bins[i].Min, Bins[i].Max);
			Acc.Count += Bins[i].Count;
			LeftArea[i] = SurfaceArea(Acc.Min, Acc.Max);
			LeftCount[i] = Acc.Count;
		}

		// 오른쪽에서 누적하며 비용 계산: 경계 i-1의 오른쪽 = bin i..끝
		Acc = FBin{};
		for (int i = BinCount - 1; i > 0; --i)
		{
			GrowBox(Acc.Min, Acc.Max, Bins[i].Min, Bins[i].Max);
			Acc.Count += Bins[i].Count;

			const int Split = i - 1;
			if (LeftCount[Split] == 0 || Acc.Count == 0) { continue; }   // 한쪽이 비면 후보가 아니다

			const float Cost = TraversalCost * ParentArea
				+ IntersectCost * (LeftArea[Split] * LeftCount[Split] + SurfaceArea(Acc.Min, Acc.Max) * Acc.Count);
			if (Cost < BestCost)
			{
				BestCost = Cost;
				BestAxis = Axis;
				BestSplit = Split;
			}
		}
	}

	// 리프 판정
	// - LeafSize는 "SAH가 리프를 원해도 이보다 크면 쪼갠다"는 상한이다.
	// - 순회 스택(64칸)을 넘지 않도록 깊이가 너무 깊으면 리프로 끝낸다.
	// 리프 비용 = IntersectCost x 부모 표면적 x 개수 (나누지 않고 전부 검사)
	const float LeafCost = IntersectCost * ParentArea * Count;
	const bool bNoSplit = (BestAxis < 0);                        // 세 축 모두 중심점 범위가 0
	const bool bLeafIsCheaper = (BestCost >= LeafCost && Count <= LeafSize);
	constexpr uint32 MaxDepth = 60;

	if (Count <= 2 || bLeafIsCheaper || (bNoSplit && Count <= LeafSize * 4) || Depth >= MaxDepth)
	{
		BVHNodes[NodeIdx].LeftOrFirst = Start;
		BVHNodes[NodeIdx].TriCount = Count;
		return;
	}

	uint32 Mid = Start;
	if (!bNoSplit)
	{
		auto It = std::partition(
			Tris.begin() + Start,
			Tris.begin() + Start + Count,
			[&](const FTriRef& T) { return BinIndex(BestAxis, T.Centroid[BestAxis]) <= BestSplit; });
		Mid = static_cast<uint32>(It - Tris.begin());
	}

	// 나눌 축이 없거나 한쪽이 비면 중앙값 분할로 대신한다. 무한 재귀를 막는다.
	if (Mid == Start || Mid == Start + Count)
	{
		const FVector Extent = CentroidBounds.Max - CentroidBounds.Min;
		int Axis = 0;
		if (Extent.Y > Extent[Axis]) Axis = 1;
		if (Extent.Z > Extent[Axis]) Axis = 2;

		Mid = Start + Count / 2;
		std::nth_element(
			Tris.begin() + Start,
			Tris.begin() + Mid,
			Tris.begin() + Start + Count,
			[Axis](const FTriRef& A, const FTriRef& B) { return A.Centroid[Axis] < B.Centroid[Axis]; });
	}

	const uint32 LeftIdx = (uint32)BVHNodes.size();
	BVHNodes.push_back({});
	BVHNodes.push_back({});

	//리프 노드가 아니면 TriCount = 0
	BVHNodes[NodeIdx].LeftOrFirst = LeftIdx;
	BVHNodes[NodeIdx].TriCount = 0;

	BuildRecursive(LeftIdx, Start, Mid - Start, Tris, Depth + 1);
	BuildRecursive(LeftIdx + 1, Mid, Start + Count - Mid, Tris, Depth + 1);
}

bool FMesh::UpdateBuffers(ID3D11Device* Device, ID3D11DeviceContext* Context, const FMeshDesc& Desc)
{
	if (!Device || !Context || !Desc.VertexData || Desc.VertexCount == 0)
	{
		return false;
	}

	// 정점 버퍼 갱신
	if (VertexBuffer && Desc.VertexDataSize <= VertexBufferSize)
	{
		D3D11_MAPPED_SUBRESOURCE Mapped;
		HRESULT hr = Context->Map(VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
		if (SUCCEEDED(hr))
		{
			memcpy(Mapped.pData, Desc.VertexData, Desc.VertexDataSize);
			Context->Unmap(VertexBuffer.Get(), 0);
		}
	}
	else
	{
		Microsoft::WRL::ComPtr<ID3D11Buffer> NewVertexBuffer;

		D3D11_BUFFER_DESC VbDesc = {
			.ByteWidth = Desc.VertexDataSize,
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		};
		D3D11_SUBRESOURCE_DATA VData = { .pSysMem = Desc.VertexData };
		if (FAILED(Device->CreateBuffer(&VbDesc, &VData, &NewVertexBuffer)))
		{
			return false;
		}

		const size_t OldSize = VertexBufferSize;

		VertexBuffer = NewVertexBuffer;
		VertexBufferSize = Desc.VertexDataSize;
	}

	VertexCount = Desc.VertexCount;
	VertexStride = Desc.VertexStride;

	// 인덱스 버퍼 갱신
	if (Desc.IndexCount > 0 && Desc.IndexData)
	{
		if (IndexBuffer && Desc.IndexDataSize <= IndexBufferSize)
		{
			Context->UpdateSubresource(IndexBuffer.Get(), 0, nullptr, Desc.IndexData, 0, 0);
		}
		else
		{
			Microsoft::WRL::ComPtr<ID3D11Buffer> NewIndexBuffer;

			D3D11_BUFFER_DESC IbDesc = {
				.ByteWidth = Desc.IndexDataSize,
				.Usage = D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11_BIND_INDEX_BUFFER,
			};
			D3D11_SUBRESOURCE_DATA IData = { .pSysMem = Desc.IndexData };
			if (FAILED(Device->CreateBuffer(&IbDesc, &IData, &NewIndexBuffer)))
			{
				return false;
			}

			const size_t OldSize = IndexBufferSize;

			IndexBuffer = NewIndexBuffer;
			IndexBufferSize = Desc.IndexDataSize;
		}
		IndexCount = Desc.IndexCount;
	}
	else
	{
		IndexCount = 0;
	}

	// 위치와 인덱스 복사
	Positions.clear();
	Positions.reserve(Desc.VertexCount);
	const auto* vertices = static_cast<const FVertexData*>(Desc.VertexData);
	for (uint32 i = 0; i < Desc.VertexCount; ++i)
	{
		Positions.push_back(FVector{ vertices[i].x, vertices[i].y, vertices[i].z });
	}

	Indices.clear();
	if (Desc.IndexCount > 0 && Desc.IndexData)
	{
		const auto* indices = static_cast<const uint32*>(Desc.IndexData);
		Indices.assign(indices, indices + Desc.IndexCount);
	}

	BuildTriangleVertices();

	// 바운딩 박스 갱신
	LocalBounds = FAxisAlignedBoundingBox{ *this };
	return true;
}


