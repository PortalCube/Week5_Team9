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
#include <Editor/UI/Imgui/FImguiStatsWindow.h>
#include <Runtime/CoreUObject/FStatsManager.h>


void FEditorApplication::Initialize_ImguiWin32DX11(
    HWND &Window, ID3D11Device *Device, ID3D11DeviceContext *Context) {
  ImguiManager.Initialize_ImplWin32DX11(Window, Device, Context);
}

void FEditorApplication::Initialize_Runtime(USceneManager *SceneManager,
                                            FRenderView *RenderView) {
  this->RenderView = RenderView;
  this->SceneManager = SceneManager;
  this->CurrentScene = SceneManager->CurrentScene;

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

void FEditorApplication::BeginFrame()
{ 
    ImguiManager.NewFrame();
}

void FEditorApplication::Tick(float DeltaTime) {
  ToolBar.Process(Editor, ConsoleWindow, ControlPanelWindow, PropertyWindow);
  EditorViewportWindow.Process(Editor, DeltaTime);
  WorldOutliner.Process(Editor);
  ControlPanelWindow.Process(Editor);
  PropertyWindow.Process(Editor);
  ConsoleWindow.Process(Editor, [this](const char* Command) {ExecuteCommand(Command);});
  ContentsDrawer.Process(Editor);
  Editor.Process();
}

void FEditorApplication::Render() {
  TArray<FEditorViewportClient> &EditorViewports = Editor.GetViewports();
  
  // 렌더 준비
  RenderView->PrepareRender();

  {
      //컬링 준비 시간 기록?
      // 
      //이동한 오브젝트는 월드 AABB 재계산
      SceneManager->CurrentScene->UpdateDirtyBounds();
  }

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
              //나중에 ViewIndex를 추가할 날이?
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
        EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::Memory);
    }

    else if (lowerCmd.compare("stat fps") == 0) {
        UE_LOG("Stat FPS Command is executed!");
        EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::FPS);
    }

    else if (lowerCmd.compare("stat unit") == 0) {
        UE_LOG("Stat unit Command is executed!");
        EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::Unit);
    }

    else if (lowerCmd.compare("stat none") == 0) {
        UE_LOG("Stat Window is closed!");
        EditorViewportWindow.SetClose();
    }

    else if (lowerCmd.compare("stat cull") == 0)
    {
        UE_LOG("Stat Cull Command is executed!");
        //EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::Cull);
    }

    else if (lowerCmd.compare("cull") == 0)
    {
        //컬링 토글
        RenderView->SetCullingEnabled(!RenderView->GetCullingSettings().bEnabled);
        UE_LOG("Culling : %s", RenderView->GetCullingSettings().bEnabled ? "ON" : "OFF");
    }

    else if (lowerCmd.compare("cull on") == 0)
    {
        //컬링 On
        RenderView->SetCullingEnabled(true);
        UE_LOG("Culling : %s", RenderView->GetCullingSettings().bEnabled ? "ON" : "OFF");
    }

    else if (lowerCmd.compare("cull off") == 0)
    {
        //컬링 Off
        RenderView->SetCullingEnabled(false);
        UE_LOG("Culling : %s", RenderView->GetCullingSettings().bEnabled ? "ON" : "OFF");
    }

    else if (lowerCmd.compare("cull freeze") == 0)
    {
        //컬링 freeze 토글
        RenderView->SetCullingFreeze(!RenderView->GetCullingSettings().bFreeze);
        RenderView->InvalidateFrozenFrustums();
        UE_LOG("Frustum Freeze : %s", RenderView->GetCullingSettings().bFreeze ? "ON" : "OFF");
    }

    else {
        UE_LOG("Unknown command: '%s'\n", Command);
        return;
    }
}

