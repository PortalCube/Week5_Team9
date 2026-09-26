#include "FWindowApplication.h"

FWindowsApplication::FWindowsApplication(HINSTANCE InInstance)
    : Instance{ InInstance }
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

FWindowsApplication::~FWindowsApplication()
{
	Quit();
	CoUninitialize();
}

FWindow* FWindowsApplication::AddWindow(const FWindowDesc& Desc)
{
	Windows.push_back(FWindow::Create(Desc));
	return Windows.back().get();
}

FWindow* FWindowsApplication::CreateMainWindow(const FWindowDesc& Desc)
{
	MainWindow = AddWindow(Desc);
	return MainWindow;
}

void FWindowsApplication::Quit()
{
	// 모든 윈도우 제거
	Windows.clear();
	MainWindow = nullptr;
}

bool FWindowsApplication::CheckExitMessage()
{
	MSG Message;

	while (PeekMessageW(&Message, nullptr, 0u, 0u, PM_REMOVE))
	{
		if (Message.message == WM_QUIT)
		{
			return true;
		}

		TranslateMessage(&Message);
		DispatchMessageW(&Message);
	}

	return false;
}
