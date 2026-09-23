#pragma once
#include "Editor/Core/FEditor.h"

class AActor;
class USceneComponent;
class UStaticMeshComponent;
class USpotLightComponent;
class UTextInstanceComponent;
class UBillBoardComp;
class UAnimatedBillboardComp;

// 선택된 액터의 컴포넌트 속성을 편집하는 창.
class FImguiPropertyWindow final {
public:
	FImguiPropertyWindow() = default;
	~FImguiPropertyWindow() = default;

	//복사 생성 금지
	FImguiPropertyWindow(const FImguiPropertyWindow&) = delete;
	//복사 대입 금지
	FImguiPropertyWindow& operator=(const FImguiPropertyWindow&) = delete;

	void Process(FEditor& Editor);

private:
	// 액터 클래스명과 UUID.
	void ShowActorHeader(const AActor& Actor) const;

	// 루트/서브 컴포넌트를 불릿으로 나열하는 요약 트리.
	void ShowComponentHierarchy(const AActor& Actor) const;

	// 컴포넌트마다 접이식 헤더를 만들고 그 안에 상세 속성을 그린다.
	void ShowComponentSections(FEditor& Editor, AActor& Actor);
	void ShowComponentDetails(FEditor& Editor, AActor& Actor, USceneComponent& Comp, bool bIsRoot);

	// 루트는 에디터 기즈모와 동기화되고, 서브는 상대 트랜스폼을 편집한다.
	void ShowTransform(FEditor& Editor, USceneComponent& Comp, bool bIsRoot) const;

	// 컴포넌트 타입별 속성
	void ShowTextSettings(UTextInstanceComponent& TextComp) const;
	void ShowBillboardSettings(UBillBoardComp& BillboardComp) const;
	void ShowAnimatedBillboardSettings(UAnimatedBillboardComp& BillboardComp) const;
	void ShowSpotLightSettings(USpotLightComponent& LightComp) const;
	void ShowStaticMeshSettings(AActor& Actor, UStaticMeshComponent& MeshComp, bool bIsRoot) const;

	// 머티리얼의 텍스처 미리보기 겸 드롭 타깃.
	void ShowMaterialSlot(UStaticMeshComponent& MeshComp, int Slot = 0) const;
	void ShowPipelineSlot(UStaticMeshComponent& MeshComp, int Slot = 0) const;
	void ShowTextureSlot(UStaticMeshComponent& MeshComp, int Slot = 0) const;
	void ShowStaticMeshSlot(UStaticMeshComponent& MeshComp) const;

	void ShowApplyAllMaterialSlot(UStaticMeshComponent& MeshComp) const;
	void ShowApplyAllPipelineSlot(UStaticMeshComponent& MeshComp) const;
	void ShowApplyAllTextureSlot(UStaticMeshComponent& MeshComp) const;

	// 창 하단의 기즈모 모드/공간 선택.
	void ShowGizmoSettings(FEditor& Editor) const;
};
