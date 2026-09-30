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
  CullDataList.clear();
  DirtyBoundsList.clear();
  OcclusionTargetFlags.clear();
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
  if (bHasBegunPlay) {
      for (AActor* Actor : Actors) {
          if (Actor) {
              Actor->Update(DeltaTime);
          }
      }



    //return;
  }

  /*for (AActor *Actor : Actors) {
    if (Actor) {
      Actor->Update(DeltaTime);
    }
  }*/
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
      //RenderComponents에 넣기 전에 인덱스 설정
	const int32 NewIndex = static_cast<int32>(RenderComponents.size());
    prim->SetSceneIndex(NewIndex);
	prim->SetBatchIndex(NewIndex);

    RenderComponents.push_back(prim);
    SceneBVH.AddObject(prim);

    //처음엔 일단 그리자
    CullDataList.push_back(MakeAlwaysVisibleCullData());
    //오클루전 대상에도 추가
    OcclusionTargetFlags.push_back(0);
    MarkBoundsDirty(prim);
  }
}

void UScene::RemoveRenderComponent(UPrimitiveComponent *prim) {
    if (prim == nullptr || prim->GetSceneIndex() < 0)
        return;
    //TODO 제거할 때 마지막 요소와 교환하는 방식의 Swap and Pop으로 처리하도록 수정할 것

  std::erase(RenderComponents, prim);
  SceneBVH.RemoveObject(prim);

  const size_t Index = static_cast<size_t>(prim->GetSceneIndex());
  CullDataList.erase(CullDataList.begin() + Index);

  //오클루전 대상에서 제거
  OcclusionTargetFlags.erase(OcclusionTargetFlags.begin() + Index);

  // 당겨진 원소들의 인덱스 멤버 갱신
  for (size_t i = Index; i < RenderComponents.size(); ++i)
  {
      RenderComponents[i]->SetSceneIndex(static_cast<int32>(i));
	  RenderComponents[i]->SetBatchIndex(static_cast<int32>(i));
  }

  // 파괴될 포인터가 dirty 목록에 남지 않게
  if (prim->GetBoundDirtyQueued())
  {
      std::erase(DirtyBoundsList, prim);
      prim->SetBoundDirtyQueued(false);
  }

  prim->SetSceneIndex(-1);
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

  return Actor;
}

void UScene::MarkBoundsDirty(UPrimitiveComponent* Prim)
{
    // 씬에 아직 추가 전이거나 이미 대기 중이면 무시
    if (Prim == nullptr || Prim->GetSceneIndex() < 0 || Prim->GetBoundDirtyQueued())
        return;

    Prim->SetBoundDirtyQueued(true);
    DirtyBoundsList.push_back(Prim);
}

void UScene::UpdateDirtyBounds()
{
    for (UPrimitiveComponent* Prim : DirtyBoundsList)
    {
        Prim->SetBoundDirtyQueued(false);
        Prim->UpdateWorldBounds();
        int32 Index = static_cast<size_t>(Prim->GetSceneIndex());
        CullDataList[Index] = Prim->GetWorldBounds();
        OcclusionTargetFlags[Index] = Prim->IsOcclusionTarget() ? 1 : 0;
    }
    DirtyBoundsList.clear();
}
