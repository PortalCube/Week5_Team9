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
	ViewportCamera.SetProjectionType(EProjectionType::Orthographic);
	switch (type)
	{
	case EOrthogonalType::ORTHOGRAPHIC_TOP:
		ViewportCamera.SetPosition(FVector(0.0f, 0.0f, distance));
		ViewportCamera.SetRotation(-90.0f, 0.0f);
		break;
	case EOrthogonalType::ORTHOGRAPHIC_BOTTOM:
		ViewportCamera.SetPosition(FVector(0.0f, 0.0f, -distance));
		ViewportCamera.SetRotation(90.0f, 0.0f);
		break;
	case EOrthogonalType::ORTHOGRAPHIC_LEFT:
		ViewportCamera.SetPosition(FVector(0.0f, -distance, 0.0f));
		ViewportCamera.SetRotation(0.0f, 90.0f);
		break;

	case EOrthogonalType::ORTHOGRAPHIC_RIGHT:
		ViewportCamera.SetPosition(FVector(0.0f, distance, 0.0f));
		ViewportCamera.SetRotation(0.0f, -90.0f);
		break;

	case EOrthogonalType::ORTHOGRAPHIC_FRONT:
		ViewportCamera.SetPosition(FVector(distance, 0.0f, 0.0f));
		ViewportCamera.SetRotation(0.0f, 0.0f);
		break;

	case EOrthogonalType::ORTHOGRAPHIC_BACK:
		ViewportCamera.SetPosition(FVector(-distance, 0.0f, 0.0f));
		ViewportCamera.SetRotation(0.0f, 180.0f);
		break;
	}
}
