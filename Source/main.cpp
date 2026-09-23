#include "Editor/Application/FEditorApplication.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Utility/EngineUtil.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Engine/USceneManager.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Resource/FResourceLoader.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Asset/UStaticMesh.h"
#include "Runtime/Utility/WindowsUtil.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include "Runtime/Core/FMemory.h"
#include <Windows.h>
#include <windowsx.h>

#include "Runtime/Parser/FObjParser.h"
#include "Runtime/Engine/ObjectViewer/FObjViewerApplication.h"

#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool bRequestNewScene = false;
static bool bRequestSaveScene = false;
static bool bRequestLoadScene = false;
static bool bRequestResize = false;
static UINT ResizeWidth = 0u;
static UINT ResizeHeight = 0u;

namespace
{
	constexpr LPCWSTR WindowName = L"OIIAII";

	HWND CreateWindowHandle(HINSTANCE Instance);
	bool ProcessWindowMessage();

	LRESULT CALLBACK WindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);
}

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd) 
{

	try
	{
	HWND Window = CreateWindowHandle(hInstance);
	if (!Window)
	{
		return -1;
	}

	ShowWindow(Window, nShowCmd);

	FInputManager::Get();

	FRenderer Renderer;
	if (!Renderer.Initialize(Window))
	{
		throw EngineUtil::CreateError("FRenderer 초기화에 실패했습니다.");
	}
	FRenderView RenderView{ Renderer };

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
	FObjViewerApplication ObjViewer(Renderer);

	ID3D11Device* Device = nullptr; ID3D11DeviceContext* Context = nullptr;
	Renderer.GetDeviceAndContext_ImplDX11(Device, Context);
	ObjViewer.Initialize(Window, Device, Context);

#else
	//새씬 생성
	USceneManager SceneManager;
	SceneManager.SetScene(NewObject<UScene>());

	FEditorApplication& EditorApp = FEditorApplication::Get();
	{
		ID3D11Device* Device = nullptr; ID3D11DeviceContext* Context = nullptr;
		Renderer.GetDeviceAndContext_ImplDX11(Device, Context);
		EditorApp.Initialize_ImguiWin32DX11(Window, Device, Context);
	}
	EditorApp.Initialize_Runtime(&SceneManager, &RenderView);
#endif	

	bool bQuit = false;
	while (!bQuit)
	{
		FTimeManager::Get().Update();

		if (!ProcessWindowMessage())
		{
			bQuit = true;
			break;
		}

		if (bRequestResize)
		{
			Renderer.OnWindowSize(ResizeWidth, ResizeHeight);
#if defined(_OBJVIEWER)
			ObjViewer.OnWindowSize(ResizeWidth, ResizeHeight);
#else
			EditorApp.OnWindowSize(ResizeWidth, ResizeHeight);
#endif
			bRequestResize = false;
		}

		FInputManager::Get().BeginFrame();
#if defined(_OBJVIEWER)
		ObjViewer.Update(FTimeManager::Get().GetDeltaTime());
#else
		EditorApp.Update(FTimeManager::Get().GetDeltaTime());
#endif
		Renderer.BeginFrame();
#if defined(_OBJVIEWER)
		ObjViewer.Render();
		ObjViewer.RenderUI();
#else
		EditorApp.Render();		
#endif
		Renderer.SwapBuffer();

		//EditorApp.CollectGarbage();
	}

#if defined(_OBJVIEWER)

#else
	EditorApp.Shutdown();
	SceneManager.Release();
#endif
	//EditorApp.CollectGarbage();
	Renderer.Shutdown();

	return 0;
	}
	catch (const std::exception& Error)
	{
		UE_LOG_ERROR("[Fatal] %s", Error.what());
		OutputDebugStringA(Error.what());
		OutputDebugStringA("\n");
		MessageBox(nullptr, WindowsUtil::ToWString(Error.what()).c_str(), L"MyEngine Fatal Error",
		            MB_OK | MB_ICONERROR);
		return -1;
	}
	catch (...)
	{
		constexpr const char* Message = "알 수 없는 치명적인 오류가 발생했습니다.";
		UE_LOG_ERROR("[Fatal] %s", Message);
		OutputDebugStringA(Message);
		OutputDebugStringA("\n");
		MessageBox(nullptr, WindowsUtil::ToWString(Message).c_str(), L"MyEngine Fatal Error",
		            MB_OK | MB_ICONERROR);
		return -1;
	}
}

namespace
{
	// TODO: Resizing 처리
	HWND CreateWindowHandle(HINSTANCE Instance)
	{
		WNDCLASS WindowClass{};
		WindowClass.lpfnWndProc = WindowCallback;
			
		WindowClass.hInstance = Instance;
		WindowClass.lpszClassName = L"MyEngine";

		if (!RegisterClass(&WindowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
		{
			return nullptr;
		}

		HWND Window = CreateWindowExW(
			0,
			WindowClass.lpszClassName,
			WindowName,
			WS_POPUP | WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800,
			nullptr, nullptr, Instance, nullptr);

		return Window;
	}

	// 닫아야되면 false 반환
	bool ProcessWindowMessage()
	{
		MSG Message;
		while (PeekMessageW(&Message, nullptr, 0u, 0u, PM_REMOVE))
		{
			if (Message.message == WM_QUIT)
			{
				return false;
			}

			TranslateMessage(&Message);
			DispatchMessageW(&Message);
		}

		return true;
	}

	LRESULT CALLBACK WindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
	{
		if (LRESULT ImGuiResult = ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam)) // imgui의 프레임 스냅샷 상태를 갱신
			return ImGuiResult; // ImGuiResult != 0인 경우: 상태가 DefWindowProcW() 함수 동작을 오버라이드해야 하는 경우

		const FVector2 MousePos{
			static_cast<float>(GET_X_LPARAM(LParam)),
			static_cast<float>(GET_Y_LPARAM(LParam))
		};

		switch (Message)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_SIZE:
		{
			if (WParam != SIZE_MINIMIZED)
			{
				bRequestResize = true;
				ResizeWidth = LOWORD(LParam);
				ResizeHeight = HIWORD(LParam);
			}

			break;
		}

		case WM_LBUTTONDOWN:
			FInputManager::Get().OnMouseButtonDown(EMouseButton::Left, MousePos);
			SetCapture(Window);
			break;

		case WM_RBUTTONDOWN:
			FInputManager::Get().OnMouseButtonDown(EMouseButton::Right, MousePos);
			SetCapture(Window);
			break;

		case WM_MBUTTONDOWN:
			FInputManager::Get().OnMouseButtonDown(EMouseButton::Middle, MousePos);
			SetCapture(Window);
			break;

		case WM_LBUTTONUP:
			FInputManager::Get().OnMouseButtonUp(EMouseButton::Left, MousePos);
			ReleaseCapture();
			break;

		case WM_RBUTTONUP:
			FInputManager::Get().OnMouseButtonUp(EMouseButton::Right, MousePos);
			ReleaseCapture();
			break;

		case WM_MBUTTONUP:
			FInputManager::Get().OnMouseButtonUp(EMouseButton::Middle, MousePos);
			ReleaseCapture();
			break;

		case WM_MOUSEMOVE:
			FInputManager::Get().OnMouseMove(MousePos);
			break;

		//case WM_CAPTURECHANGED:
		case WM_CANCELMODE:
		case WM_KILLFOCUS:
		{
			const FVector2 Last = FInputManager::Get().GetMousePosition();
			FInputManager::Get().OnMouseButtonUp(EMouseButton::Left, Last);
			FInputManager::Get().OnMouseButtonUp(EMouseButton::Right, Last);
			FInputManager::Get().OnMouseButtonUp(EMouseButton::Middle, Last);
			break;
		}
		// WM_CA
			break;
		case WM_KEYDOWN:
			switch (WParam)
			{
			case VK_F5: bRequestSaveScene = true; break;
			case VK_F6: bRequestLoadScene = true; break;
			case VK_F7: bRequestNewScene = true; break;
			}
			break;

		case WM_MOUSEWHEEL:
		{
			const float WheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(WParam)) / static_cast<float>(WHEEL_DELTA);
			FInputManager::Get().OnMouseWheel(WheelDelta);
			break;
		}		

		default:
			return DefWindowProc(Window, Message, WParam, LParam);
		}

		return 0;
	}
}
