#pragma once

#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/Geometry/FTransform.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/FRenderQueue.h"
#include "Runtime/Engine/FSceneView.h"

struct FCamera;
class FGizmo;
class FGrid;
class AActor;
class UScene;

class FRenderView final {
	FRenderer& Renderer;
	FRenderQueue RenderQueue;

public:
	FRenderView(FRenderer& Renderer);
	FRenderer& GetRenderer() { return Renderer; }
	const FRenderer& GetRenderer() const { return Renderer; }
	FRenderView(const FRenderView&) = delete;
	FRenderView& operator=(const FRenderView&) = delete;
	
	// 전체 렌더링 준비
	void PrepareRender();

	// 전체 뷰포트 렌더링
	void RenderView(const FSceneView& View, const UScene& Scene, const FEditorRenderContext& EditorCtx);
	void CollectScenePrimitives(const UScene& Scene, const FSceneView& View, const AActor* SelectedActor);

	// 뷰포트 패스 파이프라인
	void BeginView(const FSceneView& View);
	void DrawGrid(const FCamera& Camera, FGrid& Grid);
	void FlushBasePass(const FCamera& Camera);
	void FlushLinePass(const FCamera& Camera);
	void RenderPostProcessPass(const FCamera& Camera, const AActor* SelectedActor);
	void RenderOverlayPass(const FCamera& Camera, const FSceneView& SceneView, const FTransform& SelectedTransform, const FGizmo& Gizmo, UTextInstanceComponent* TextComp);

	// 개별 렌더 및 디버그 라인
	void RenderGizmo(const FTransform& Transform, const FCamera& Camera, FVector2 TopLeftUV, FVector2 LengthUV, const FGizmo& Gizmo);

	void RenderLine(const FVector& Start, const FVector& End, const FVector4& Color);
	void RenderBoxCenterExtent(const FVector& Center, const FVector& Extent, const FVector4& Color);
	void RenderBoxMinMax(const FVector& Min, const FVector& Max, const FVector4& Color);
	void RenderQuad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector4& Color);
	void RenderSphere(const FVector& Center, float Radius, const FVector4& Color, uint32 Segments = 16);

	void RenderOutline(const FCamera& Camera, const AActor* SelectedActor);
	void DrawStencilMask(const FCamera& Camera, const AActor* SelectedActor);
	void RenderVerticetoline();

	void SetRenderMode(EViewModeIndex InMode);
	void UpdateLightConstants(const FLightConstants& Constants, const EViewModeIndex InMode);
	void DrawInstances(const FCamera& Camera);
	void ClearTextInstances();
	void FlushLineBatch(const FMatrix& ViewProjection, const FName& PipelineId = FName("Simple_Line"));
	void FlushQueue(const FCamera& Camera);

	FRenderQueue& GetRenderQueue() { return RenderQueue; }
	const FRenderQueue& GetRenderQueue() const { return RenderQueue; }
};
