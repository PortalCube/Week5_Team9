#include "FEditorViewportClient.h"

void FEditorViewportClient::UpdateFocusedAndHovered(bool bFocused, bool bHovered)
{
	this->bFocused = bFocused; this->bHovered = bHovered;
	return;
}
void FEditorViewportClient::SetOrthograpihcView(FEditorViewportClient::EOrthogonalType type)
{
	float distance = 5.0f;
	eOrthogonalType = type;
	ViewportCamera.Projection.ProjectionType = EProjectionType::Orthographic;
	switch (type)
	{
	case EOrthogonalType::ORTHOGRAPHIC_TOP:
		ViewportCamera.Position = FVector( 0.0f,0.0f,distance );
		ViewportCamera.Pitch = -90.0f;
		ViewportCamera.Yaw = 0.0f;
		break;
	case EOrthogonalType::ORTHOGRAPHIC_BOTTOM:
		ViewportCamera.Position = FVector(0.0f, 0.0f, -distance);
		ViewportCamera.Pitch = 90.0f;
		ViewportCamera.Yaw = 0.0f;
		break;
	case EOrthogonalType::ORTHOGRAPHIC_LEFT:
		ViewportCamera.Position = FVector(0.0f, -distance, 0.0f);
		ViewportCamera.Pitch = 0.0f;
		ViewportCamera.Yaw = 90.0f;
		break;

	case EOrthogonalType::ORTHOGRAPHIC_RIGHT:
		ViewportCamera.Position = FVector(0.0f, distance, 0.0f);
		ViewportCamera.Pitch = 0.0;
		ViewportCamera.Yaw = -90.0f;
		break;

	case EOrthogonalType::ORTHOGRAPHIC_FRONT:
		ViewportCamera.Position = FVector(distance, 0.0f, 0.0f);
		ViewportCamera.Pitch = 0.0f;
		ViewportCamera.Yaw = 0.0f;
		break;

	case EOrthogonalType::ORTHOGRAPHIC_BACK:
		ViewportCamera.Position = FVector(-distance, 0.0f, 0.0f);
		ViewportCamera.Pitch = 0.0f;
		ViewportCamera.Yaw = 180.0f;
		break;
	}
}
