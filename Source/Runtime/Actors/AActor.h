#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include <type_traits>
#include <concepts>

class UScene;

class AActor : public UObject
{
	DECLARE_UCLASS(AActor, UObject)
	GENERATED_BODY()

	friend class UScene;

protected:
	USceneComponent* RootComponent = nullptr;
	TArray<USceneComponent*> AttachedComp;

	explicit AActor() = default;

	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;

public:
	void Initialize() override;
	void Release() override;
	UScene* GetOwner() const { return Owner; }

	void CreateRootComponent(UClass* ClassType);

	void SetRootComponent(USceneComponent* Component);
	USceneComponent* GetRootComponent() const { return RootComponent; }
	const TArray<USceneComponent*>& GetAttachedComponents() const { return AttachedComp; }


	FTransform GetTransform() const { return RootComponent ? RootComponent->GetRelativeTransform() : FTransform{}; }
	void SetTransform(const FTransform& NewTransform) { if (RootComponent) RootComponent->SetRelativeTransform(NewTransform); }

	void AddComponent(USceneComponent* Addcomp);
	virtual void Register(UScene& Scene);
	virtual void BeginPlay();
	virtual void Update(float DeltaTime);
	virtual void EndPlay();
	virtual void Unregister();

	[[nodiscard]] bool IsRegistered() const { return Owner != nullptr; }
	[[nodiscard]] bool HasBegunPlay() const { return bHasBegunPlay; }

	void Destroy();

private:
	UScene* Owner = nullptr; // SpawnActor될 때 설정됨
	bool bHasBegunPlay = false;
};
