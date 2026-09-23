#pragma once
#include "Editor/Core/FEditor.h"



class FImguiStatsWindow final
{
	
public:
	enum class EStatsWindow
	{
		Memory,
		FPS
	};

	FImguiStatsWindow() = default;
	~FImguiStatsWindow() = default;

	FImguiStatsWindow(const FImguiStatsWindow&) = delete;
	FImguiStatsWindow& operator=(const FImguiStatsWindow&) = delete;

	void Process(FEditor& Editor, float DeltaTime);



	void SetOpen(EStatsWindow Window, bool bOpen)
	{
		switch (Window)
		{
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
	// bool IsOpen() const { return bOpen; }

	// Stat Memory, FPS 구분 필요
	void DrawMemory();
	void DrawFPS();

private:
	bool bOpenMemory = false;
	bool bOpenFPS = false;

	float DeltaTime = 0.0f;
};

