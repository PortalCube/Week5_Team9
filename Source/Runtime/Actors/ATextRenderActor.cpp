#include "ATextRenderActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"

IMPLEMENT_UCLASS(ATextRenderActor, AActor)
UCLASS_META(ATextRenderActor, DisplayName, "TextRender Actor")

ATextRenderActor::ATextRenderActor()
{
	// 기본 큐브 컴포넌트 장착
	CreateRootComponent(UTextInstanceComponent::StaticClass());
}

UTextInstanceComponent* ATextRenderActor::GetTextComponent() const
{
	return RootComponent ? RootComponent->Cast<UTextInstanceComponent>() : nullptr;
}
