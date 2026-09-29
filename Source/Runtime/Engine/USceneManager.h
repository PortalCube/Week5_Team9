#pragma once
#include "UScene.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
class FCamera;
class USceneManager final
{

public:
	void SaveScene(const FString& path) const;
	void LoadScene(const FString& path, FCamera* OutCamera = nullptr);
	void SetScene(UScene* scene);
	void Release();

	UScene* CurrentScene = nullptr;
};
