#include "FGizmo.h"

#include "Runtime/Core/IntTypes.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector4.h"
#include "Editor/Core/FEditor.h"
#include <numbers>

#include "Runtime/Engine/FRayCastingManager.h"

void FGizmo::Initialize()
{
	auto& RenderResources = FRenderResourceLibrary::Get();
	ArrowMesh = RenderResources.GetMesh(FName("#Arrow"));
	CircleMesh = RenderResources.GetMesh(FName("#Circle"));
	SquareArrowMesh = RenderResources.GetMesh(FName("#SquareArrow"));

	Material = RenderResources.GetMaterial(FName("Gizmo"));
}

void FGizmo::Draw(FRenderer& Renderer, const FTransform& Transform, const FCamera& Camera) const
{
	static FMatrix YAxisRotation = FMatrix::MakeRotationZ(std::numbers::pi_v<float> * 0.5f);
	static FMatrix ZAxisRotation = FMatrix::MakeRotationY(std::numbers::pi_v<float> * 0.5f);

	float GizmoScale = CalculateGizmoScale(Transform.Location, Camera);
	FMatrix Scale = FMatrix::MakeScale(FVector{ GizmoScale, GizmoScale, GizmoScale });
	FMatrix ObjectRotation = GetSpace() == EGizmoSpace::World ? FMatrix::GetIdentity() : Transform.Rotation.ToMatrixRow();
	FMatrix Translation = FMatrix::MakeTranslation(Transform.Location);
	FMatrix VP = Camera.CreateViewProjectionMatrix();
	 
	DrawAxis(Renderer, EGizmoHandle::XAxis, Scale * ObjectRotation * Translation * VP);
	DrawAxis(Renderer, EGizmoHandle::YAxis, Scale * YAxisRotation * ObjectRotation * Translation * VP);
	DrawAxis(Renderer, EGizmoHandle::ZAxis, Scale * ZAxisRotation * ObjectRotation * Translation * VP);
}

EGizmoHandle FGizmo::HitTest(const FTransform& Transform, const FRay& Ray, const FCamera& Camera) const
{
	float GizmoScale = CalculateGizmoScale(Transform.Location, Camera);

	static FMatrix YAxisRotation = FMatrix::MakeRotationZ(std::numbers::pi_v<float> * 0.5f);
	static FMatrix ZAxisRotation = FMatrix::MakeRotationY(std::numbers::pi_v<float> * 0.5f);

	FMatrix Scale = FMatrix::MakeScale(FVector{ GizmoScale, GizmoScale, GizmoScale });
	FMatrix ObjectRotation = GetSpace() == EGizmoSpace::World ? FMatrix::GetIdentity() : Transform.Rotation.ToMatrixRow();
	FMatrix Translation = FMatrix::MakeTranslation(Transform.Location);

	TSharedPtr<FMesh> GizmoMesh;
	switch (Mode)
	{
	case EGizmoMode::Translate:
		GizmoMesh = ArrowMesh;
		break;

	case EGizmoMode::Rotate:
		GizmoMesh = CircleMesh;
		break;

	case EGizmoMode::Scale:
		GizmoMesh = SquareArrowMesh;
		break;

	case EGizmoMode::None:
		return EGizmoHandle::None;
	}

	float ClosestDistance = (std::numeric_limits<float>::max)();
	EGizmoHandle ClosestHandle = EGizmoHandle::None;

	float HitDistance;
	FVector ImpactPoint;
	if (FRayCastingManager::RayIntersectsMesh(
			Ray,
			*GizmoMesh,
			Scale * ObjectRotation * Translation,
			HitDistance,
			ImpactPoint) &&
		HitDistance < ClosestDistance)
	{
		ClosestDistance = HitDistance;
		ClosestHandle = EGizmoHandle::XAxis;
	}
	if (FRayCastingManager::RayIntersectsMesh(
		Ray,
		*GizmoMesh,
		Scale * YAxisRotation * ObjectRotation * Translation,
		HitDistance,
		ImpactPoint) &&
		HitDistance < ClosestDistance)
	{
		ClosestDistance = HitDistance;
		ClosestHandle = EGizmoHandle::YAxis;
	}
	if (FRayCastingManager::RayIntersectsMesh(
		Ray,
		*GizmoMesh,
		Scale * ZAxisRotation * ObjectRotation * Translation,
		HitDistance,
		ImpactPoint) &&
		HitDistance < ClosestDistance)
	{
		ClosestDistance = HitDistance;
		ClosestHandle = EGizmoHandle::ZAxis;
	}

	return ClosestHandle;
}

void FGizmo::BeginInteraction(const FTransform& Transform, EGizmoHandle Handle, const FVector2& MousePosition, const FCamera& Camera, const FVector2& ViewportSize)
{
	switch (Handle)
	{
	case EGizmoHandle::XAxis:
		InteractionAxisLocal = FVector{ 1.0f, 0.0f, 0.0f };
		break;
	case EGizmoHandle::YAxis:
		InteractionAxisLocal = FVector{ 0.0f, 1.0f, 0.0f };
		break;
	case EGizmoHandle::ZAxis:
		InteractionAxisLocal = FVector{ 0.0f, 0.0f, 1.0f };
		break;
	case EGizmoHandle::None:
		return;
	}
	InteractionAxisWorld = GetSpace() == EGizmoSpace::World ? InteractionAxisLocal : Transform.Rotation.RotateVector(InteractionAxisLocal);

	InteractionStartTransform = Transform;
	InteractionStartMouse = MousePosition;

	float GizmoScale = CalculateGizmoScale(Transform.Location, Camera);

	FVector OriginWorld = Transform.Location;
	FVector AxisEndWorld = OriginWorld + InteractionAxisWorld * GizmoScale;

	FVector2 OriginViewport = WorldToViewport(OriginWorld, Camera, ViewportSize);
	FVector2 AxisEndViewport = WorldToViewport(AxisEndWorld, Camera, ViewportSize);

	FVector2 AxisViewport = AxisEndViewport - OriginViewport;
	float AxisViewportLength = AxisViewport.Size();

	InteractionOriginViewport = OriginViewport;

	FVector CenterToCamera = Camera.Position - OriginWorld;
	InteractionRotationSign = (CenterToCamera.Dot(InteractionAxisWorld) <= 0.0f) ? 1.0f : -1.0f;

	if (AxisViewportLength > 1e-5f)
	{
		InteractionAxisViewport = AxisViewport / AxisViewportLength;
		InteractionWorldUnitsPerPixel = GizmoScale / AxisViewportLength;
		ActiveHandle = Handle;
	}
}

void FGizmo::UpdateInteraction(FEditor& Editor, const FVector2& MousePosition)
{
	if (!Editor.ObjectSelected() || ActiveHandle == EGizmoHandle::None)
	{
		return;
	}

	FVector2 MouseDelta = MousePosition - InteractionStartMouse;
	float ViewportDistance = MouseDelta.Dot(InteractionAxisViewport);
	float WorldDistance = ViewportDistance * InteractionWorldUnitsPerPixel;

	switch (Mode)
	{
	case EGizmoMode::Translate:
		Editor.SelectedTransform.Location = InteractionStartTransform.Location + InteractionAxisWorld * WorldDistance;
		break;
		
	case EGizmoMode::Rotate:
	{
		FVector2 BA = InteractionStartMouse - InteractionOriginViewport;
		FVector2 BC = MousePosition - InteractionOriginViewport;
		float Theta = (std::atan2f(BA.Y, BA.X) - std::atan2f(BC.Y, BC.X)) * InteractionRotationSign * 180.0f / std::numbers::pi_v<float>;
		if (GetSpace() == EGizmoSpace::World)
		{
			FQuaternion Delta = FQuaternion::FromAxisAngle(InteractionAxisWorld, Theta);
			Editor.SelectedTransform.Rotation = Delta * InteractionStartTransform.Rotation;
		}
		else
		{
			FQuaternion Delta = FQuaternion::FromAxisAngle(InteractionAxisLocal, Theta);
			Editor.SelectedTransform.Rotation = InteractionStartTransform.Rotation * Delta;
		}
		Editor.SelectedEulerDegDisplay = Editor.SelectedTransform.Rotation.GetEulerXYZ() * 180.0f / std::numbers::pi_v<float>;
		break;
	}

	case EGizmoMode::Scale:
		Editor.SelectedTransform.Scale3D = InteractionStartTransform.Scale3D + InteractionAxisLocal * WorldDistance;
		break;

	case EGizmoMode::None:
		return;
	}
}

void FGizmo::EndInteraction()
{
	ActiveHandle = EGizmoHandle::None;
}

void FGizmo::DrawAxis(FRenderer& Renderer, EGizmoHandle Handle, const FMatrix& MVP) const
{
	constexpr FVector4 Color[3] = {
		{ 0.8f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.8f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 0.8f, 1.0f },
	};

	constexpr FVector4 ActiveColor { 1.0f, 1.0f, 0.1f, 1.0f };
	constexpr FVector4 HoverColor { 0.7f, 0.7f, 0.0f, 1.0f };

	TSharedPtr<FMesh> GizmoMesh;
	TSharedPtr<FMaterial> GizmoMaterial;
	switch (Mode)
	{
	case EGizmoMode::Translate:
		GizmoMesh = ArrowMesh;
		GizmoMaterial = Material;
		break;
	case EGizmoMode::Rotate:
		GizmoMesh = CircleMesh;
		GizmoMaterial = Material;
		break;
	case EGizmoMode::Scale:
		GizmoMesh = SquareArrowMesh;
		GizmoMaterial = Material;
		break;
	case EGizmoMode::None:
		return;
	}

	FVector DrawColor = Color[static_cast<uint8>(Handle) - 1];
	if (ActiveHandle == Handle)
	{
		DrawColor = ActiveColor;
	}
	else if (HoveredHandle == Handle && ActiveHandle == EGizmoHandle::None)
	{
		DrawColor = HoverColor;
	}

	FObjectConstants Constants{};
	Constants.MVP = MVP;
	Constants.Color = DrawColor;
	Constants.DisableShading = 1.0f;
	Renderer.Draw(*GizmoMesh, *GizmoMaterial, Constants);
}

float FGizmo::CalculateGizmoScale(const FVector& GizmoLocation, const FCamera& Camera) const
{
	constexpr float ScalePerDistance = 0.15f;

	// 직교투영은 거리가 화면상 크기에 영향을 주지 않는다.
	// 거리를 곱하면 멀어질수록 기즈모가 커지므로, 뷰 높이를 기준으로 삼는다.
	if (Camera.Projection.ProjectionType == EProjectionType::Orthographic)
	{

		constexpr float ScalePerViewHeight = 0.15f;
		return Camera.Projection.Height * ScalePerViewHeight;
	}

	FVector ToTarget = GizmoLocation - Camera.Position;
	return ToTarget.Size() * ScalePerDistance;
}

FVector2 FGizmo::WorldToViewport(const FVector& WorldPosition, const FCamera& Camera,
	const FVector2& ViewportSize) const
{
	FMatrix VP = Camera.CreateViewProjectionMatrix();

	FVector Projected = VP.TransformPointRow(WorldPosition);

	return FVector2{
		(Projected.Y + 1.0f) * 0.5f * ViewportSize.X,
		(1.0f - Projected.Z) * 0.5f * ViewportSize.Y
	};
}

