#include "AActor.h"
#include "Runtime/Core/Log.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Engine/UScene.h"

IMPLEMENT_UCLASS(AActor, UObject)

void AActor::Initialize()
{
	Super::Initialize();
	Owner = nullptr;
	bHasBegunPlay = false;
}

void AActor::Release()
{
	UScene* RegisteredScene = Owner;
	if (bHasBegunPlay)
	{
		EndPlay();
	}

	if (Owner)
	{
		Unregister();
	}

	if (RegisteredScene)
	{
		RegisteredScene->RemoveActor(this);
	}

	while (!AttachedComp.empty())
	{
		USceneComponent* Component = AttachedComp.back();
		std::erase(AttachedComp, Component);

		if (RootComponent == Component)
		{
			RootComponent = nullptr;
		}

		DestroyObject(Component);
	}

	if (RootComponent)
	{
		USceneComponent* RemainingRoot = RootComponent;
		RootComponent = nullptr;
		DestroyObject(RemainingRoot);
	}

	Super::Release();
}

void AActor::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	if (RootComponent)
	{
		FArchive RootArchive{};
		RootComponent->Serialize(RootArchive);
		Archive.SetArchive("RootComponent", RootArchive);
	}
	else
	{
		Archive.SetNull("RootComponent");
	}
}

void AActor::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

	if (Archive.IsNull("RootComponent"))
	{
		if (RootComponent)
		{
			UE_LOG_WARN("[%s::Deserialize] RootComponent(%s)에 대한 직렬화 데이터가 "
				"누락되었습니다.",
				GetClass()->GetUClassName(),
				RootComponent->GetClass()->GetUClassName());
		}
		return;
	}

	FArchive RootComponentArchive = Archive.GetArchive("RootComponent");
	const FString& SavedTypeName = RootComponentArchive.GetString("Type");
	UClass* SavedClass = UClass::FindByName(SavedTypeName);

	if (SavedClass == nullptr)
	{
		UE_LOG_WARN("[%s::Deserialize] 알 수 없는 타입 %s",
			GetClass()->GetUClassName(), SavedTypeName);
		return;
	}

	if (RootComponent == nullptr)
	{
		CreateRootComponent(SavedClass);

		if (RootComponent == nullptr)
		{
			UE_LOG_WARN("[%s::Deserialize] RootComponent %s를 생성할 수 없습니다.",
				GetClass()->GetUClassName(), SavedTypeName);
			return;
		}
	}

	if (RootComponent->GetClass() != SavedClass)
	{
		UE_LOG_WARN("[%s::Deserialize] 기본 RootComponent (%s)와 저장된 타입 "
			"(%s)가 일치하지 않습니다.",
			GetClass()->GetUClassName(),
			RootComponent->GetClass()->GetUClassName(), SavedTypeName);
		return;
	}

	RootComponent->Deserialize(RootComponentArchive);
}

void AActor::CreateRootComponent(UClass* ClassType)
{
	if (RootComponent) { return; }

	UObject* Object = NewObject(ClassType);
	USceneComponent* Component = Object->Cast<USceneComponent>();

	if (!Component)
	{
		DestroyObject(Object);
		return;
	}

	SetRootComponent(Component);
}

void AActor::SetRootComponent(USceneComponent* Component)
{
	if (RootComponent)
	{
		throw EngineUtil::CreateError("이미 Root 컴포넌트가 있습니다.");
	}

	RootComponent = Component;

	// TODO ActorOwner를 이렇게 지정하면 안됨
	RootComponent->ActorOwner = this;
	RootComponent->SetupAttachment(nullptr);
	RootComponent->Initialize();
	AttachedComp.push_back(RootComponent);

	if (Owner)
	{
		RootComponent->Register(*Owner);
	}

	if (bHasBegunPlay)
	{
		RootComponent->BeginPlay();
	}
}

void AActor::AddComponent(USceneComponent* Addcomp)
{
	if (Addcomp == nullptr)
	{
		return;
	}

	if (RootComponent == nullptr)
	{
		RootComponent = Addcomp;
		Addcomp->SetupAttachment(nullptr);
	}

	else if (Addcomp->GetSceneOwner() == nullptr)
	{
		Addcomp->SetupAttachment(RootComponent);
	}

	Addcomp->ActorOwner = this;
	AttachedComp.push_back(Addcomp);
	Addcomp->Initialize();

	if (Owner)
	{
		Addcomp->Register(*Owner);
	}

	if (bHasBegunPlay)
	{
		Addcomp->BeginPlay();
	}
}

void AActor::Register(UScene& Scene)
{
	if (Owner == &Scene)
	{
		return;
	}

	if (Owner)
	{
		Unregister();
	}

	Owner = &Scene;
	for (USceneComponent* Component : AttachedComp)
	{
		if (Component)
		{
			Component->Register(Scene);
		}
	}
}

void AActor::BeginPlay() {
	if (!Owner || bHasBegunPlay)
	{
		return;
	}

	bHasBegunPlay = true;
	for (USceneComponent* Component : AttachedComp)
	{
		if (Component)
		{
			Component->BeginPlay();
		}
	}
}

void AActor::Update(float DeltaTime) {
	if (!bHasBegunPlay)
	{
		return;
	}

	for (USceneComponent* Component : AttachedComp)
	{
		if (Component)
		{
			Component->Update(DeltaTime);
		}
	}
}

void AActor::EndPlay() {
	if (!bHasBegunPlay)
	{
		return;
	}

	for (auto It = AttachedComp.rbegin(); It != AttachedComp.rend(); ++It)
	{
		if (*It)
		{
			(*It)->EndPlay();
		}
	}
	bHasBegunPlay = false;
}

void AActor::Unregister() {
	if (bHasBegunPlay)
	{
		EndPlay();
	}

	if (!Owner)
	{
		return;
	}

	for (auto It = AttachedComp.rbegin(); It != AttachedComp.rend(); ++It)
	{
		if (*It)
		{
			(*It)->Unregister();
		}
	}
	Owner = nullptr;
}

void AActor::Destroy() {
	if (Owner)
	{
		Owner->DestroyActor(this);
		return;
	}
	DestroyObject(this);
}
