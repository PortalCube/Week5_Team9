    #pragma once

#include "Editor/EditorViewport/FEditorViewportClient.h"
#include "Editor/Gizmo/FGizmo.h"
#include "Editor/Core/FEditorState.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/TWeakObjectPtr.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/Engine/USceneManager.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"


#include "SSplitter.h"
enum class EEditorPrimitiveType : uint8 {
  Cube,
  Cylinder,
  Sphere,
  Billboard,
  Spotlight,
};

class FEditor {
public:
  FTransform SelectedTransform;
  FVector SelectedEulerDegDisplay;

  // TODO: 이건 Scene에 들어가야함. 아마 아래와 같은 컴포넌트가 부착된 액터로 들어가야할 것
  // https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UDirectionalLightComponent
  FLightConstants GlobalLight;

  FEditorState State;

public:
  void Initialize(USceneManager *SceneManager);
  void Shutdown();

  void Process();

  void NewScene();
  void SaveScene(const FString &Path);
  void LoadScene(const FString &Path);
  bool CheckSceneExists();

  void AddViewport(FEditorViewportClient Viewport);
  void InitMultiViewport(FEditorViewportClient Viewport);
  void ResizeView(FEditorState::SplitViewMode mode);
  void DeleteViewport(int32 IndexOfViewport);
  FEditorViewportClient* GetActiveViewport(); // 임시로 0번 반환

  void UpdateCamera();

  bool SelectActor(AActor *Actor);
  void UnSelectActor();
  AActor *GetSelectedActor() const { return SelectedActor.Get(); }
  [[nodiscard]] bool ActorSelected() const { return SelectedActor.IsValid(); }
  [[nodiscard]] bool ObjectSelected() const { return SelectedActor.IsValid(); }

  [[nodiscard]] TArray<FEditorViewportClient> &GetViewports() {
    return EditorViewports;
  }
  [[nodiscard]] UScene* GetCurrentScene() const {
    return SceneManager ? SceneManager->CurrentScene : nullptr;
  }
  void SpawnActorToCurrentScene(UClass* Type, int Count = 1);
  // 피킹 등에서 현재 씬의 렌더링 대상 컴포넌트가 필요할 때 사용
  [[nodiscard]] TArray<UPrimitiveComponent *> GetPrimitiveComponents() const;
  FGizmo &GetGizmo() { return Gizmo; }
  FRenderResourceLibrary *GetRendererLibrary();

  void ClearSelectionForGC();

  void SaveState();
  void LoadState();
  void SetViewLayout(FEditorState::SplitViewMode mode);
  UTextInstanceComponent* GetTextcomp() { return SelectedActorTextComp; }
  
 //Viewport관련
  int32 ActiveViewportIndex = 0;
  SWindow* Root=nullptr;
  SWindow Leaf[4];
  SSplitterH HorizonSplitter; //세로선
  SSplitterH HorizonSplitter2; //세로선
  SSplitterV VerticalSplitter; // 가로선
private:
  USceneManager* SceneManager =
      nullptr; // 씬을 다중으로 가질 수 있도록 구조개선 가능-이경우 에디터쪽에
               // 클래스를 추가해 씬과 FEditorViewportClient들을 연관
  TArray<FEditorViewportClient> EditorViewports;
  FGizmo Gizmo;
  TWeakObjectPtr<AActor> SelectedActor;
  TWeakObjectPtr<UTextInstanceComponent> SelectedActorTextComp;
};
