#pragma once
#include "Editor/Core/FEditor.h"
#include "ThirdParty/Imgui/imgui.h"


class FImguiStatsWindow final
{
	
public:
	enum class EStatsWindow
	{
		Memory,
		FPS,
		Unit
	};

	FImguiStatsWindow() = default;
	~FImguiStatsWindow() = default;

	FImguiStatsWindow(const FImguiStatsWindow&) = delete;
	FImguiStatsWindow& operator=(const FImguiStatsWindow&) = delete;

	void Process(FEditor& Editor, float DeltaTime);

	void DrawStatsMemory();

	void DrawGPUStatsMemory();
	void DrawStatsFPS();
	void DrawUnits();
	void DrawRow(ImDrawList* DrawList, const ImVec2& Pos, float& Y, const float& Width, const float& RowHeight, const float& ValueOffsetX, const char* Name, const char* Value, double Data, FVector4 Color, FVector4 RowColor);



	// 패널을 켜고 끈다. Cycle/Counter 스탯 수집도 같이 따라간다.
	void Toggle(EStatsWindow Window);
	void SetClose();

	// 패널 상태에 맞춰 스탯 수집 여부를 갱신한다.
	void RefreshCollecting();

private:
	bool bOpenMemory = false;
	bool bOpenFPS = false;
	bool bOpenUnit = false;

	float CpuY = 0;
	float GpuY = 0;
};

