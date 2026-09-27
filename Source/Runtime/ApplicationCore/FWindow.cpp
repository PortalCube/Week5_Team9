#include "FWindow.h"

#include <windowsx.h>

#include "Runtime/Input/FInputManager.h"
#include "Runtime/CoreUObject/FStatsManager.h"

#include "Runtime/Utility/EngineUtil.h"
#include "Runtime/Utility/WindowsUtil.h"
#include "Runtime/Core/Globals.h"
#include "Runtime/Math/FVector2.h"

TUniquePtr<FWindow> FWindow::Create(const FWindowDesc& Desc)
{
	FWString ClassName = WindowsUtil::ToWString(FString{ Desc.ClassName });
	FWString WindowName = WindowsUtil::ToWString(FString{ Desc.WindowName });

	WNDCLASSEX WindowClass
	{
		.cbSize = sizeof(WNDCLASSEX),
		.lpfnWndProc = GlobalMessageCallback,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = Desc.Instance,
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = CreateSolidBrush(RGB(100, 100, 100)),
		.lpszMenuName = nullptr,
		.lpszClassName = ClassName.c_str(),
	};

	if (!RegisterClassEx(&WindowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
	{
		throw EngineUtil::CreateError("윈도우 클래스 등록에 실패했습니다. ClassName: {}, WindowName: ", Desc.ClassName, Desc.WindowName);
	}

	TUniquePtr<FWindow> WindowPtr = MakeUnique<FWindow>();

	WindowPtr->ClassName = ClassName;
	WindowPtr->WindowName = WindowName;
	WindowPtr->Width = Desc.Width;
	WindowPtr->Height = Desc.Height;

	HWND Window = CreateWindowExW(
	    0,
	    WindowClass.lpszClassName,
	    WindowName.c_str(),
	    WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
	    CW_USEDEFAULT, CW_USEDEFAULT, Desc.Width, Desc.Height,
	    nullptr, nullptr, Desc.Instance, WindowPtr.get()
	);

	if (!Window)
	{
		throw EngineUtil::CreateError("윈도우 생성에 실패했습니다. ClassName: {}, WindowName: ", Desc.ClassName, Desc.WindowName);
	}

	WindowPtr->Handle = Window;

	// TODO: 이거 제대로 고칠것
	//ShowWindow(Window, 10);

	return WindowPtr;
}

// ImGui Handler
// TODO: ImGui Handler를 런타임 계층에서 처리하도록 이동
extern LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND,
    UINT,
	WPARAM,
	LPARAM
);

LRESULT FWindow::GlobalMessageCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	FWindow* Self = nullptr;

	// 윈도우를 새로 만들었다면, HWND의 USERDATA로 현재 객체의 포인터 주소를 등록
	if (Message == WM_NCCREATE)
	{
		CREATESTRUCT* Create = reinterpret_cast<CREATESTRUCT*>(LParam);
		SetWindowLongPtr(Window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(Create->lpCreateParams));
	}

	switch (Message)
	{
	case WM_KEYDOWN:   case WM_KEYUP:
	case WM_SYSKEYDOWN: case WM_SYSKEYUP:
	case WM_CHAR:
	case WM_LBUTTONDOWN: case WM_LBUTTONUP:
	case WM_RBUTTONDOWN: case WM_RBUTTONUP:
	case WM_MBUTTONDOWN: case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
		//입력 지연 측정 시작 시간 기록
		FInputLatencyTimer::Get().Trigger();
		break;
	default:
		break;
	}

	// ImGui에서 처리해야 하는 이벤트 체크
	if (ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam))
	{
		return true;
	}

	// HWND의 USERDATA에서 객체 포인터 주소를 받아와서 FWindow의 Handler로 호출
	LONG_PTR WindowPointer = GetWindowLongPtr(Window, GWLP_USERDATA);
	Self = reinterpret_cast<FWindow*>(WindowPointer);

	if (Self)
	{
		return Self->MessageCallback(Window, Message, WParam, LParam);
	}

	return DefWindowProc(Window, Message, WParam, LParam);
}

FWindow::FWindow()
{
}

FWindow::~FWindow()
{
	Destroy();
}

void FWindow::Destroy()
{
	if (Handle != nullptr)
	{
		DestroyWindow(Handle);
		Handle = nullptr;
	}
}

LRESULT FWindow::MessageCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	const FVector2 MousePos{
		static_cast<float>(GET_X_LPARAM(LParam)),
		static_cast<float>(GET_Y_LPARAM(LParam))
	};

	switch (Message)
	{

	case WM_DESTROY:
	{
		Globals::bIsRequestingExit = true;

		PostQuitMessage(0);
		break;
	}

	case WM_NCDESTROY:
	{
		SetWindowLongPtr(Window, GWLP_USERDATA, 0);
		this->Handle = nullptr;
		return DefWindowProc(Window, Message, WParam, LParam);
	}

	case WM_SIZE:
	{
		if (WParam != SIZE_MINIMIZED)
		{
			Globals::bIsRequestingResize = true;
			Globals::ResizeWidth = LOWORD(LParam);
			Globals::ResizeHeight = HIWORD(LParam);
		}

		break;
	}

	case WM_LBUTTONDOWN:
	{
		FInputManager::Get().OnMouseButtonDown(EMouseButton::Left, MousePos);
		SetCapture(Window);
		break;
	}

	case WM_RBUTTONDOWN:
	{
		FInputManager::Get().OnMouseButtonDown(EMouseButton::Right, MousePos);
		SetCapture(Window);
		break;
	}

	case WM_MBUTTONDOWN:
	{
		FInputManager::Get().OnMouseButtonDown(EMouseButton::Middle, MousePos);
		SetCapture(Window);
		break;
	}

	case WM_LBUTTONUP:
	{
		FInputManager::Get().OnMouseButtonUp(EMouseButton::Left, MousePos);
		ReleaseCapture();
		break;
	}

	case WM_RBUTTONUP:
	{
		FInputManager::Get().OnMouseButtonUp(EMouseButton::Right, MousePos);
		ReleaseCapture();
		break;
	}

	case WM_MBUTTONUP:
	{
		FInputManager::Get().OnMouseButtonUp(EMouseButton::Middle, MousePos);
		ReleaseCapture();
		break;
	}

	case WM_MOUSEMOVE:
	{
		FInputManager::Get().OnMouseMove(MousePos);
		break;
	}

	// case WM_CAPTURECHANGED:
	case WM_CANCELMODE:
	case WM_KILLFOCUS:
	{
		const FVector2 Last = FInputManager::Get().GetMousePosition();
		FInputManager::Get().OnMouseButtonUp(EMouseButton::Left, Last);
		FInputManager::Get().OnMouseButtonUp(EMouseButton::Right, Last);
		FInputManager::Get().OnMouseButtonUp(EMouseButton::Middle, Last);
		break;
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
