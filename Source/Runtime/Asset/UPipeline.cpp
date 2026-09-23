#include "UPipeline.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(UPipeline, UAsset)

void UPipeline::Load(UPipelineDesc& Desc)
{
	LoadInternal(Desc);
	Pipeline = Desc.Pipeline;
}
