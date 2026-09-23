#include "AAnimatedBillboardActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UAnimatedBillboardComp.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AAnimatedBillboardActor, AActor)
UCLASS_META(AAnimatedBillboardActor, DisplayName, "Animated Billboard Actor")

AAnimatedBillboardActor::AAnimatedBillboardActor()
{
	// 루트 컴포넌트 생성 및 장착
	CreateRootComponent(UAnimatedBillboardComp::StaticClass());
	UAnimatedBillboardComp* Comp = GetAnimatedBillboardComponent();
	if (Comp)
	{
		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		UTexture* ExplosionTexture = Registry.Get<UTexture>("Texture/Explosion.json");

		// 폭발 스프라이트 텍스처 지정
		Comp->SetTexture(ExplosionTexture);
		// 시트 분할 및 루프 재생 설정
		Comp->SetSpriteSheet(6, 6, 20.0f, 36);
		Comp->SetLooping(true);
		Comp->Play();
	}
}

UAnimatedBillboardComp* AAnimatedBillboardActor::GetAnimatedBillboardComponent() const
{
	return RootComponent ? RootComponent->Cast<UAnimatedBillboardComp>() : nullptr;
}
