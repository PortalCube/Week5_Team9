#include "FMesh.h"

#include <d3d11.h>
#include <wrl/client.h>

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
	BuildRecursive(0, 0, (uint32)Tris.size(), Tris);

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

void FMesh::BuildRecursive(uint32 NodeIdx, uint32 Start, uint32 Count, TArray<FTriRef> &Tris)
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

	const FVector Extent = CentroidBounds.Max - CentroidBounds.Min;
	int Axis = 0;
	if (Extent.Y > Extent[Axis]) Axis = 1;
	if (Extent.Z > Extent[Axis]) Axis = 2;

	const bool bDegenerate = Extent[Axis] < 1e-6f;
	if (Count <= LeafSize || (bDegenerate && Count <= LeafSize * 4))
	{
		BVHNodes[NodeIdx].LeftOrFirst = Start;
		BVHNodes[NodeIdx].TriCount = Count;
		return;
	}

	const uint32 Mid = Start + Count / 2;
	std::nth_element(
		Tris.begin() + Start,
		Tris.begin() + Mid,
		Tris.begin() + Start + Count,
		[Axis](const FTriRef& A, const FTriRef& B) { return A.Centroid[Axis] < B.Centroid[Axis];});

	const uint32 LeftIdx = (uint32)BVHNodes.size();
	BVHNodes.push_back({});
	BVHNodes.push_back({});

	//리프 노드가 아니면 TriCount = 0
	BVHNodes[NodeIdx].LeftOrFirst = LeftIdx;
	BVHNodes[NodeIdx].TriCount = 0;

	BuildRecursive(LeftIdx, Start, Mid - Start, Tris);
	BuildRecursive(LeftIdx + 1, Mid, Start + Count - Mid, Tris);
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


