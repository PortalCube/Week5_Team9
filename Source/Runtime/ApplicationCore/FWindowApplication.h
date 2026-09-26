#pragma once

#include "Runtime/ApplicationCore/FWindow.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/PointerTypes.h"

#include <Windows.h>

class FWindowsApplication
{
private:

	HINSTANCE Instance;
	TArray<TUniquePtr<FWindow>> Windows;

	FWindow* MainWindow;

public:

	FWindowsApplication(HINSTANCE InInstance);
	~FWindowsApplication();

	// 각종 생성자, 대입 연산자 제거
	FWindowsApplication(const FWindowsApplication&) = delete;
	FWindowsApplication& operator=(const FWindowsApplication&) = delete;

	FWindowsApplication(FWindowsApplication&& Other) = delete;
	FWindowsApplication& operator=(FWindowsApplication&& Other) = delete;

	FWindow* AddWindow(const FWindowDesc& Desc);
	FWindow* CreateMainWindow(const FWindowDesc& Desc);

	FWindow* GetMainWindow() const { return MainWindow; }

	void Quit();

	bool CheckExitMessage();

};
