#include "UScene.h"

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/FArchive.h"
#include <algorithm>

#include "Runtime/CoreUObject/TObjectIterator.h"
#include "Runtime/Core/Log.h"

IMPLEMENT_UCLASS(UScene, UObject)
UCLASS_META(UScene, SerializeName, "Scene")

const TArray<UPrimitiveComponent *> & UScene::GetRenderComponents() const {
  return RenderComponents;
}

void UScene::Initialize() {
  if (bInitialized) {
    return;
  }
  Super::Initialize();
  bInitialized = true;
}

void UScene::Release() {
  if (bHasBegunPlay) {
    EndPlay();
  }
  if (bActive) {
    Deactivate();
  }

  while (!Actors.empty()) {
    AActor *Actor = Actors.back();
    Actors.pop_back();
    DestroyObject(Actor);
  }

  RenderComponents.clear();
  RenderResourceLibrary = nullptr;
  bInitialized = false;

  Super::Release();
}

void UScene::Activate() {
  if (bActive) {
    return;
  }

  for (AActor *Actor : Actors) {
    if (Actor) {
      Actor->Register(*this);
    }
  }
  bActive = true;
}

void UScene::Deactivate() {
  if (!bActive) {
    return;
  }
  if (bHasBegunPlay) {
    EndPlay();
  }

  for (auto It = Actors.rbegin(); It != Actors.rend(); ++It) {
    if (*It) {
      (*It)->Unregister();
    }
  }
  bActive = false;
}

void UScene::BeginPlay() {
  if (!bActive || bHasBegunPlay) {
    return;
  }

  bHasBegunPlay = true;
  for (AActor *Actor : Actors) {
    if (Actor) {
      Actor->BeginPlay();
    }
  }
}

void UScene::Update(float DeltaTime) {
  if (!bHasBegunPlay) {
    return;
  }

  for (AActor *Actor : Actors) {
    if (Actor) {
      Actor->Update(DeltaTime);
    }
  }
}

void UScene::EndPlay() {
  if (!bHasBegunPlay) {
    return;
  }

  for (auto It = Actors.rbegin(); It != Actors.rend(); ++It) {
    if (*It) {
      (*It)->EndPlay();
    }
  }
  bHasBegunPlay = false;
}

void UScene::SetRenderResourceLibrary(
    FRenderResourceLibrary *InRenderResourceLibrary) {
  RenderResourceLibrary = InRenderResourceLibrary;
}

void UScene::Serialize(FArchive &Archive) const {
  Super::Serialize(Archive);

  TArray<FArchive> ActorArchives;

  for (const auto &Item : Actors) {
    if (!Item) {
      continue;
    }

    FArchive ItemArchive;
    Item->Serialize(ItemArchive);
    ActorArchives.push_back(ItemArchive);
  }

  Archive.SetArchiveArray("Actors", ActorArchives);
}

void UScene::Deserialize(const FArchive &Archive) {
  Super::Deserialize(Archive);

  if (Archive.IsNull("Actors")) {
    // Actor 목록이 비어있음
    return;
  }

  TArray<FArchive> ActorArchives = Archive.GetArchiveArray("Actors");

  for (const auto &Item : ActorArchives) {
    UClass *ClassType = UClass::FindByName(Item.GetString("Type"));
    if (ClassType == nullptr) {
      continue;
    }

    AActor *Actor = SpawnActor(ClassType);
    if (!Actor) {
      continue;
    }
    Actor->Deserialize(Item);

    if (bActive) {
      Actor->Register(*this);
    }
    if (bHasBegunPlay) {
      Actor->BeginPlay();
    }
  }
}

void UScene::AddRenderComponent(UPrimitiveComponent *prim) {
  if (prim == nullptr)
    return;

  if (std::find(RenderComponents.begin(), RenderComponents.end(), prim) ==
      RenderComponents.end()) {
    RenderComponents.push_back(prim);
  }
}

void UScene::RemoveRenderComponent(UPrimitiveComponent *prim) {
  std::erase(RenderComponents, prim);
}

void UScene::RemoveActor(AActor *Actor) { std::erase(Actors, Actor); }

void UScene::DestroyActor(AActor *Actor) {
  if (Actor == nullptr)
    return;

  RemoveActor(Actor);
  DestroyObject(Actor);
}

AActor *UScene::SpawnActor(UClass *ClassType) {
  UObject *Object = NewObject(ClassType);
  AActor *Actor = Object->Cast<AActor>();
  if (!Actor) {
    DestroyObject(Object);
    return nullptr;
  }
  Actor->Initialize();
  Actor->Register(*this);

  Actors.push_back(Actor);

  int Count = 0;

  for (TObjectIterator<AActor> It; It; ++It)
  {
      AActor* Actor = *It;

      if (Actor)
      {
          ++Count;
      }
  }

  UE_LOG("Total Actor Count : %d", Count);


  return Actor;
}
