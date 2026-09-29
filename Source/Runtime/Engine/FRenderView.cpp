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
#include "Runtime/Core/Globals.h"
#include <fstream>

#include "Runtime/CoreUObject/FStatsManager.h"

FRenderView::FRenderView(FRenderer &Renderer) : Renderer(Renderer) {}

namespace
{
    FDrawCommand GetDrawCommand(const UPrimitiveComponent& Component, const FCamera& Camera)
    {
        const FRenderData& Data = Component.GetRenderData(Camera);

        if (!Data.Mesh || Data.Materials.empty())
        {
            return {};
        }

        const bool bIsSpecial = (Data.Type == ERenderType::Text || Data.Type == ERenderType::Instancing);
        const FMatrix InitialMVP = bIsSpecial ? (Data.ModelMatrix * Camera.GetViewProjectionMatrix()) : FMatrix::Identity;

        FDrawCommand Command
        {
            .Mesh = Data.Mesh->Get(),
            .Type = Data.Type,
            .Instances = Data.Instances,
        };


        Command.Constants =
        {
            .MVP = InitialMVP,
            .Color = Data.Materials[0].Color,
            .UVScale = Data.Materials[0].UVScale,
            .UVOffset = Data.Materials[0].UVOffset,
            .World = Data.ModelMatrix,
            .DisableShading = Data.Materials[0].bDisableShading ? 1.0f : 0.0f,
        };

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

            Command.Materials.push_back(Material);
        }

        const FMaterialInstance& PrimaryMaterial = Data.Materials[0];
        

        if (Globals::bSortTest)
        {
            uint64 PipelineId = 0;
            uint64 MaterialId = 0;
            uint64 TextureId = 0;
            uint64 MeshId = static_cast<uint64>(Data.Mesh->GetID().GetHash());
            
            if (PrimaryMaterial.Pipeline)
            {
                PipelineId = static_cast<uint64>(PrimaryMaterial.Pipeline->GetID().GetHash());
            }
            
            if (PrimaryMaterial.Material)
            {
                MaterialId = static_cast<uint64>(PrimaryMaterial.Material->GetID().GetHash());
            }
            
            if (PrimaryMaterial.Texture)
            {
                TextureId = static_cast<uint64>(PrimaryMaterial.Texture->GetID().GetHash());
            }
            
            Command.RenderStateKey = (PipelineId << 48) |
                (MaterialId << 32) |
                (TextureId << 16) |
                MeshId;
            
            // AABB의 Min X 값을 Depth로 지정
            Command.Depth = Component.GetViewBounds(Camera).Min.X;
            float Near = Camera.GetProjection().GetNearPlane();
            float Far = Camera.GetProjection().GetFarPlane();
            
            Command.DepthBucket = static_cast<int32>((Command.Depth - Near) * 16 / (Far - Near));
        }

        return Command;
    }
}

void FRenderView::ReserveScratchMVPBuffer(size_t RequiredCount)
{
	if (RequiredCount <= ScratchMVPAllocated)
	{
		return;
	}
	if (ScratchMVPBuffer)
	{
		_aligned_free(ScratchMVPBuffer);
	}
	size_t NewAlloc = (RequiredCount + 3) & ~3;
	ScratchMVPBuffer = (FMatrix*)_aligned_malloc(sizeof(FMatrix) * NewAlloc, 16);
	ScratchMVPAllocated = NewAlloc;
}

void FRenderView::CollectScenePrimitives(const UScene& Scene, const FSceneView& View, const AActor* SelectedActor)
{
    const auto& SceneTransforms = Scene.GetSceneTransforms();
    const int32 TotalBatchCount = static_cast<int32>(Scene.GetActors().size());

    if (Globals::bEnableBatchTransform)
    {
        ReserveScratchMVPBuffer(TotalBatchCount);
        if (TotalBatchCount > 0)
        {
            SceneTransforms.ComputeBatchMVP(View.ViewProj, ScratchMVPBuffer, TotalBatchCount);
        }
    }

    //SCOPE_INDEPENDENT_CYCLE_COUNTER("Test");
    const TArray<UPrimitiveComponent*>& Primitives = Scene.GetRenderComponents();

    //assert(!bCullResultValid || VisibleFlags.size() == Primitives.size());

    //컬링 결과 인덱스를 맞추기 위해 인덱스 for문으로 변경
    for (size_t i = 0; i < Primitives.size(); i++)
    {
        UPrimitiveComponent* PrimitiveComponent = Primitives[i];
        if (!PrimitiveComponent) continue;

        //bCullResultValid가 false라면 통과
        // bCullResultValid가 true라면 컬링 결과 통과시에만 수집
        if (bCullResultValid && !VisibleFlags[i]) continue;
        
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

        FDrawCommand DrawCommand = GetDrawCommand(*PrimitiveComponent, View.Camera);
            
        // 인스턴싱 및 텍스트는 인스턴스 배열을 사용하므로 바로 푸시
        if (DrawCommand.Type == ERenderType::Text || DrawCommand.Type == ERenderType::Instancing)
        {
            RenderQueue.Push(DrawCommand);
            continue;
        }

		int32 Index = PrimitiveComponent->GetBatchIndex();

        if (Globals::bEnableBatchTransform && Index >= 0 && Index < TotalBatchCount && !PrimitiveComponent->Cast<UBillBoardComp>())
        {
            DrawCommand.Constants.World = SceneTransforms.WorldMatrices[Index];
            DrawCommand.Constants.MVP = ScratchMVPBuffer[Index];
        }
        else
        {
            const FMatrix World = PrimitiveComponent->GetRenderMatrix(View.Camera);
            DrawCommand.Constants.World = World;
            DrawCommand.Constants.MVP = World * View.ViewProj;
        }
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
        .Time = FTimeManager::GetTime(),
        .DeltaTime = FTimeManager::GetDeltaTime(),
    };

    Renderer.UpdateFrameConstants(FrameConstants);
}

void FRenderView::RenderView(const FSceneView& View, const UScene& Scene, const FEditorRenderContext& EditorCtx)
{


    // 뷰포트 시작
    BeginView(View);

    //컬링 측정
    {
        SCOPE_CYCLE_COUNTER("Cull");
        CullScene(View, Scene);
    }

    // 씬 컴포넌트 수집
    CollectScenePrimitives(Scene, View, EditorCtx.SelectedActor);

    if (Globals::bSortTest)
    {
        RenderQueue.Sort();
    }

    // 기본 씬 오브젝트 패스
    FlushBasePass(View.Camera);

    Renderer.ClearLastRenderStateKey();

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

    Renderer.ClearLastRenderStateKey();

    // 후처리 외곽선 패스
    RenderPostProcessPass(View.Camera, EditorCtx.SelectedActor);

    Renderer.ClearLastRenderStateKey();
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
    Constants.MVP = Camera.GetViewProjectionMatrix();
    Constants.CameraPosition = Camera.GetPosition();
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
    FlushLineBatch(Camera.GetViewProjectionMatrix());
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
        FDrawCommand Command = GetDrawCommand(*TextComp, Camera);
        if (!Command.Instances.empty())
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

    const FMatrix ModelMatrix = PrimComp->GetRenderMatrix(Camera);
    FDrawCommand DrawCommand = GetDrawCommand(*PrimComp, Camera);

    DrawCommand.Constants.DisableShading = true;
    DrawCommand.Constants.MVP = ModelMatrix * Camera.GetViewProjectionMatrix();
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

FCullingSettings& FRenderView::GetCullingSettings()
{
    return CullingSettings;
}

const FCullingSettings& FRenderView::GetCullingSettings() const
{
    return CullingSettings;
}

void FRenderView::SetCullingEnabled(bool pCullingEnable)
{
    CullingSettings.bEnabled = pCullingEnable;
}

void FRenderView::SetCullingFreeze(bool pCullingFreeze)
{
    CullingSettings.bFreeze = pCullingFreeze;
}

void FRenderView::CullScene(const FSceneView& View, const UScene& Scene)
{
    const TArray<FAxisAlignedBoundingBox>& CullDataList = Scene.GetCullDataList();
    const uint32 Count = static_cast<uint32>(CullDataList.size());

    bCullResultValid = CullingSettings.bEnabled;
    if (!bCullResultValid)
    {
        // 컬링 OFF: 전부 가시로 집계
        return;
    }

    // 매 프레임 그 프레임의 Frustum으로 전체 판정 (이전 결과 재사용 없음)
    const FFrustum Frustum = GetCullFrustum(View);
    const uint32 VisibleCount = Culler->Cull(Frustum, CullDataList, VisibleFlags);

    //Culling 결과를 카운트
    //...
}

void FRenderView::InvalidateFrozenFrustums()
{
    for (FFrozenView& Frozen : FrozenViews)
    {
        Frozen.bValid = false;
        Frozen.bHasCorners = false;
    }
}

FFrustum FRenderView::GetCullFrustum(const FSceneView& View)
{
    return FFrustum::FromViewProjection(View.ViewProj);
}

FRenderView::~FRenderView()
{
	if (ScratchMVPBuffer)
	{
		_aligned_free(ScratchMVPBuffer);
		ScratchMVPBuffer = nullptr;
	}
	ScratchMVPAllocated = 0;
}
