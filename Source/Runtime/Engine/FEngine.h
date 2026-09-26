#pragma once

#include "Editor/Application/IApplication.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Core/Globals.h"
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

#include "Runtime/Engine/FEngineLoop.h"

#include "Runtime/Parser/FObjParser.h"
#include "Editor/Application/FObjViewerApplication.h"

#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"

class FEngineLoop;

// 엔진의 런타임 계층을 담당하는 클래스
// 엔진 로직, 시스템을 처리하는 부분은 여기서 담당
class FEngine
{
private:

	FRenderer Renderer;
	FRenderView RenderView{ Renderer };
	FEngineLoop& EngineLoop;
	TUniquePtr<IApplication> Application;
	USceneManager SceneManager;

public:
	FEngine(FEngineLoop& InEngineLoop)
	    : EngineLoop{ InEngineLoop }
	{
	}

	void Init();

	void Tick(float DeltaTime);

	void Exit();
};
