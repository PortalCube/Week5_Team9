#pragma once
#include "Editor/Core/FEditor.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Input/FCameraInputController.h"
#include "ThirdParty/Imgui/imgui.h"



// 3D 씬 위를 덮는 투명한 ImGui 창.
// - 다른 패널(ControlPanel, Property 등)이 이 창 위에 그려지므로,
//   ImGui 의 hover/active 판정이 "다른 패널에 가려지지 않은 뷰포트 영역"만 걸러준다.
// - 창의 위치와 크기를 활성 뷰포트의 UV로 되돌려주고 focus/hover 상태를 갱신한다.
// - 뷰포트 위에서 클릭이 발생하면 피킹을 수행한다.
class FImguiEditorViewportWindow final
{
	
public:
	enum class EStatsWindow
	{
		Memory,
		FPS
	};


	FImguiEditorViewportWindow() = default;
	~FImguiEditorViewportWindow() = default;

	//복사 생성 금지
	FImguiEditorViewportWindow(const FImguiEditorViewportWindow&) = delete;
	//복사 대입 금지
	FImguiEditorViewportWindow& operator=(const FImguiEditorViewportWindow&) = delete;


	void Process(FEditor& Editor, float DeltaTime);

	void SetOpen(EStatsWindow Window, bool bOpen) {
		switch (Window) {
		case EStatsWindow::Memory:
			bOpenMemory = bOpen;
			break;

		case EStatsWindow::FPS:
			bOpenFPS = bOpen;
			break;
		}
	}

	void SetClose() {
		bOpenMemory = false;
		bOpenFPS = false;
	}

private:
	// 이 프레임의 뷰포트 입력 상태.
	// 피킹·기즈모·카메라가 모두 같은 값을 보므로 한 번만 모아서 넘긴다.
	struct FViewportInput
	{
		// 뷰포트 좌상단 기준 마우스 좌표(픽셀)
		FVector2 LocalMouse{};
		// 뷰포트 크기(픽셀)
		FVector2 SizePixels{};

		bool bHovered = false;
		bool bFocused = false;
		bool bPickRequested = false;
		bool bLeftDown = false;
		bool bLeftReleased = false;
	};

	// ImGui 창을 열고 스타일을 적용한다. Process 가 EndWindow 로 짝을 맞춘다.
	void BeginWindow() const;
	void EndWindow() const;

	// ImGui 창의 실제 사각형을 뷰포트 UV 와 종횡비에 반영한다.
	// 사용자가 창을 옮기거나 크기를 바꾸면 3D 렌더 영역이 따라간다.
	void SyncViewportRect(FEditorViewportClient& Viewport, const FRect& Rect, const FVector2& ClientSize) const;

	// 창 전체를 덮는 클릭 판정용 아이템을 만들고 입력 상태를 모은다.
	FViewportInput GatherInput(const FVector2& ViewportTopLeftPixels,
		const FVector2& ViewportSizePixels) const;

	// 창이 작업 영역 위로 올라가 타이틀바에 가리는 것을 막는다.
	void ClampWindowToWorkArea() const;

	void UpdateSelection(FEditor& Editor, const FEditorViewportClient& Viewport,
		const FViewportInput& Input);
	void UpdateGizmo(FEditor& Editor, const FEditorViewportClient& Viewport,
		const FViewportInput& Input);
	void UpdateCamera(FEditor& Editor, FEditorViewportClient& Viewport,
		const FViewportInput& Input, float DeltaTime);
	void UpdateShortcuts(FEditor& Editor) const;

	void HandlePicking(FEditor& Editor, const FEditorViewportClient& Viewport,
		const FVector2& LocalMousePixels, const FVector2& ViewportSizePixels);
	void UpdateGizmoHover(FEditor& Editor, const FEditorViewportClient& Viewport,
		const FVector2& LocalMousePixels, const FVector2& ViewportSizePixels);
	void ShowViewportVerticalSplitter(SSplitter& Splitter);
	void ShowViewportHorizontalSplitter(SSplitter& Splitter);
	void ApplyPendingViewportMaximize(FEditor& Editor);

	// 스탯 드로우
	void DrawRow(ImDrawList* DrawList, const ImVec2& Pos, float& Y,
		const float& Width, const float& RowHeight,
		const float& ValueOffsetX,
		const char* Name, const char* Value, double Data,
		FVector4 TextColor, FVector4 RowColor);
	void DrawStatsMemory();
	void DrawGPUStatsMemory();
	void DrawStatsFPS();
	bool GetViewportSceneRect(const ImVec2& Origin, FRect& OutRect) const;
	void DrawViewportHeader(int32 ViewportIndex,FEditor& Editor);
	float DT = 1.0f;
	bool bOpenMemory = false;
	bool bOpenFPS = false;

	float CpuY = 0;
	float GpuY = 0;
	int32 PendingMaximizeViewport = -1;
	FCameraInputController CameraController;
};
