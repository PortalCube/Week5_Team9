#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/FUObjectArray.h"
#include "Runtime/Asset/UAsset.h"
#include "Runtime/Rendering/FMesh.h"

struct UStaticMeshDesc : UAssetDesc
{
	FMesh* Mesh = nullptr;
};

class UStaticMesh : public UAsset
{

	GENERATED_BODY()
	DECLARE_UCLASS(UStaticMesh, UAsset)

private:

	FMesh* Mesh = nullptr;

public:

	void Load(UStaticMeshDesc& Desc);

	FMesh* Get() const { return Mesh; }

};
