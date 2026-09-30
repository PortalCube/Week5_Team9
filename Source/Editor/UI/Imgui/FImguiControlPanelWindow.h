#pragma once
#include "Editor/Core/FEditor.h"

class FImguiControlPanelWindow final {
public:
	FImguiControlPanelWindow() = default;
	~FImguiControlPanelWindow() = default;

	//복사 생성 금지
	FImguiControlPanelWindow(const FImguiControlPanelWindow&) = delete;
	//복사 대입 금지
	FImguiControlPanelWindow& operator=(const FImguiControlPanelWindow&) = delete;

	void Process(FEditor& Editor);
private:
	void ActorSpawnSetting(FEditor& Editor);
	void GridSetting(FEditor& Editor);
	void RenderModeAndShowFlagSetting(FEditor& Editor);
	void CameraSetting(FEditor& Editor);
	//TODO : Directional light또한 Actor가 되어야하므로 지워야함
	void DirectionLightSetting(FEditor& Editor);
	void BVHDebugSetting(FEditor& Editor);
	// 마지막 피킹 광선으로 Iterations번 반복 측정해 중앙값/최솟값/평균과 작업량을 로그로 출력한다.
	void RunPickBenchmark(FEditor& Editor, int Iterations);
	// 마지막 피킹 광선을 파일로 저장/불러오기. 재빌드 전후를 같은 광선으로 비교하기 위함.
	void SavePickRay();
	void LoadPickRay();
	static constexpr const char* PickRayFilePath = "PickBenchRay.txt";
	void RenderStateSort(FEditor& Editor);
	void FrameResourceDebugSetting(FEditor& Editor);
	void LODSetting(FEditor& Editor);
};
