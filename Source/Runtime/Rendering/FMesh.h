#pragma once

#include "Vertices.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/FName.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <limits>
#include "Runtime/Core/TArray.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"

class FRenderer;

struct FMeshSection
{
	FString SectionName;
	uint32 StartIndex = 0;
	uint32 IndexCount = 0;
};

class FMesh final
{
	friend class FRenderer;

	//AxisAlignedBox 사용할 필요 없이 Min, Max만 저장해서 사용한다.
	struct FTriRef
	{
		FVector Min, Max;
		FVector Centroid;
		uint32  TriIndex;
	};

public:

	struct FMeshBVHNode
	{
		FVector BoundsMin;
		FVector BoundsMax;
		uint32  LeftOrFirst;
		uint32  TriCount;
	};

	~FMesh();
	[[nodiscard]] bool HasIndices() const { return IndexCount > 0; }
	[[nodiscard]] uint32 GetVertexCount() const { return VertexCount; }
	[[nodiscard]] uint32 GetIndexCount() const { return IndexCount; }
	[[nodiscard]] const TArray<FVector>& GetPositions() const { return Positions; }
	[[nodiscard]] const TArray<uint32>& GetIndices() const { return Indices; }
	[[nodiscard]] const FAxisAlignedBoundingBox& GetLocalBounds() const { return LocalBounds; }
	[[nodiscard]] const TArray<FVector>& GetTriangleVertices() const { return TriangleVertices; }
	[[nodiscard]] const TArray<FMeshBVHNode>& GetMeshBVHNodes() const { return BVHNodes; }
	const uint32 GetSectionCount() const { return static_cast<uint32>(Sections.size()); }
	const TArray<FMeshSection>& GetSections() const { return Sections; }

	// 버퍼 데이터 갱신
	bool UpdateBuffers(ID3D11Device* Device, ID3D11DeviceContext* Context, const struct FMeshDesc& Desc);
	FName MeshId{"None"};

	uint32 GetBufferSize() { return VertexBufferSize + IndexBufferSize; }
private:
	void BindResources(ID3D11DeviceContext& Context) const;
	void BuildTriangleVertices();
	void BuildRecursive(uint32 NodeIdx, uint32 Start, uint32 Count, TArray<FTriRef>& Tris, uint32 Depth);

	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;
	uint32 VertexCount = 0u;
	uint32 VertexStride = 0u;
	uint32 VertexBufferSize = 0u;

	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer;
	uint32 IndexCount = 0u;
	uint32 IndexBufferSize = 0u;

	TArray<FVector> Positions;
	TArray<uint32> Indices;
	TArray<FMeshSection> Sections;
	TArray<FVector> TriangleVertices;   // 삼각형 순서대로 펼친 정점 (3개씩)

	//=================
	//Mesh BVH
	TArray<FMeshBVHNode> BVHNodes;
	uint32 LeafSize = 16;
	//=================

	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FAxisAlignedBoundingBox LocalBounds = {};
};

struct FMeshDesc
{
	const void* VertexData = nullptr;
	uint32 VertexDataSize = 0u;
	uint32 VertexStride = sizeof(FVertexData);
	uint32 VertexCount = 0u;

	const void* IndexData = nullptr;
	uint32 IndexDataSize = 0u;
	uint32 IndexCount = 0u;

	TArray<FMeshSection> Sections;

	bool bIsLine = false;

	// 피킹용 삼각형 배열과 BVH를 만들지 여부. 피킹에 쓰지 않는 LOD 메시는 끈다.
	bool bBuildBVH = true;
};
