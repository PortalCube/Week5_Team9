#include "UStaticMesh.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(UStaticMesh, UAsset)

void UStaticMesh::Load(UStaticMeshDesc& Desc)
{
	LoadInternal(Desc);

	LODs.clear();
	LODs.push_back({ .Mesh = Desc.Mesh, .ScreenSize = 0.0f });
	for (const FStaticMeshLOD& LOD : Desc.AdditionalLODs)
	{
		if (LOD.Mesh) { LODs.push_back(LOD); }
	}
}

FMesh* UStaticMesh::Get(uint32 LODIndex) const
{
	if (LODs.empty()) { return nullptr; }
	return LODs[std::min<size_t>(LODIndex, LODs.size() - 1)].Mesh;
}

uint32 UStaticMesh::SelectLOD(float ScreenSize) const
{
	return SelectLODSquared(ScreenSize * ScreenSize);
}

uint32 UStaticMesh::SelectLODSquared(float ScreenSizeSq) const
{
	// 가장 거친 LOD부터 보면서, 전환 기준보다 작게 보이는 첫 LOD를 고른다.
	// ScreenSize는 항상 0 이상이라 제곱끼리 비교해도 결과가 같다.
	for (size_t i = LODs.size(); i-- > 1;)
	{
		const float Threshold = LODs[i].ScreenSize;
		if (ScreenSizeSq < Threshold * Threshold) { return static_cast<uint32>(i); }
	}
	return 0;
}
