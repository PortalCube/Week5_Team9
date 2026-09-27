#include "FEngine.h"

#include "Runtime/Core/Log.h"
#include "Runtime/Core/Globals.h"
#include "Runtime/Core/FMemory.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Engine/FEngineLoop.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Engine/USceneManager.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Resource/FResourceLoader.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Utility/EngineUtil.h"

#include "Editor/Application/FEditorApplication.h"
#include "Editor/Application/FObjViewerApplication.h"

#include <Windows.h>

void FEngine::Init()
{
	HWND Window = EngineLoop.GetMainWindowHandle();
	if (!Renderer.Initialize(Window))
	{
		throw EngineUtil::CreateError("FRenderer 초기화에 실패했습니다.");
	}
	FStatsManager::Get().Initialize(Renderer.GetDevice());
	FMemory::Init();

	FRenderResourceLibrary& RenderResources = FRenderResourceLibrary::Get();
	if (!RenderResources.Initialize(Renderer))
	{
		throw EngineUtil::CreateError("FRenderResourceLibrary 초기화에 실패했습니다.");
	}

	UClass::ResolveTypeBitsets();

	FResourceLoader::LoadAssets();

#if defined(_OBJVIEWER)
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;
	Renderer.GetDeviceAndContext_ImplDX11(Device, Context);

	TUniquePtr<FObjViewerApplication> ObjViewer = MakeUnique<FObjViewerApplication>(Renderer);
	ObjViewer->Initialize(Window, Device, Context);
	Application = std::move(ObjViewer);

#else
	// 새씬 생성
	SceneManager.SetScene(NewObject<UScene>());

	TUniquePtr<FEditorApplication> EditorApp = MakeUnique<FEditorApplication>();
	{
		ID3D11Device* Device = nullptr;
		ID3D11DeviceContext* Context = nullptr;
		Renderer.GetDeviceAndContext_ImplDX11(Device, Context);
		EditorApp->Initialize_ImguiWin32DX11(Window, Device, Context);
	}
	EditorApp->Initialize_Runtime(&SceneManager, &RenderView);
	Application = std::move(EditorApp);
#endif

}

void FEngine::Tick(float DeltaTime)
{
	FStatsManager::Get().ResetFrame();
	if (Globals::bIsRequestingResize)
	{
		Renderer.OnWindowSize(Globals::ResizeWidth, Globals::ResizeHeight);
		Application->OnWindowSize(Globals::ResizeWidth, Globals::ResizeHeight);
		Globals::bIsRequestingResize = false;
	}

	FInputManager::Get().BeginFrame();

	{
		SCOPE_CYCLE_COUNTER("Game");
		Application->Update(DeltaTime);
	}

	{
		SCOPE_CYCLE_COUNTER("Draw");
		Renderer.BeginFrame();
		Application->Render();
		Renderer.SwapBuffer();
	}

	SET_CYCLE_COUNTER("Frame", FTimeManager::GetDeltaTime() * 1000.0f);

	// 입력 메시지 수신 ~ 프레임 종료까지의 지연. 프레임의 맨 마지막이어야 한다.
	FInputLatencyTimer::Get().Tick();
}

void FEngine::Exit()
{

	Application->Shutdown();

#if !defined(_OBJVIEWER)
	SceneManager.Release();
#endif
	Application.Reset();

	Renderer.Shutdown();

}
