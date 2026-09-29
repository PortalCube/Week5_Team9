#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/FUObjectArray.h"
#include "Runtime/Asset/UAsset.h"
#include "Runtime/Rendering/FMesh.h"

// 하나의 LOD 단계. 화면 점유율(바운딩 구 지름 / 화면 높이)이 ScreenSize보다 작아지면 이 LOD로 전환한다.
struct FStaticMeshLOD
{
	FMesh* Mesh = nullptr;
	float ScreenSize = 0.0f;
};

struct UStaticMeshDesc : UAssetDesc
{
	FMesh* Mesh = nullptr;

	// LOD1부터의 추가 LOD. ScreenSize 내림차순이어야 한다.
	TArray<FStaticMeshLOD> AdditionalLODs;
};

class UStaticMesh : public UAsset
{

	GENERATED_BODY()
	DECLARE_UCLASS(UStaticMesh, UAsset)

private:
	TArray<FStaticMeshLOD> LODs;

public:

	void Load(UStaticMeshDesc& Desc);

	// LOD를 지정하지 않으면 원본(LOD0)을 반환한다. 피킹, 바운드 계산은 LOD0를 사용한다.
	FMesh* Get(uint32 LODIndex = 0) const;
	uint32 GetLODCount() const { return static_cast<uint32>(LODs.size()); }
	const FStaticMeshLOD& GetLOD(uint32 LODIndex) const { return LODs[LODIndex]; }

	// 화면 점유율에 맞는 LOD 인덱스를 반환한다.
	uint32 SelectLOD(float ScreenSize) const;

	// 화면 점유율의 제곱으로 LOD를 고른다. 제곱근 없이 매 프레임 호출할 수 있다.
	uint32 SelectLODSquared(float ScreenSizeSq) const;

};
