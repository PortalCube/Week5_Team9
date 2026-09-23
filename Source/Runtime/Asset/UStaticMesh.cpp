#include "UStaticMesh.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(UStaticMesh, UAsset)

void UStaticMesh::Load(UStaticMeshDesc& Desc)
{
	LoadInternal(Desc);
	Mesh = Desc.Mesh;
}
