#include "FRenderView.h"

#include "Editor/Gizmo/FGizmo.h"
#include "Editor/Grid/FGrid.h"
#include "Editor/Visualizer/FVisualizerRegistry.h"
#include "Editor/Visualizer/IVisualizer.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/CoreUObject/UBillBoardComp.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Engine/FSceneView.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Engine/FRenderData.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Engine/FTimeManager.h"
#include <fstream>

FRenderView::FRenderView(FRenderer &Renderer) : Renderer(Renderer) {}

namespace
{
    FDrawCommand GetDrawCommand(const FCamera& Camera, const FRenderData& Data)
    {
        if (!Data.Mesh || Data.Materials.empty())
        {
            return {};
        }

        FObjectConstants Constants
        {
            .MVP = Data.ModelMatrix * Camera.CreateViewProjectionMatrix(),
            .Color = Data.Materials[0].Color,
            .UVScale = Data.Materials[0].UVScale,
            .UVOffset = Data.Materials[0].UVOffset,
            .World = Data.ModelMatrix,
            .DisableShading = Data.Materials[0].bDisableShading ? 1.0f : 0.0f,
        };

        TArray<FMaterial> Materials;

        for (const auto& Item : Data.Materials)
        {
            if (!Item.Pipeline)
            {
                continue;
            }

            FMaterial Material{};
            Material.SetPipeLine(Item.Pipeline->Get());

            if (Item.Texture)
            {
                Material.SetTexture(Item.Texture->Get());
            }

            Material.SetSamplerDesc(Item.SamplerDesc);

            Materials.push_back(Material);
        }

        return FDrawCommand
        {
            .Mesh = Data.Mesh->Get(),
            .Materials = Materials,
            .Constants = Constants,
            .Type = Data.Type,
            .Instances = Data.Instances,
        };
    }
}


void FRenderView::CollectScenePrimitives(const UScene& Scene, const FSceneView& View, const AActor* SelectedActor)
{
    for (auto& PrimitiveComponent : Scene.GetRenderComponents())
    {
        if (!PrimitiveComponent) continue;

        // 쇼 플래그 확인
        if ((static_cast<uint64>(View.ShowFlags) & static_cast<uint64>(PrimitiveComponent->GetShowFlag())) == 0)
        {
            continue;
        }

        bool bSelected = false;
        if (PrimitiveComponent->GetActorOwner() && PrimitiveComponent->GetActorOwner() == SelectedActor)
        {
            bSelected = true;
        }

        const FRenderData& Data = PrimitiveComponent->GetRenderData(View.Camera);
        FDrawCommand DrawCommand = GetDrawCommand(View.Camera, Data);

        // 인스턴싱 및 텍스트는 인스턴스 배열을 사용하므로 바로 푸시
        if (DrawCommand.Type == ERenderType::Text || DrawCommand.Type == ERenderType::Instancing)
        {
            RenderQueue.Push(DrawCommand);
            continue;
        }

        const FMatrix World = PrimitiveComponent->GetRenderMatrix(View.Camera);
        DrawCommand.Constants.MVP = World * View.ViewProj;
        DrawCommand.Constants.World = World;
        DrawCommand.Constants.Color = { 1.0f, 1.0f, 1.0f, 0.0f };
        DrawCommand.Constants.DisableShading = View.ViewMode == EViewModeIndex::VMI_Unlit ? 1.0f : 0.0f;

        if (bSelected && DrawCommand.Constants.Color.W > 0.0f)
        {
            DrawCommand.Constants.Color = DrawCommand.Constants.Color * 0.7f + FVector4{ 0.3f, 0.3f, 0.3f, 0.0f };
        }
        else if (bSelected)
        {
            DrawCommand.Constants.Color = { 1.0f, 1.0f, 1.0f, 0.5f };
        }
        RenderQueue.Push(DrawCommand);
    }
}

void FRenderView::PrepareRender()
{
    FFrameConstants FrameConstants
    {
        .Time = FTimeManager::Get().GetTime(),
        .DeltaTime = FTimeManager::Get().GetDeltaTime(),
    };

    Renderer.UpdateFrameConstants(FrameConstants);
}

void FRenderView::RenderView(const FSceneView& View, const UScene& Scene, const FEditorRenderContext& EditorCtx)
{
    // 뷰포트 시작
    BeginView(View);

    // 씬 컴포넌트 수집
    CollectScenePrimitives(Scene, View, EditorCtx.SelectedActor);

    // 기본 씬 오브젝트 패스
    FlushBasePass(View.Camera);

    // 에디터 라인 패스
    if (EditorCtx.Grid && (View.ShowFlags & static_cast<uint32>(EEngineShowFlags::SF_Grid)) != 0) {
        DrawGrid(View.Camera, *EditorCtx.Grid);
    }

    if (EditorCtx.SelectedPrimitive && EditorCtx.VisualizerRegistry) {

        UClass* ClassType = EditorCtx.SelectedPrimitive->GetClass();
        FVisualizerRegistry& Registry = *EditorCtx.VisualizerRegistry;

        IVisualizer* Visualizer = Registry.FindVisualizer(ClassType);

        if (Visualizer)
        {
            Visualizer->Draw(
                *EditorCtx.SelectedPrimitive,
                *this,
                View.Camera,
                FVector4{0.0f, 1.0f, 0.0f, 1.0f}
            );
        }
    }
    
    FlushLinePass(View.Camera);

    // 후처리 외곽선 패스
    RenderPostProcessPass(View.Camera, EditorCtx.SelectedActor);

    // 오버레이 패스
    if (EditorCtx.Gizmo && EditorCtx.SelectedActor)
    {
        //RenderOverlayPass(View.Camera, View, EditorCtx.SelectedTransform, *EditorCtx.Gizmo, EditorCtx.TextComp);
    }
}

void FRenderView::BeginView(const FSceneView& View)
{
    // 에디터 뷰포트 렌더타겟 바인딩
    Renderer.BindEditorViewportRenderTargets();
    Renderer.SetViewportUV(View.TopLeftUV, View.LengthUV);
    Renderer.SetRenderMode(View.ViewMode);
    Renderer.UpdateLightConstants(View.LightConstants, View.ViewMode);

    // ViewConstants 갱신
    FViewConstants ViewConstants
    {
        .VP = View.ViewProj,
        .ViewportSize = FVector2
        {
            View.LengthUV.X * Renderer.GetWidth(),
            View.LengthUV.Y * Renderer.GetHeight(),
        },
    };

    Renderer.UpdateBuffer(ViewConstants, 1);
}

void FRenderView::DrawGrid(const FCamera& Camera, FGrid& Grid)
{
    Grid.DrawLine(Renderer, Camera);

    FGridLineConstants Constants{};
    Constants.MVP = Camera.CreateViewProjectionMatrix();
    Constants.CameraPosition = Camera.Position;
    Constants.FadeStartDistance = 3.0f;
    Constants.FadeEndDistance = 75.0f;
    Renderer.FlushLineBatch(Constants, FName("Grid"));
}

void FRenderView::FlushBasePass(const FCamera& Camera)
{
    FlushQueue(Camera);
}

void FRenderView::FlushLinePass(const FCamera& Camera)
{
    FlushLineBatch(Camera.CreateViewProjectionMatrix());
}

void FRenderView::RenderPostProcessPass(const FCamera& Camera, const AActor* SelectedActor)
{
    RenderOutline(Camera, SelectedActor);
}

void FRenderView::RenderOverlayPass(const FCamera& Camera, const FSceneView& SceneView, const FTransform& SelectedTransform, const FGizmo& Gizmo, UTextInstanceComponent* TextComp)
{
    // 뷰포트 영역 재설정
    Renderer.SetViewportUV(SceneView.TopLeftUV, SceneView.LengthUV);

    //// 기즈모 렌더링
    //Renderer.ClearDepth();
    //Gizmo.Draw(Renderer, SelectedTransform, Camera);

    // 텍스트 오버레이 렌더링
    if (TextComp && (SceneView.ShowFlags & static_cast<uint64>(EEngineShowFlags::SF_BillboardText)))
    {
        Renderer.ClearDepth();
        FRenderData Data = TextComp->GetRenderData(Camera);
        FDrawCommand Command = GetDrawCommand(Camera, Data);
        if (!Data.Instances.empty())
        {
            Renderer.AddTextInstanceArray(Command);
            Renderer.DrawTextInstances(Command);
            Renderer.ClearTextInstances();
        }
    }
}

void FRenderView::RenderGizmo(const FTransform &Transform,
                              const FCamera &Camera, FVector2 TopLeftUV,
                              FVector2 LengthUV, const FGizmo &Gizmo) {
  Renderer.SetViewportUV(TopLeftUV, LengthUV);
  Renderer.ClearDepth();
  Gizmo.Draw(Renderer, Transform, Camera);
}

void FRenderView::RenderLine(const FVector &Start, const FVector &End,
                             const FVector4 &Color) {
  FLineBatcher &LineBatcher = Renderer.GetLineBatcher();
  LineBatcher.DrawLine(Start, End, Color);
}

void FRenderView::RenderBoxCenterExtent(const FVector &Center,
                                        const FVector &Extent,
                                        const FVector4 &Color) {
  FLineBatcher &LineBatcher = Renderer.GetLineBatcher();
  LineBatcher.DrawBoxCenterExtent(Center, Extent, Color);
}

void FRenderView::RenderBoxMinMax(const FVector &Min, const FVector &Max,
                                  const FVector4 &Color) {
  FLineBatcher &LineBatcher = Renderer.GetLineBatcher();
  LineBatcher.DrawBoxMinMax(Min, Max, Color);
}

void FRenderView::RenderQuad(
    const FVector& A,
    const FVector& B,
    const FVector& C,
    const FVector& D,
    const FVector4& Color
)
{
    FLineBatcher& LineBatcher = Renderer.GetLineBatcher();
    LineBatcher.DrawQuad(A, B, C, D, Color);
}

void FRenderView::RenderSphere(const FVector &Center, float Radius,
                               const FVector4 &Color, uint32 Segments) {
  FLineBatcher &LineBatcher = Renderer.GetLineBatcher();
  LineBatcher.DrawSphere(Center, Radius, Color, Segments);
}

void FRenderView::RenderOutline(const FCamera &Camera,
                                const AActor *SelectedActor) {
  DrawStencilMask(Camera, SelectedActor);
  Renderer.RenderOutline();
}

void FRenderView::DrawStencilMask(const FCamera& Camera,
                                  const AActor* SelectedActor) {
    if (!SelectedActor) return;

    USceneComponent* RootComp = SelectedActor->GetRootComponent();
    if (!RootComp) return;

    UPrimitiveComponent* PrimComp = RootComp->Cast<UPrimitiveComponent>();
    if (!PrimComp) return;

    FRenderData Data = PrimComp->GetRenderData(Camera);
    const FMatrix ModelMatrix = PrimComp->GetRenderMatrix(Camera);
    FDrawCommand DrawCommand = GetDrawCommand(Camera, Data);

    DrawCommand.Constants.DisableShading = true;
    DrawCommand.Constants.MVP = ModelMatrix * Camera.CreateViewProjectionMatrix();
    DrawCommand.Constants.World = ModelMatrix;

    auto OutlineMaterial = FRenderResourceLibrary::Get().GetMaterial("#Outline");
    if (OutlineMaterial)
    {
        OutlineMaterial->GetPipeline()->SetStencilRef(1);
        DrawCommand.Materials = { *OutlineMaterial };
        Renderer.Draw(DrawCommand, 2, false);
    }
}

void FRenderView::SetRenderMode(EViewModeIndex InMode)
{
    Renderer.SetRenderMode(InMode);
}

void FRenderView::UpdateLightConstants(const FLightConstants& Constants, const EViewModeIndex InMode)
{
    Renderer.UpdateLightConstants(Constants, InMode);
}

void FRenderView::DrawInstances(const FCamera& Camera)
{
    Renderer.DrawInstances(Camera);
}

void FRenderView::ClearTextInstances()
{
    Renderer.ClearTextInstances();
}

void FRenderView::FlushLineBatch(const FMatrix& ViewProjection, const FName& PipelineId)
{
    FObjectConstants Constants{};
    Constants.MVP = ViewProjection;
    Constants.DisableShading = 1.0f;
    Renderer.FlushLineBatch(Constants, PipelineId);
}

void FRenderView::FlushQueue(const FCamera& Camera)
{
    auto& ResLib = FRenderResourceLibrary::Get();
    
    // Primitive 큐 처리
    for (const FDrawCommand& Data : RenderQueue.GetPrimRenderQ())
    {
        Renderer.Draw(Data);
    }

    // Instancing 큐
    if (!RenderQueue.IsInstancingRQEmpty())
    {
        for (const FDrawCommand& Data : RenderQueue.GetInstancingRenderQ())
        {
            Renderer.AddTextInstanceArray(Data);
        }
        Renderer.DrawInstances(Camera);
        Renderer.ClearTextInstances();
    }

    // Texture 큐: Primitive와 동일하지만 TextureId로 머티리얼 텍스처 교체 후 드로우
    for (const FDrawCommand& Data : RenderQueue.GetTextureRenderQ())
    {
        Renderer.Draw(Data);
    }

    // Spotlight 큐: 불투명 렌더링 후 가산 블렌딩 수행
    for (const FDrawCommand& Data : RenderQueue.GetSpotlightRenderQ())
    {
        Renderer.Draw(Data);
    }

    // Text 큐: BuildRenderData()에서 이미 계산된 Instances 배열 사용
    if (!RenderQueue.IsTextRQEmpty())
    {
        for (const FDrawCommand& Data : RenderQueue.GetTextRenderQ())
        {
            // Font에서 미리 계산된 글자별 쿼드 데이터를 그대로 넘김
            Renderer.AddTextInstanceArray(Data);
        }

        // 각 DrawCommand의 머티리얼 주소가 인스턴스 배치 키에 포함되므로,
        // 첫 번째 명령만 그리면 나머지 Text 배치는 렌더링되지 않는다.
        for (const FDrawCommand& Data : RenderQueue.GetTextRenderQ())
        {
            Renderer.DrawTextInstances(Data);
        }
        Renderer.ClearTextInstances();
    }

    RenderQueue.Clear();
}

