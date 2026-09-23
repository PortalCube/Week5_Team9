#pragma once

#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/FMaterial.h"

struct FVector2;
struct FCamera;
class FRenderer;
class FEditor;

enum class EGizmoMode : uint8
{
	None = 0u,
	Translate = 1u,
	Rotate = 2u,
	Scale = 3u,
};

enum class EGizmoSpace : uint8
{
	World = 0u,
	Local = 1u,
};

enum class EGizmoHandle : uint8
{
	None = 0u,
	XAxis = 1u,
	YAxis = 2u,
	ZAxis = 3u,
};

class FGizmo final
{
public:
	void Initialize();

	void Draw(FRenderer& Renderer, const FTransform& Transform, const FCamera& Camera) const;

	[[nodiscard]] EGizmoHandle HitTest(const FTransform& Transform, const FRay& Ray, const FCamera& Camera) const;
	
	void BeginInteraction(const FTransform& Transform, EGizmoHandle Handle, const FVector2& MousePosition, const FCamera& Camera, const FVector2& ViewportSize);
	void UpdateInteraction(FEditor& Editor, const FVector2& MousePosition);
	void EndInteraction();
	[[nodiscard]] bool IsInteracting() const { return ActiveHandle != EGizmoHandle::None; }
	[[nodiscard]] EGizmoSpace GetSpace() const { return ModeSpace[static_cast<uint8>(Mode)]; }
	void SetGizmoSpace(EGizmoSpace Space) { ModeSpace[static_cast<uint8>(Mode)] = Space; }

	EGizmoMode Mode = EGizmoMode::Translate;

	EGizmoHandle HoveredHandle = EGizmoHandle::None;
	EGizmoHandle ActiveHandle = EGizmoHandle::None;

private:
	void DrawAxis(FRenderer& Renderer, EGizmoHandle Handle, const FMatrix& MVP) const;
	[[nodiscard]] float CalculateGizmoScale(const FVector& GizmoLocation, const FCamera& Camera) const;
	[[nodiscard]] FVector2 WorldToViewport(const FVector& WorldPosition, const FCamera& Camera, const FVector2& ViewportSize) const;

private:
	TSharedPtr<FMesh> ArrowMesh;
	TSharedPtr<FMesh> CircleMesh;
	TSharedPtr<FMesh> SquareArrowMesh;
	TSharedPtr<FMaterial> Material;

	TArray<EGizmoSpace> ModeSpace = {
		EGizmoSpace::World, // None
		EGizmoSpace::World, // Translate
		EGizmoSpace::World, // Rotate
		EGizmoSpace::Local, // Scale
	};

	FTransform InteractionStartTransform;
	FVector InteractionAxisWorld;
	FVector InteractionAxisLocal;
	FVector2 InteractionAxisViewport;
	FVector2 InteractionStartMouse;
	FVector2 InteractionOriginViewport;
	float InteractionRotationSign = 1.0f;
	float InteractionWorldUnitsPerPixel = 0.0f;
};
