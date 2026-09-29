#pragma once

#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/Geometry/FTransform.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/FRenderQueue.h"
#include "Runtime/Engine/FSceneView.h"
#include "Runtime/Geometry/FFrustum.h"
#include "Runtime/Engine/FCulling.h"
#include "Runtime/Engine/FOcclusionCuller.h"

class FCamera;
class FGizmo;
class FGrid;
class AActor;
class UScene;

// 커맨드로 제어하는 컬링 옵션
struct FCullingSettings
{
	bool bEnabled = true;   // cull on/off
	bool bFreeze = false;   // cull freeze (Frustum 고정)
};

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

	FCullingSettings& GetCullingSettings();
	const FCullingSettings& GetCullingSettings() const;

	void SetCullingEnabled(bool pCullingEnable);
	void SetCullingFreeze(bool pCullingFreeze);

	//렌더 전에 컬링 판정
	void CullScene(const FSceneView& View, const UScene& Scene);

	void SetOcclusionEnabled(bool bEnable) { bOcclusionEnabled = bEnable; }
	bool IsOcclusionEnabled() const { return bOcclusionEnabled; }
	FOcclusionCuller& GetOcclusionCuller() { return OcclusionCuller; }

	//측정 : 다음에 렌더되는 뷰 하나에서 오라클을 실행(한 프레임 멈춤)
	void RequestOcclusionOracle() { bOracleRequested = true; }

private:
	FCullingSettings CullingSettings;
	//컬링 후 가시 여부 인덱스(실제 renderComponent 인덱스와 동일하게)
	TArray<uint8> VisibleFlags;
	bool bCullResultValid = false;

	static constexpr uint32 MaxViewCount = 4;   // FEditor::Leaf 개수

	struct FFrozenView
	{
		FFrustum Frustum;
		FMatrix ViewProj;
		FVector Corners[8];         // 와이어프레임용 (월드 좌표)
		bool bValid = false;
		bool bHasCorners = false;
	};

	FFrustum GetCullFrustum(const FSceneView& View);

	FFlatFrustumCuller FlatCuller;
	IPrimitiveCuller* Culler = &FlatCuller;     // 추후 BVH/SIMD 컬러로 교체하는 지점

	//Occlusion Culling
	FOcclusionCuller OcclusionCuller;
	bool bOcclusionEnabled = false;

	// [SceneIndex] 오클루전으로 지웠으면 1
	TArray<uint8> OccludedFlags;

	bool bOracleRequested = false;
	TArray<FDrawCommand> OracleDrawnCommands;     // 그린 것
	TArray<FDrawCommand> OracleOccludedCommands;  // 오클루전으로 지운 것 (검증 대상)

	void RunOcclusionOracle();
};
