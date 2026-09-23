#pragma once
#include "Runtime/Math/FVector2.h"
#include "Runtime/Engine/FCamera.h"
#include "Editor/Grid/FGrid.h"

#include "Runtime/Engine/ShowFlags.h"
class FEditorViewportClient final {
	bool bFocused = false;
	bool bHovered = false;
	FGrid Grid;
	//Grid 이식중, ShowFlag 추가필요

public:
	// Type에 따라 키보드,마우스 조작이 달라지기 때문에 ViewportClient에 있어야 한다고 생각함
	enum class EOrthogonalType {
		PERSPECTIVE,
		ORTHOGRAPHIC,
		ORTHOGRAPHIC_TOP,
		ORTHOGRAPHIC_BOTTOM,
		ORTHOGRAPHIC_LEFT,
		ORTHOGRAPHIC_RIGHT,
		ORTHOGRAPHIC_FRONT,
		ORTHOGRAPHIC_BACK,
	} eOrthogonalType = EOrthogonalType::PERSPECTIVE; 
	void SetOrthograpihcView(EOrthogonalType type);
	
	FCamera ViewportCamera;
	// 전체 클라이언트 영역 기준 고정 UV: 좌상단 (0,0), 우하단 (1,1).
	// 픽셀 위치/크기는 사용할 때 클라이언트 크기를 곱해 계산한다.
	FVector2 TopLeftUV = { 0.0f, 0.0f };
	FVector2 LengthUV = { 1.0f, 1.0f };

	// 뷰포트 렌더 모드 및 쇼 플래그
	EViewModeIndex ViewMode = EViewModeIndex::VMI_Lit;
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
	                   static_cast<uint64>(EEngineShowFlags::SF_BillboardText) |
					   static_cast<uint64>(EEngineShowFlags::SF_Grid);

	
	
	
	
	FGrid& GetGrid() { return Grid; }
	void UpdateFocusedAndHovered(bool bFocused, bool bHovered);
	const FGrid& GetGrid() const { return Grid; }

	[[nodiscard]] bool HasShowFlag(EEngineShowFlags Flag) const {
		return (ShowFlags & static_cast<uint64>(Flag)) != 0;
	}
	
	void ToggleShowFlag(EEngineShowFlags Flag) {
		ShowFlags ^= static_cast<uint64>(Flag);
	}

	[[nodiscard]] bool IsFocused() const { return bFocused; }
	[[nodiscard]] bool IsHovered() const { return bHovered; }
	void Update();

};
