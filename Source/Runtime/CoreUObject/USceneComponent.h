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
    
    void SetInheritRotation(bool bInherit) { bInheritRotation = bInherit; bGlobalDirty = true; }
protected:
	USceneComponent() = default;

	FTransform RelativeTransform;

    //파생 클래스에서 Transfrom 변경에 따라 반응.
    virtual void OnTransformChanged() {}

public:
	const FTransform& GetRelativeTransform() const { return RelativeTransform; }
	virtual void SetRelativeTransform(const FTransform& RelativeTransform);
	const FTransform& GetGlobalTransform() const;
	const FMatrix& GetGlobalTransformMatrix() const { return GetGlobalTransform().GetMatrix(); }
	//void SetRelativeTransformFromGlobal(const FTransform& GlobalTransform);

    //Transform이 바뀔 때 알림. 액터 전체 컴포넌트에 전파
    void MarkActorTransformDirty();

    virtual void SetRelativeLocation(const FVector& RelativeLocation);
    virtual void SetRelativeRotation(const FVector& RelativeRotation);
    virtual void SetRelativeRotation(const FQuaternion& RelativeRotation);
    virtual void SetRelativeScale(const FVector& RelativeScale);

    virtual const FVector& GetRelativeLocation() const;
    virtual const FQuaternion& GetRelativeRotation() const;
    virtual const FVector& GetRelativeScale() const;

    void SetBatchIndex(int32 Index) { BatchIndex = Index; }
    int32 GetBatchIndex() const { return BatchIndex; }

protected:
    AActor* ActorOwner = nullptr;
    USceneComponent* SceneOwner = nullptr;
    UScene* Scene = nullptr;
    bool bHasBegunPlay = false;
    bool bInheritRotation = true;

    int32 BatchIndex = -1;

private:
    USceneComponent* GetTransformParent() const;

    // 월드 Transform 캐시. 부모의 GlobalVersion이 바뀌면 자식도 자동으로 재계산된다.
    mutable FTransform CachedGlobal;
    mutable const USceneComponent* CachedParent = nullptr;
    mutable uint32 CachedParentVersion = 0;
    mutable uint32 GlobalVersion = 0;
    mutable bool bGlobalDirty = true;
};
