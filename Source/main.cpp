#include "Editor/Application/FEditorApplication.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Core/Globals.h"
#include "Runtime/Utility/WindowsUtil.h"
#include "Runtime/Engine/FEngineLoop.h"
#include <Windows.h>
#include <format>

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int32 nShowCmd)
{
	FWString ErrorTitle = std::format(L"{} Fatal Error", WindowsUtil::ToWString(Globals::EngineName));

	FEngineLoop EngineLoop;

	try
	{
		EngineLoop.Init(hInstance);
		EngineLoop.Tick();
		EngineLoop.Exit();
	}
	catch (const std::exception& Error)
	{
		UE_LOG_ERROR("[Fatal] %s", Error.what());
		OutputDebugStringA(Error.what());
		OutputDebugStringA("\n");

		MessageBox(EngineLoop.GetMainWindowHandle(), WindowsUtil::ToWString(Error.what()).c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);

		return -1;
	}
	catch (...)
	{
		constexpr FStringView Message = "알 수 없는 치명적인 오류가 발생했습니다.";

		UE_LOG_ERROR("[Fatal] %s", Message.data());
		OutputDebugStringA(Message.data());
		OutputDebugStringA("\n");

		MessageBox(EngineLoop.GetMainWindowHandle(), WindowsUtil::ToWString(Message).c_str(), ErrorTitle.c_str(), MB_OK | MB_ICONERROR);

		return -1;
	}

	return 0;
}
