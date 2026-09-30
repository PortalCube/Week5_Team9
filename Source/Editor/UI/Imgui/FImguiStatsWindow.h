#pragma once
#include "Editor/Core/FEditor.h"
#include "ThirdParty/Imgui/imgui.h"
#include <vector>


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

	// 패널을 켜고 끈다. Cycle/Counter 스탯 수집도 같이 따라간다.
	void Toggle(EStatsWindow Window);
	void SetClose();

	// 패널 상태에 맞춰 스탯 수집 여부를 갱신한다.
	void RefreshCollecting();

private:
	struct ScrollingBuffer
	{
		explicit ScrollingBuffer(size_t InMaxSize = 256);

		void AddPoint(float X, float Y);
		void Reset();
		bool IsEmpty() const { return Data.empty(); }

		std::vector<ImVec2> Data;
		size_t MaxSize = 0;
		size_t Offset = 0;
	};

	double GetStat(const FName& Name, size_t Range = 1) const;
	void UpdateFPSHistory(float DeltaTime);
	void ResetFPSHistory();
	void DrawStatsMemory();
	void DrawGPUStatsMemory();
	void DrawStatsFPSGraph();
	void DrawStatsFPS(float DeltaTime);
	void DrawUnits();
	void DrawRow(ImDrawList* DrawList, const ImVec2& Pos, float& Y, const float& Width, const float& RowHeight, const float& ValueOffsetX, const char* Name, const char* Value, double Data, FVector4 Color, FVector4 RowColor);

	bool bOpenMemory = false;
	bool bOpenFPS = false;
	bool bOpenUnit = false;
	bool bHasSmoothedFPS = false;

	float CpuY = 0;
	float GpuY = 0;
	float FPSHistoryTime = 0.0f;
	float FPSSampleAccumulator = 0.0f;
	float SmoothedFPS = 0.0f;
	ScrollingBuffer FPSHistory;
};

