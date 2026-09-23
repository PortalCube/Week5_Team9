#include "UTexture.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(UTexture, UAsset)

void UTexture::Load(UTextureDesc& Desc)
{
	LoadInternal(Desc);
	Texture = Desc.Texture;
}
