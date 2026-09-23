#include "TestTextActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"

IMPLEMENT_UCLASS(ATestTextActor, AActor)
UCLASS_META(ATestTextActor, DisplayName, "Test Text Actor")

ATestTextActor::ATestTextActor()
{
	// 텍스트 인스턴스 컴포넌트 장착
	CreateRootComponent(UTextInstanceComponent::StaticClass());
}

UTextInstanceComponent* ATestTextActor::GetTextInstanceComponent() const
{
	return RootComponent ? RootComponent->Cast<UTextInstanceComponent>() : nullptr;
}
