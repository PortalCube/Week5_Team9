#include "UMaterial.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(UMaterial, UAsset)

void UMaterial::Load(UMaterialDesc& Desc)
{
	LoadInternal(Desc);
	Pipeline = Desc.Pipeline;
	Texture = Desc.Texture;
	SamplerDesc = Desc.TextureSamplerDesc;
}
