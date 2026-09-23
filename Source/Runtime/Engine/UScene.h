#pragma once

#include "Runtime/Actors/AActor.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include <concepts>
#include <type_traits>


#include "ThirdParty/Json/json.hpp"

class UScene final : public UObject {
    DECLARE_UCLASS(UScene, UObject)
    GENERATED_BODY()

public:

  void Initialize() override;
  void Release() override;
  void Activate();
  void Deactivate();
  void BeginPlay();
  void Update(float DeltaTime);
  void EndPlay();

  [[nodiscard]] bool IsActive() const { return bActive; }
  [[nodiscard]] bool HasBegunPlay() const { return bHasBegunPlay; }

  // 렌더링 컴포넌트 목록 반환
  [[nodiscard]] const TArray<UPrimitiveComponent*>& GetRenderComponents() const;
  [[nodiscard]] FRenderResourceLibrary* GetRenderResourceLibrary() const {
    return RenderResourceLibrary;
  }
  void SetRenderResourceLibrary(FRenderResourceLibrary* InRenderResourceLibrary);

  // 액터 목록 반환
  [[nodiscard]] const TArray<AActor*> &GetActors() const { return Actors; }

  // 위치와 크기를 지정하여 액터 생성
  template <typename TActor, typename... TArgs>
    requires std::derived_from<TActor, AActor>
  TActor *SpawnActor(const FVector &Location, const FVector &Scale,
                     TArgs &&...Args) {
    TActor *Actor = NewObject<TActor>(std::forward<TArgs>(Args)...);
    Actor->Initialize();

    if (Actor->GetRootComponent()) {
      FTransform Transform{};
      Transform.Location = Location;
      Transform.Scale3D = Scale;
      Actor->GetRootComponent()->SetRelativeTransform(Transform);
    }

    Actors.push_back(Actor);

    if (bActive) {
      Actor->Register(*this);
    }
    if (bHasBegunPlay) {
      Actor->BeginPlay();
    }
    return Actor;
  }

  // 기본 위치와 크기로 액터 생성
  template <typename TActor>
    requires std::derived_from<TActor, AActor>
  TActor *SpawnActor() {
    return SpawnActor<TActor>(FVector(0.0f, 0.0f, 0.0f),
                              FVector(1.0f, 1.0f, 1.0f));
  }

  // 첫번째 인자가 벡터가 아닐 때 기본 위치와 크기 전달
  template <typename TActor, typename FirstArg, typename... RestArgs>
    requires std::derived_from<TActor, AActor> &&
             (!std::is_same_v<std::decay_t<FirstArg>, FVector>)
  TActor *SpawnActor(FirstArg &&First, RestArgs &&...Rest) {
    return SpawnActor<TActor>(
        FVector(0.0f, 0.0f, 0.0f), FVector(1.0f, 1.0f, 1.0f),
        std::forward<FirstArg>(First), std::forward<RestArgs>(Rest)...);
  }

  virtual void Serialize(FArchive& Archive) const override;
  virtual void Deserialize(const FArchive& Archive) override;

  void AddRenderComponent(UPrimitiveComponent *prim);
  void RemoveRenderComponent(UPrimitiveComponent *prim);
  void RemoveActor(AActor* Actor);

  void DestroyActor(AActor* Actor);

    AActor* SpawnActor(UClass* ClassType);

private:
  TArray<AActor*> Actors;                        // 액터 목록 (Update용)
  TArray<UPrimitiveComponent*> RenderComponents; // 렌더링큐 (Draw용)
  TMap<UPrimitiveComponent*, size_t> RenderIndices;

  FRenderResourceLibrary* RenderResourceLibrary = nullptr;
  bool bInitialized = false;
  bool bActive = false;
  bool bHasBegunPlay = false;
};
