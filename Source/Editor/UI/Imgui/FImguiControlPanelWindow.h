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

};
