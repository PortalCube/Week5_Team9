#pragma once

#include "Runtime/Geometry/FTransform.h"
#include "ThirdParty/Json/json.hpp"
#include "UObject.h"


class UScene;
class AActor;
class FArchive;

class USceneComponent : public UObject
{
	GENERATED_BODY()
	DECLARE_UCLASS(USceneComponent, UObject)
	friend class AActor;

public:
    virtual void Initialize() override;
    virtual void Release() override;
    
    AActor* GetActorOwner() const { return ActorOwner; }
    USceneComponent* GetSceneOwner() const { return SceneOwner; }
    void SetActorOwner(AActor* Owner) { ActorOwner = Owner; } //selectedacotor 한테 textcomponent 바로 붙여야해서 만듦

    virtual void Register(UScene& InScene);
    virtual void BeginPlay();
    virtual void Update(float DeltaTime) {}
    virtual void EndPlay();
    virtual void Unregister();

    void SetupAttachment(USceneComponent* InParent);

    [[nodiscard]] bool IsRegistered() const { return Scene != nullptr; }
    [[nodiscard]] bool HasBegunPlay() const { return bHasBegunPlay; }

	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;
    
    void SetInheritRotation(bool bInherit) { bInheritRotation = bInherit; }
protected:
	USceneComponent() = default;

	FTransform RelativeTransform;

public:
	FTransform& GetRelativeTransform() { return RelativeTransform; }
	const FTransform& GetRelativeTransform() const { return RelativeTransform; }
	virtual void SetRelativeTransform(const FTransform& RelativeTransform);
	FTransform GetGlobalTransform() const;
	//void SetRelativeTransformFromGlobal(const FTransform& GlobalTransform);

protected:
    AActor* ActorOwner = nullptr;
    USceneComponent* SceneOwner = nullptr;
    UScene* Scene = nullptr;
    bool bHasBegunPlay = false;
    bool bInheritRotation = true;
};
