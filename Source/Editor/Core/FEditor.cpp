#include "FEditor.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/ACubeActor.h"
#include "Runtime/Actors/ASphereActor.h"
#include "Runtime/Actors/ACylinderActor.h"
#include "Runtime/Actors/ABillboardActor.h"
#include "Runtime/Actors/ASpotlightActor.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Math/Random.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include <numbers>

void FEditor::Initialize(USceneManager *SceneManager) {
  State.ReadFromFile();
  Gizmo.Initialize();
  SelectedActorTextComp = NewObject<UTextInstanceComponent>();
  if (SelectedActorTextComp)
  {
    SelectedActorTextComp->Initialize();
    SelectedActorTextComp->SetInheritRotation(false);
    FAssetRegistry& Registry = FAssetRegistry::GetInstance();
    SelectedActorTextComp->SetMesh(Registry.Get<UStaticMesh>("#Rect"));
    SelectedActorTextComp->SetMaterial(Registry.Get<UMaterial>("Material/SelectedActor_Text.json"));
    SelectedActorTextComp->SetFont(FName("bazziotf"));
  }
  this->SceneManager = SceneManager;
}

void FEditor::Shutdown() {
  SaveState();
  State.FlushToFile();
}

FRenderResourceLibrary *FEditor::GetRendererLibrary() {
  return &FRenderResourceLibrary::Get();
}

void FEditor::Process() {
  // 씬의 액터 업데이트
  
    if (FInputManager::Get().IsKeyDown(VK_DELETE) && SelectedActor)
    {
        AActor* Target = SelectedActor;
        UnSelectActor();
        Target->Destroy();
    }
    
  if (SceneManager && SceneManager->CurrentScene) {
    SceneManager->CurrentScene->Update(FTimeManager::Get().GetDeltaTime());
  }

  if (SelectedActor) {
    SelectedActor->SetTransform(SelectedTransform);
  }

  SaveState();
  State.Tick(FTimeManager::Get().GetDeltaTime());
}

void FEditor::SaveState() {
  const FEditorViewportClient* Viewport = GetActiveViewport();
  if (!Viewport) { return; }

  const FCamera& Camera = Viewport->ViewportCamera;
  State.SetCameraLocation(Camera.Position);
  State.SetCameraPitch(Camera.Pitch);
  State.SetCameraYaw(Camera.Yaw);
  State.SetCameraFOV(Camera.Projection.FOV);
  State.SetGridCellSize(Viewport->GetGrid().GetCellSize());
  State.SetGizmoMode(static_cast<uint8>(Gizmo.Mode));
  State.SetGizmoSpace(static_cast<uint8>(Gizmo.GetSpace()));
  State.SetSelectedActor(SelectedActor ? SelectedActor->GetUUID() : static_cast<uint32>(-1));
}

void FEditor::LoadState()
{
    FEditorViewportClient* Viewport = GetActiveViewport();
    if (!Viewport) { return; }

    FCamera& Camera = Viewport->ViewportCamera;

    Camera.Position = State.GetCameraLocation();
    Camera.Pitch = State.GetCameraPitch();
    Camera.Yaw = State.GetCameraYaw();
    Camera.Projection.FOV = State.GetCameraFOV();
    Viewport->GetGrid().SetCellSize(State.GetGridCellSize());
    Gizmo.Mode = static_cast<EGizmoMode>(State.GetGizmoMode());
    Gizmo.SetGizmoSpace(static_cast<EGizmoSpace>(State.GetGizmoSpace()));

    //viewmode관련
    VerticalSplitter.Ratio = State.GetSplitter().X;
    HorizonSplitter.Ratio = State.GetSplitter().Y;
    HorizonSplitter2.Ratio = State.GetSplitter().Z;
    
}

void FEditor::NewScene() {
  UnSelectActor();
  SceneManager->SetScene(NewObject<UScene>());
  State.ResetToDefaults();
  LoadState();
}

void FEditor::SaveScene(const FString &Path) { SceneManager->SaveScene(Path); }

void FEditor::LoadScene(const FString &Path) 
{

  // 씬 로드
  SceneManager->LoadScene(Path);
  SelectedActor = nullptr;
}

bool FEditor::CheckSceneExists() {
  if (SceneManager->CurrentScene == nullptr)
    return false;
  return true;
}

void FEditor::AddViewport(FEditorViewportClient Viewport) {
  EditorViewports.push_back(Viewport);
}
void FEditor::InitMultiViewport(FEditorViewportClient Viewport) {
  EditorViewports.push_back(Viewport);
  EditorViewports.push_back(Viewport);
  EditorViewports.push_back(Viewport);
  EditorViewports.push_back(Viewport);

}
void FEditor::DeleteViewport(int32 IndexOfViewport) {
  EditorViewports.erase(EditorViewports.begin() + IndexOfViewport);
}

FEditorViewportClient* FEditor::GetActiveViewport() {
  if (EditorViewports.empty()) {
    return nullptr;
  }
  return &EditorViewports[ActiveViewportIndex];
}

bool FEditor::SelectActor(AActor *Actor) {
  if (SelectedActor) {
    UnSelectActor();
  }

  SelectedActor = Actor;
  if (SelectedActor) {
    SelectedTransform = SelectedActor->GetTransform();
    SelectedEulerDegDisplay = SelectedTransform.Rotation.GetEulerXYZ();
    if (Gizmo.Mode == EGizmoMode::None) {
      Gizmo.Mode = EGizmoMode::Translate;
    }

    if (SelectedActorTextComp) {
      SelectedActorTextComp->SetActorOwner(SelectedActor.Get());
      FTransform RelativeTrans;
      RelativeTrans.Location = FVector{ 0.0f, 0.0f, 1.5f }; 
      SelectedActorTextComp->SetRelativeTransform(RelativeTrans);
      SelectedActorTextComp->SetText(L"UUID : " + std::to_wstring(SelectedActor->GetUUID()));
    }
  }

  return true;
}

void FEditor::UnSelectActor() {
  if (SelectedActor) {
    SelectedActor->SetTransform(SelectedTransform);
  }
  SelectedActor = nullptr;
  if (SelectedActorTextComp) {
    SelectedActorTextComp->SetActorOwner(nullptr);
  }
}

TArray<UPrimitiveComponent *> FEditor::GetPrimitiveComponents() const {
  if (!SceneManager || !SceneManager->CurrentScene) {
    return {};
  }
  return SceneManager->CurrentScene->GetRenderComponents();
}

void FEditor::ClearSelectionForGC() {
  SelectedActor = nullptr;
  Gizmo.EndInteraction();
  Gizmo.HoveredHandle = EGizmoHandle::None;
}

void FEditor::SpawnActorToCurrentScene(UClass* Type, int Size) {
    if (!SceneManager || !SceneManager->CurrentScene) {
        return;
    }

    if (Size <= 0) { return; }

    const float Min = State.GetSpawnActorMinLocation();
    const float Max = State.GetSpawnActorMaxLocation();
    if (Min > Max) { return; }

    for (int i = 0; i < Size; ++i)
    {

        FVector Location
        {
            Random::GetFloat(Min, Max, 2),
            Random::GetFloat(Min, Max, 2),
            Random::GetFloat(Min, Max, 2),
        };

        FTransform Transform;
        Transform.Location = Location;
        Transform.Scale3D = FVector{ 0.5f, 0.5f, 0.5f };

        AActor* NewActor = SceneManager->CurrentScene->SpawnActor(Type);
        if (!NewActor) { return; }


        FTransform CurrentTransform = NewActor->GetTransform();
        CurrentTransform.Location = Location;
        NewActor->SetTransform(CurrentTransform);

        // 액터 시작 및 선택
        NewActor->BeginPlay();
        SelectActor(NewActor);
    }
}

void FEditor::ResizeView(FEditorState::SplitViewMode mode)
{
//viewport를 가지고있는 splitter,window를 업데이트
    ActiveViewportIndex = 0;
    //=== 초기화 ===//
    for (int32 i = 0; i < 4; ++i)
    {
        Leaf[i].ViewportIndex = i;
        Leaf[i].bisActive = false;
    }

    HorizonSplitter2.bisActive = false;
    VerticalSplitter.bisActive = false;
    HorizonSplitter.bisActive = false;
    //=== 초기화 ===//

    //===람다함수===//
    auto Connect = [](SSplitter& Splitter, SWindow& LT, SWindow& RB)
        {
            Splitter.SideLT = &LT;
            Splitter.SideRB = &RB;

            Splitter.bisActive = true;
            LT.bisActive = true;
            RB.bisActive = true;
        };

    switch (mode)
    {
        case FEditorState::SplitViewMode::SINGLE:
        Leaf[0].bisActive = true;
        Root = &Leaf[0];
        break;

    case FEditorState::SplitViewMode::HORIZONTAL:
        Leaf[0].bisActive = true;
        Leaf[1].bisActive = true;
        Connect(HorizonSplitter, Leaf[0], Leaf[1]);
        Root = &HorizonSplitter;
        break;

    case FEditorState::SplitViewMode::VERTICAL:
        Leaf[0].bisActive = true;
        Leaf[2].bisActive = true;
        Connect(VerticalSplitter, Leaf[0], Leaf[2]);
        Root = &VerticalSplitter;
        break;

    case FEditorState::SplitViewMode::QUAD:
        Leaf[0].bisActive = true;
        Leaf[1].bisActive = true;
        Leaf[2].bisActive = true;
        Leaf[3].bisActive = true;
        Connect(VerticalSplitter, HorizonSplitter, HorizonSplitter2);
        Connect(HorizonSplitter, Leaf[0], Leaf[1]);
        Connect(HorizonSplitter2, Leaf[2], Leaf[3]);
        Root = &VerticalSplitter;
        break;
    }
}
void FEditor::SetViewLayout(FEditorState::SplitViewMode mode) {
    ResizeView(mode);

    auto SetPerspectiveView = [this](int32 ViewportIndex)
    {
        FEditorViewportClient& Viewport = EditorViewports[ViewportIndex];
        Viewport.eOrthogonalType = FEditorViewportClient::EOrthogonalType::PERSPECTIVE;
        Viewport.ViewportCamera.Projection.ProjectionType = EProjectionType::Perspective;
    };

    auto SetOrthographicView = [this](int32 ViewportIndex, FEditorViewportClient::EOrthogonalType Type)
    {
        EditorViewports[ViewportIndex].SetOrthograpihcView(Type);
    };

    switch (mode)
    {
    case FEditorState::SplitViewMode::SINGLE:
        VerticalSplitter.bisActive = false;
        HorizonSplitter.bisActive = false;
        HorizonSplitter2.bisActive = false;
        SetPerspectiveView(0);
        State.SetSplitMode(FEditorState::SplitViewMode::SINGLE);
        break;

    case FEditorState::SplitViewMode::VERTICAL:
        VerticalSplitter.bisActive = true;
        HorizonSplitter.bisActive = false;
        HorizonSplitter2.bisActive = false;
        SetOrthographicView(0, FEditorViewportClient::EOrthogonalType::ORTHOGRAPHIC_TOP);
        SetPerspectiveView(2);
        State.SetSplitMode(FEditorState::SplitViewMode::VERTICAL);
        break;

    case FEditorState::SplitViewMode::HORIZONTAL:
        VerticalSplitter.bisActive = false;
        HorizonSplitter.bisActive = true;
        HorizonSplitter2.bisActive = false;
        SetOrthographicView(0, FEditorViewportClient::EOrthogonalType::ORTHOGRAPHIC_TOP);
        SetPerspectiveView(1);
        State.SetSplitMode(FEditorState::SplitViewMode::HORIZONTAL);
        break;

    case FEditorState::SplitViewMode::QUAD:
        VerticalSplitter.bisActive = true;
        HorizonSplitter.bisActive = true;
        HorizonSplitter2.bisActive = true;
        SetOrthographicView(0, FEditorViewportClient::EOrthogonalType::ORTHOGRAPHIC_TOP);
        SetPerspectiveView(1);
        SetOrthographicView(2, FEditorViewportClient::EOrthogonalType::ORTHOGRAPHIC_FRONT);
        SetOrthographicView(3, FEditorViewportClient::EOrthogonalType::ORTHOGRAPHIC_RIGHT);
        State.SetSplitMode(FEditorState::SplitViewMode::QUAD);
        break;

    }
}
