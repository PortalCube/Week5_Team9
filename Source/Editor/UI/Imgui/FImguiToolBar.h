#pragma once
#include "FImguiConsoleWindow.h"
#include "FImguiControlPanelWindow.h"
#include "FImguiEditorViewportWindow.h"
#include "FImguiPropertyWindow.h"

class FImguiToolbar final 
{


public:
	FImguiToolbar() = default;
	~FImguiToolbar() = default;

	//복사 생성 금지
	FImguiToolbar(const FImguiToolbar&) = delete;
	//복사 대입 금지
	FImguiToolbar& operator=(const FImguiToolbar&) = delete;

	void Process(FEditor& Editor, FImguiConsoleWindow& ConsoleWindow,
		FImguiControlPanelWindow& ControlPanelWindow,
		FImguiPropertyWindow& PropertyWindow);

	FString ToNarrow(const wchar_t* Wide);
	bool PickSceneFile(FString& OutPath, bool bSave);
	void ShowFileBar(FString CurrentScenePath, FEditor& Editor);
	void ShowViewBar(FEditor& Editor, FImguiConsoleWindow& ConsoleWindow);	

	bool PickObjFile(FString& OutPath);

};