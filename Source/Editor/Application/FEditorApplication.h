#pragma once
#include "Editor/Core/FEditor.h"
#include "Editor/UI/Imgui/FImguiManager.h"
#include "Editor/UI/Imgui/FImguiToolBar.h"
#include "Editor/UI/Imgui/FImguiPropertyWindow.h"
#include "Editor/UI/Imgui/FImguiEditorViewportWindow.h"
#include "Editor/UI/Imgui/FImguiControlPanelWindow.h"
#include "Editor/UI/Imgui/FImguiConsoleWindow.h"
#include "Editor/UI/Imgui/FImguiWorldOutliner.h"
#include "Editor/UI/Imgui/FImguiContentsDrawer.h"
#include "Editor/UI/Imgui/FImguiStatsWindow.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Input/FCameraInputController.h"

#include "Editor/Visualizer/FVisualizerRegistry.h"
#include "SWindow.h"

class FEditorApplication final {
	FEditor Editor;

	USceneManager* SceneManager = nullptr;
	UScene* CurrentScene = nullptr;

	FImguiManager ImguiManager;

	FImguiToolbar ToolBar;
	FImguiControlPanelWindow ControlPanelWindow;
	FImguiEditorViewportWindow EditorViewportWindow;
	FImguiPropertyWindow PropertyWindow;
	FImguiConsoleWindow ConsoleWindow;
	FImguiWorldOutliner WorldOutliner;
	FImguiContentsDrawer ContentsDrawer;
	FImguiStatsWindow StatsWindow;
	FVisualizerRegistry VisualizerRegistry;

	FRenderView* RenderView = nullptr;

	SWindow EditorViewports;
public:
	
	static FEditorApplication& Get()
	{
		static FEditorApplication Instance;
		return Instance;
	}

	FEditorApplication(const FEditorApplication&) = delete;
	FEditorApplication& operator=(const FEditorApplication&) = delete;

	FEditorApplication(FEditorApplication&&) = delete;
	FEditorApplication& operator=(FEditorApplication&&) = delete;

	void Initialize_ImguiWin32DX11(HWND& Window, ID3D11Device* Device, ID3D11DeviceContext* Context);
	void Initialize_Runtime(USceneManager* SceneManager, FRenderView* RenderView);
	void Shutdown();
	void Update(float DeltaTime);
	void Render();
	void OnWindowSize(UINT Width, UINT Height);

	void ExecuteCommand(const char* Command);
	
private:
	FEditorApplication() = default;
	~FEditorApplication() = default;
	void BeginFrame();
	void Tick(float DeltaTime);
};
