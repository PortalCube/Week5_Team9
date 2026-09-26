#pragma once

#include <Windows.h>

#include "Runtime/Core/FString.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/PointerTypes.h"

struct FWindowDesc
{
	HINSTANCE Instance;
	FStringView ClassName;
	FStringView WindowName;

	int32 Width;
	int32 Height;
};

// HWND를 소유하는 윈도우 창을 관리합니다.
class FWindow
{
private:
	HWND Handle = nullptr;

	FWString ClassName = L"";
	FWString WindowName = L"";

	int32 Width = 0;
	int32 Height = 0;



public:

	FWindow();

	static TUniquePtr<FWindow> Create(const FWindowDesc& Desc);

	static LRESULT CALLBACK GlobalMessageCallback(
	    HWND Window,
	    UINT Message,
	    WPARAM WParam,
	    LPARAM LParam
	);

	~FWindow();

	// 각종 생성자, 대입 연산자 제거
	FWindow(const FWindow&) = delete;
	FWindow& operator=(const FWindow&) = delete;

	FWindow(FWindow&& Other) = delete;
	FWindow& operator=(FWindow&& Other) = delete;

	HWND GetHandle() const
	{
		return Handle;
	}

	bool IsDestroyed()
	{
		return Handle == nullptr;
	}

	void Destroy();

	LRESULT CALLBACK MessageCallback(
	    HWND _hWnd,
	    UINT Message,
	    WPARAM wParam,
	    LPARAM lParam
	);

};
