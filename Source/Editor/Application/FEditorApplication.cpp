#include "FEditorApplication.h"

#include "Runtime/CoreUObject/UAnimatedBillboardComp.h"
#include "Runtime/CoreUObject/UBillBoardComp.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Rendering/FMesh.h"
#include <Windows.h>

#include <algorithm>
#include <cctype>

#include "Runtime/Core/FString.h"

#include "Runtime/Engine/FSceneView.h"

#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/TestTextActor.h"

#include "Editor/Visualizer/IVisualizer.h"
#include "Editor/Core/FEditor.h"
void FEditorApplication::Initialize_ImguiWin32DX11(
    HWND &Window, ID3D11Device *Device, ID3D11DeviceContext *Context) {
  ImguiManager.Initialize_ImplWin32DX11(Window, Device, Context);
}

void FEditorApplication::Initialize_Runtime(USceneManager *SceneManager,
                                            FRenderView *RenderView) {
  this->RenderView = RenderView;
  this->SceneManager = SceneManager;
  this->CurrentScene = SceneManager->CurrentScene;

  //
  Editor.Initialize(SceneManager);
  Editor.InitMultiViewport(FEditorViewportClient{});
  Editor.LoadState();
  Editor.SetViewLayout(Editor.State.GetSplitMode());
}

void FEditorApplication::Shutdown() { Editor.Shutdown(); }

void FEditorApplication::Update(float DeltaTime) {
  BeginFrame();
  Tick(DeltaTime);
}

void FEditorApplication::BeginFrame() { ImguiManager.NewFrame(); }

void FEditorApplication::Tick(float DeltaTime) {
  ToolBar.Process(Editor, ConsoleWindow, ControlPanelWindow, PropertyWindow);
  EditorViewportWindow.Process(Editor, DeltaTime);
  WorldOutliner.Process(Editor);
  ControlPanelWindow.Process(Editor);
  PropertyWindow.Process(Editor);
  ConsoleWindow.Process(Editor, [this](const char* Command) {ExecuteCommand(Command);});
  ContentsDrawer.Process(Editor);
  StatsWindow.Process(Editor, DeltaTime); // deltatime 전달 필요
  Editor.Process();
}

void FEditorApplication::Render() {
  TArray<FEditorViewportClient> &EditorViewports = Editor.GetViewports();
  
  // 렌더 준비
  RenderView->PrepareRender();

  //Active인 ViewportClient만 렌더링
  for (SWindow& Leaf : Editor.Leaf)
  {
      if (!Leaf.bisActive) continue;
      FEditorViewportClient& EditorViewport = EditorViewports[Leaf.ViewportIndex];

          // 뷰포트 렌더링 명세 구성
          FSceneView sceneview{
              .Camera = EditorViewport.ViewportCamera,
              .ViewProj = EditorViewport.ViewportCamera.CreateViewProjectionMatrix(),
              .TopLeftUV = EditorViewport.TopLeftUV,
              .LengthUV = EditorViewport.LengthUV,
              .ViewMode = EditorViewport.ViewMode,
              .ShowFlags = EditorViewport.ShowFlags,
              .LightConstants = Editor.GlobalLight
          };

          // 에디터 렌더링 컨텍스트 구성
          FEditorRenderContext EditorCtx;
          EditorCtx.SelectedActor = Editor.GetSelectedActor();
          EditorCtx.SelectedTransform = Editor.SelectedTransform;
          EditorCtx.Gizmo = Editor.ObjectSelected() ? &Editor.GetGizmo() : nullptr;
          EditorCtx.TextComp = Editor.ObjectSelected() ? Editor.GetTextcomp() : nullptr;
          EditorCtx.Grid = &EditorViewport.GetGrid();
          EditorCtx.VisualizerRegistry = &VisualizerRegistry;

          if (EditorCtx.SelectedActor) {
              if (USceneComponent* RootComp = EditorCtx.SelectedActor->GetRootComponent()) {
                  EditorCtx.SelectedPrimitive = RootComp->Cast<UPrimitiveComponent>();
              }
          }

          // 뷰포트 렌더링 일괄 수행
          RenderView->RenderView(sceneview, *SceneManager->CurrentScene, EditorCtx);

  }

  //기즈모 그리기
  if (Editor.ObjectSelected())
  {
      for (const SWindow& Leaf : Editor.Leaf)
      {
          if (!Leaf.bisActive)
              continue;

          const auto& Viewport = EditorViewports[Leaf.ViewportIndex];

          FSceneView SceneView{
    .Camera = Viewport.ViewportCamera,
    .ViewProj = Viewport.ViewportCamera.CreateViewProjectionMatrix(),
    .TopLeftUV = Viewport.TopLeftUV,
    .LengthUV = Viewport.LengthUV,
    .ViewMode = Viewport.ViewMode,
    .ShowFlags = Viewport.ShowFlags,
    .LightConstants = Editor.GlobalLight
          };

          RenderView->RenderOverlayPass(Viewport.ViewportCamera, SceneView, Editor.SelectedTransform, Editor.GetGizmo(), Editor.GetTextcomp());
          // 마지막으로 그린 뷰의 렌더 모드가 남지 않도록 설정

          RenderView->SetRenderMode(Viewport.ViewMode);
          RenderView->RenderGizmo(
              Editor.SelectedTransform,
              Viewport.ViewportCamera,
              Viewport.TopLeftUV,
              Viewport.LengthUV,
              Editor.GetGizmo());
      }
  }

  ImguiManager.RenderUI();
}

void FEditorApplication::OnWindowSize(UINT Width, UINT Height) {
  // 뷰포트 종횡비 갱신
  for (auto &Viewport : Editor.GetViewports()) {
    const FVector2 SizePixels =
        Viewport.LengthUV *
        FVector2{static_cast<float>(Width), static_cast<float>(Height)};

    auto &Camera = Viewport.ViewportCamera;
    Camera.Projection.Aspect = SizePixels.X / SizePixels.Y;
  }
}

void FEditorApplication::ExecuteCommand(const char* Command) {
    if (!Command) return;

    FString lowerCmd = Command;
    std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(), ::tolower);

    if (lowerCmd.compare("stat memory") == 0) {
        UE_LOG("Stat Memory Command is executed!");
        EditorViewportWindow.SetOpen(FImguiEditorViewportWindow::EStatsWindow::Memory, true);
        //StatsWindow.SetOpen(FImguiStatsWindow::EStatsWindow::Memory, true);
    }

    else if (lowerCmd.compare("stat fps") == 0) {
        UE_LOG("Stat FPS Command is executed!");
        EditorViewportWindow.SetOpen(FImguiEditorViewportWindow::EStatsWindow::FPS, true);
    }

    else if (lowerCmd.compare("stat none") == 0) {
        UE_LOG("Stat Window is closed!");
        EditorViewportWindow.SetClose();
    }

    else {
        UE_LOG("Unknown command: '%s'\n", Command);
        return;
    }
}

