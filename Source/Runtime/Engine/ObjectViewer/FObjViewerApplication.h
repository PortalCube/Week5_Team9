#pragma once
#include <Windows.h>
#include <memory>

#include "Source/Runtime/Engine/FCamera.h"
#include "Source/Runtime/Rendering/FRenderer.h"
#include "Source/Runtime/Rendering/FRenderResourceLibrary.h"

#include "Runtime/Parser/FObjParser.h"
#include "FViewerCameraController.h"

class FObjViewerApplication
{
public:
	FObjViewerApplication(FRenderer& InRenderer);

	void Initialize(HWND hWnd, ID3D11Device* Device, ID3D11DeviceContext* Context);
	void Update(float DeltaTime);
	void Render();
	void RenderUI();
	void Shutdown();
	void OpenObj(const char* InPath);
	void ImportBinary(const char* InPath);
	void ExportObjToBinary(const char* OutPath);
	
	void OpenMtl(const char* InFilePath);

	void OnWindowSize(UINT Width, UINT Height);

private:
	// UI Functions
	void RenderSideBar();
	void RenderConsole();
	void RenderToolbar();

	TArray<FString> ConsoleLog;
	void AddLog(const FString& Message);
	bool PickFile(FString& OutPath, const wchar_t* InFileFilter, LPCWSTR InlpstrDefExt, bool bSave);

	HWND WindowHandle;
	FRenderer* Renderer;
	TSharedPtr<FMesh> CurrentMesh = nullptr;

	TArray<FVertexData> Vertices;
	TArray<uint32> Indices;
	TArray<FMeshSection> Sections;
	TArray<FMtlData> MtlDatas;
	uint64 CurrentObjHash = 0;

	TMap<FString, FMtlData> MtlMap;
	TMap<FString, TSharedPtr<FTexture>> TextureMap;

	FCamera Camera;
	FViewerCameraController CameraController;

	FLightConstants Light;
	float LightYaw = 45.0f;
	float LightPitch = -45.0f;

	float BackgroundColor[4] = { 0.15f, 0.15f, 0.22f, 1.0f };
};
