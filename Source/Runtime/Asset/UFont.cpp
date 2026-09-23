#include "UFont.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(UFont, UAsset)

void UFont::Load(UFontDesc& Desc)
{
	LoadInternal(Desc);
	Texture = Desc.Texture;
	Font = Desc.Font;
}
