#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/FName.h"

class FArchive;

class FResourceLoader
{

private:

	static constexpr int32 CurrentSchemaVersion = 1;

	static void LoadPipelineAsset(const FArchive& Archive, const FName& ID);
	static void LoadMaterialAsset(const FArchive& Archive, const FName& ID);
	static void LoadStaticMeshAsset(const FArchive& Archive, const FName& ID);
	static void LoadFontAsset(const FArchive& Archive, const FName& ID);
	static void LoadTextureAsset(const FArchive& Archive, const FName& ID);

	static void LoadMtlMaterial(const std::filesystem::path& MtlFilePath, const std::filesystem::path& RootPath);

	/// <summary>
	/// MeshUtil을 사용하여 엔진에서 기본으로 사용하는 메쉬를 생성하고 UStaticMesh 애셋으로 등록합니다.
	/// </summary>
	static void LoadDefaultStaticMeshAssets();
	static void LoadCodeGeneratedRenderAssets();

public:


	/// <summary>
	/// AssetPath의 모든 애셋을 로드합니다. 
	/// </summary>
	static void LoadAssets();

	// Import Obj by UI
	static bool ImportObj(const std::filesystem::path& ObjFilePath);

};
