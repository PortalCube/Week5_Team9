#pragma once

#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Engine/USceneManager.h"
#include "Editor/Application/IApplication.h"

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
