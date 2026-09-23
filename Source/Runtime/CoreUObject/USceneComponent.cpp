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
    if (InParent)
    {
        ActorOwner = InParent->GetActorOwner();
    }
}

void USceneComponent::Serialize(FArchive& Archive) const
{
    Super::Serialize(Archive);

    Archive.SetVector("Location", RelativeTransform.Location);
    Archive.SetVector("Rotation", RelativeTransform.Rotation.GetEulerXYZ());
    Archive.SetVector("Scale", RelativeTransform.Scale3D);
}

void USceneComponent::Deserialize(const FArchive& Archive)
{
    Super::Deserialize(Archive);

    // Location
    RelativeTransform.Location = Archive.GetVector("Location");

    // Rotation
    constexpr float RadToDeg = 180.0f / std::numbers::pi_v<float>;
    FVector Rotation = Archive.GetVector("Rotation");
    for (int i = 0; i < 3; ++i)
    {
        Rotation[i] *= RadToDeg;
    }
    RelativeTransform.Rotation = FQuaternion::FromEulerXYZDeg(Rotation);

    // Scale
    RelativeTransform.Scale3D = Archive.GetVector("Scale");
}

void USceneComponent::SetRelativeTransform(const FTransform& RelativeTransform)
{
    this->RelativeTransform = RelativeTransform;
}

FTransform USceneComponent::GetGlobalTransform() const //나중에 부모 rootcomponent world좌표 써야됨
{
    if (SceneOwner)
    {
        return SceneOwner->GetGlobalTransform() * RelativeTransform;
    }

    if (!ActorOwner || ActorOwner->GetRootComponent() == this)
    {
        return RelativeTransform;
    }

    FTransform ParentWorld = ActorOwner->GetRootComponent()->GetGlobalTransform();
    
    if (!bInheritRotation)
    {
        // 부모 회전 무시 - 위치와 스케일만 상속
        FTransform Result;
        Result.Scale3D = RelativeTransform.Scale3D;
        Result.Rotation = RelativeTransform.Rotation; // 자신의 회전만 사용
        Result.Location = ParentWorld.Location + RelativeTransform.Location; // 월드 축 기준 오프셋
        return Result;
    }
    return ParentWorld * RelativeTransform;
}
