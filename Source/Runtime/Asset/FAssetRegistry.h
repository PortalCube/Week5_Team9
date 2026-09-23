#pragma once

#include "Runtime/Core/FName.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Core/TSet.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Rendering/FRenderPipeline.h"
#include "Runtime/Asset/UAsset.h"

#include "Runtime/Asset/UStaticMesh.h"
#include "Runtime/Asset/UMaterial.h"
#include "Runtime/Asset/UPipeline.h"
#include "Runtime/Asset/UFont.h"

#include <filesystem>

struct FFolderView
{
	TArray<std::filesystem::path> Folders;
	TArray<UAsset*> Assets;
};

// FObjManager 역할의 클래스
class FAssetRegistry
{
private:

	TMap<FName, UAsset*> AssetMap;

	mutable TMap<std::filesystem::path, FFolderView> DirectoryCache;

public:

	static FAssetRegistry& GetInstance();

	void Register(const FName& Name, UAsset* Pipeline);
	void Clear();
	
	template <typename T>
	T* Get(const FName& Name);

	const TMap<FName, UAsset*>& GetAssetMap() const { return AssetMap; }

	FFolderView GetAssetDirectory(const std::filesystem::path& ParentPath) const;
	
};

template<typename T>
inline T* FAssetRegistry::Get(const FName& Name)
{
	auto It = AssetMap.find(Name);

	if (It == AssetMap.end())
	{
		return nullptr;
	}

	UAsset* Asset = It->second;
	return Asset->Cast<T>();
}
