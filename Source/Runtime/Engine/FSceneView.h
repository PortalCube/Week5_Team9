#pragma once
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Geometry/FTransform.h"
class AActor;
class FGizmo;
class FGrid;
class FVisualizerRegistry;
class UPrimitiveComponent;
class UTextInstanceComponent;

// 뷰포트 렌더링 명세
struct FSceneView
{
	const FCamera& Camera;
	FMatrix  ViewProj;
	FVector2 TopLeftUV;
	FVector2 LengthUV;
	EViewModeIndex ViewMode = EViewModeIndex::VMI_Lit;
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives);
	FLightConstants LightConstants{};
};

// 에디터 렌더링 컨텍스트
struct FEditorRenderContext
{
	const AActor* SelectedActor = nullptr;
	UPrimitiveComponent* SelectedPrimitive = nullptr;
	FGrid* Grid = nullptr;
	FVisualizerRegistry* VisualizerRegistry = nullptr;
	FTransform SelectedTransform;
	const FGizmo* Gizmo = nullptr;
	UTextInstanceComponent* TextComp = nullptr;
};
