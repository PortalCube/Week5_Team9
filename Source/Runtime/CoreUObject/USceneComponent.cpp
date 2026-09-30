#include "UClass.h"
#include "USceneComponent.h"
#include "ThirdParty/Json/json.hpp"
#include "UObjectGlobals.h" 
#include "UPrimitiveComponent.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Engine/UScene.h"


IMPLEMENT_UCLASS(USceneComponent, UObject)

void USceneComponent::Initialize()
{
    Super::Initialize();
    Scene = nullptr;
    bHasBegunPlay = false;
}
void USceneComponent::Release()
{
    if (bHasBegunPlay) { EndPlay(); }
    if (Scene) { Unregister(); }

    ActorOwner = nullptr;
    SceneOwner = nullptr;
    Scene = nullptr;

    Super::Release();
}

void USceneComponent::Register(UScene& InScene)
{
    if (Scene == &InScene) { return; }
    if (Scene) { Unregister(); }

    Scene = &InScene;
}

void USceneComponent::BeginPlay()
{
    if (!Scene || bHasBegunPlay) { return; }
    bHasBegunPlay = true;
}

void USceneComponent::EndPlay()
{
    if (!bHasBegunPlay) { return; }
    bHasBegunPlay = false;
}

void USceneComponent::Unregister()
{
    if (bHasBegunPlay) { EndPlay(); }
    Scene = nullptr;
}

void USceneComponent::SetupAttachment(USceneComponent* InParent)
{
    if (InParent == this) { return; }

    SceneOwner = InParent;
    bGlobalDirty = true;
    if (InParent)
    {
        ActorOwner = InParent->GetActorOwner();
    }
}

void USceneComponent::Serialize(FArchive& Archive) const
{
    Super::Serialize(Archive);

    Archive.SetVector("Location", RelativeTransform.GetLocation());
    Archive.SetVector("Rotation", RelativeTransform.GetRotation().GetEulerXYZ());
    Archive.SetVector("Scale", RelativeTransform.GetScale3D());
}

void USceneComponent::Deserialize(const FArchive& Archive)
{
    Super::Deserialize(Archive);

    // Location
    RelativeTransform.SetLocation(Archive.GetVector("Location"));

    // Rotation
    constexpr float RadToDeg = 180.0f / std::numbers::pi_v<float>;
    FVector Rotation = Archive.GetVector("Rotation");
    for (int i = 0; i < 3; ++i)
    {
        Rotation[i] *= RadToDeg;
    }
    RelativeTransform.SetRotation(FQuaternion::FromEulerXYZDeg(Rotation));

    // Scale
    RelativeTransform.SetScale3D(Archive.GetVector("Scale"));
    MarkActorTransformDirty();
}

void USceneComponent::SetRelativeTransform(const FTransform& RelativeTransform)
{
    if (this->RelativeTransform == RelativeTransform) { return; }
    this->RelativeTransform = RelativeTransform;
    MarkActorTransformDirty();
}

USceneComponent* USceneComponent::GetTransformParent() const
{
    if (SceneOwner)
    {
        return SceneOwner;
    }

    if (!ActorOwner)
    {
        return nullptr;
    }

    USceneComponent* Root = ActorOwner->GetRootComponent();
    return Root == this ? nullptr : Root;
}

const FTransform& USceneComponent::GetGlobalTransform() const //나중에 부모 rootcomponent world좌표 써야됨
{
    const USceneComponent* Parent = GetTransformParent();
    uint32 ParentVersion = 0;
    if (Parent)
    {
        Parent->GetGlobalTransform(); // 부모 캐시를 먼저 최신화
        ParentVersion = Parent->GlobalVersion;
    }

    if (!bGlobalDirty && CachedParent == Parent && CachedParentVersion == ParentVersion)
    {
        return CachedGlobal;
    }

    if (!Parent)
    {
        CachedGlobal = RelativeTransform;
    }
    else if (SceneOwner || bInheritRotation)
    {
        CachedGlobal = Parent->CachedGlobal * RelativeTransform;
    }
    else
    {
        // 부모 회전 무시 - 위치와 스케일만 상속
        FTransform Result;
        Result.SetScale3D(RelativeTransform.GetScale3D());
        Result.SetRotation(RelativeTransform.GetRotation()); // 자신의 회전만 사용
        Result.SetLocation(Parent->CachedGlobal.GetLocation() + RelativeTransform.GetLocation()); // 월드 축 기준 오프셋
        CachedGlobal = Result;
    }

    CachedGlobal.GetMatrix(); // 행렬도 이 시점에 한 번만 계산해 둔다
    CachedParent = Parent;
    CachedParentVersion = ParentVersion;
    bGlobalDirty = false;
    ++GlobalVersion;
    return CachedGlobal;
}

void USceneComponent::MarkActorTransformDirty()
{
    bGlobalDirty = true;
    OnTransformChanged();
    
    if (Scene && BatchIndex >= 0)
    {
		Scene->MarkTransformDirty(BatchIndex);
    }

    if (ActorOwner)
    {
        ActorOwner->MarkComponentsTransformDirty();
    }
}

void USceneComponent::SetRelativeLocation(const FVector& RelativeLocation)
{
    FTransform NewTransform = GetRelativeTransform();
    NewTransform.SetLocation(RelativeLocation);
    SetRelativeTransform(NewTransform);
}

void USceneComponent::SetRelativeRotation(const FVector& RelativeRotationEulerAngle)
{
    FTransform NewTransform = GetRelativeTransform();
    NewTransform.SetRotation(FQuaternion::FromEulerXYZDeg(RelativeRotationEulerAngle));
    SetRelativeTransform(NewTransform);
}

void USceneComponent::SetRelativeRotation(const FQuaternion& RelativeRotation)
{
    FTransform NewTransform = GetRelativeTransform();
    NewTransform.SetRotation(RelativeRotation);
    SetRelativeTransform(NewTransform);
}

void USceneComponent::SetRelativeScale(const FVector& RelativeScale)
{
    FTransform NewTransform = GetRelativeTransform();
    NewTransform.SetScale3D(RelativeScale);
    SetRelativeTransform(NewTransform);
}

const FVector& USceneComponent::GetRelativeLocation() const
{
    return GetRelativeTransform().GetLocation();
}

const FQuaternion& USceneComponent::GetRelativeRotation() const
{
    return GetRelativeTransform().GetRotation();
}

const FVector& USceneComponent::GetRelativeScale() const
{
    return GetRelativeTransform().GetScale3D();
}
