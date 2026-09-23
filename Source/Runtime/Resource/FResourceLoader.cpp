#include "FResourceLoader.h"
#include "Runtime/Utility/EngineUtil.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Asset/UPipeline.h"
#include "Runtime/Asset/UMaterial.h"
#include "Runtime/Asset/UFont.h"
#include "Runtime/Asset/UStaticMesh.h"
#include "Runtime/Asset/UTexture.h"
#include "Runtime/Parser/FObjParser.h"
#include "Runtime/Mesh/MeshUtil.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "ThirdParty/Json/json.hpp"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/FStatsManager.h"

#include "Runtime/Material/FRasterizerDesc.h"
#include "Runtime/Material/FDepthStencilDesc.h"
#include "Runtime/Material/FBlendDesc.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <utility>

#pragma region StringEnumMap

TMap<FString, ERasterizerFillMode> RasterizerFillModeMap
{
	{ "Solid", ERasterizerFillMode::Solid },
	{ "Wireframe", ERasterizerFillMode::Wireframe },
};

TMap<FString, ERasterizerCullMode> RasterizerCullModeMap
{
	{ "None", ERasterizerCullMode::None },
	{ "Front", ERasterizerCullMode::Front },
	{ "Back", ERasterizerCullMode::Back },
};

TMap<FString, ERasterizerFrontFaceMode> RasterizerFrontFaceModeMap
{
	{ "CounterClockwise", ERasterizerFrontFaceMode::CounterClockwise },
	{ "Clockwise", ERasterizerFrontFaceMode::Clockwise },
};

TMap<FString, EDepthWriteMode> DepthWriteModeMap
{
	{ "Disable", EDepthWriteMode::Disable },
	{ "Enable", EDepthWriteMode::Enable },
};

TMap<FString, EBlendMode> BlendModeMap
{
	{ "Opaque", EBlendMode::Opaque },
	{ "Masked", EBlendMode::Masked },
	{ "Translucent", EBlendMode::Translucent },
	{ "Additive", EBlendMode::Additive },
	{ "PremultipliedAlpha", EBlendMode::PremultipliedAlpha },
};

TMap<FString, ETextureSamplerFilterMode> TextureSamplerFilterModeMap
{
	{ "Point", ETextureSamplerFilterMode::Point },
	{ "Bilinear", ETextureSamplerFilterMode::Bilinear },
	{ "Trilinear", ETextureSamplerFilterMode::Trilinear },
	{ "Anisotropic", ETextureSamplerFilterMode::Anisotropic },
};

TMap<FString, ETextureSamplerWrapMode> TextureSamplerWrapModeMap
{
	{ "Wrap", ETextureSamplerWrapMode::Wrap },
	{ "Mirror", ETextureSamplerWrapMode::Mirror },
	{ "Clamp", ETextureSamplerWrapMode::Clamp },
};

#pragma endregion

void FResourceLoader::LoadDefaultStaticMeshAssets()
{
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	FRenderResourceLibrary& ResourceLibrary = FRenderResourceLibrary::Get();
	FRenderer* Renderer = ResourceLibrary.GetRenderer();
	if (Renderer == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadDefaultStaticMeshAssets] 렌더러가 초기화되지 않았습니다.");
	}

	auto RegisterStaticMeshAsset = [&Registry, &ResourceLibrary](const FName& ID, bool bCreated)
	{
		if (!bCreated)
		{
			throw EngineUtil::CreateError(
				"[FResourceLoader::LoadDefaultStaticMeshAssets] 기본 메쉬 생성에 실패했습니다. {}",
				ID.ToString());
		}

		TSharedPtr<FMesh> Mesh = ResourceLibrary.GetMesh(ID);
		if (Mesh == nullptr)
		{
			throw EngineUtil::CreateError(
				"[FResourceLoader::LoadDefaultStaticMeshAssets] 등록된 FMesh를 찾지 못했습니다. {}",
				ID.ToString());
		}

		FStatsManager::Get().AddMemory(EStatMemoryCategory::StaticMesh, Mesh->GetBufferSize());

		UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
		UStaticMeshDesc StaticMeshDesc{};
		StaticMeshDesc.ID = ID;
		StaticMeshDesc.Name = ID;
		StaticMeshDesc.Mesh = Mesh.get();

		StaticMesh->Load(StaticMeshDesc);
		Registry.Register(ID, StaticMesh);
	};

	RegisterStaticMeshAsset("#Cube", MeshUtil::CreateCubeMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("#Cylinder", MeshUtil::CreateCylinderMesh(*Renderer, ResourceLibrary, 1.0f, 24u, 1.0f, 1.0f));
	RegisterStaticMeshAsset("#Cone", MeshUtil::CreateConeMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("#SpotlightCone", MeshUtil::CreateSpotlightConeMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("#Arrow", MeshUtil::CreateArrowMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("#Circle", MeshUtil::CreateCircleMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("#RotGizmo", MeshUtil::CreateRotationGizmoMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("#SquareArrow", MeshUtil::CreateSquareArrowMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("#Grid", MeshUtil::CreateGridMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("#Sphere", MeshUtil::CreateSphereMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("#Line", MeshUtil::CreateLineMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("#Plane", MeshUtil::CreatePlaneMesh(*Renderer, ResourceLibrary));
	RegisterStaticMeshAsset("#Rect", MeshUtil::CreateRectMesh(*Renderer, ResourceLibrary));
}

void FResourceLoader::LoadCodeGeneratedRenderAssets()
{
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	FRenderResourceLibrary& Library = FRenderResourceLibrary::Get();
	TSharedPtr<FRenderPipeline> Pipeline = Library.GetPipeline("#Outline");
	if (!Pipeline)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadCodeGeneratedRenderAssets] Outline Pipeline 생성에 실패했습니다.");
	}

	UPipeline* PipelineAsset = NewObject<UPipeline>();
	UPipelineDesc PipelineDesc{};
	PipelineDesc.ID = "#Pipeline/Outline";
	PipelineDesc.Name = "#Outline";
	PipelineDesc.Pipeline = Pipeline.get();
	PipelineAsset->Load(PipelineDesc);
	Registry.Register(PipelineDesc.ID, PipelineAsset);

	UMaterial* MaterialAsset = NewObject<UMaterial>();
	UMaterialDesc MaterialDesc{};
	MaterialDesc.ID = "#Material/Outline";
	MaterialDesc.Name = "#Outline";
	MaterialDesc.Pipeline = PipelineAsset;
	MaterialAsset->Load(MaterialDesc);
	Registry.Register(MaterialDesc.ID, MaterialAsset);

	TSharedPtr<FMaterial> Material = MakeShared<FMaterial>();
	Material->SetPipeLine(Pipeline.get());
	Library.RegisterMaterial("#Outline", Material);
}

void FResourceLoader::LoadAssets()
{
	namespace fs = std::filesystem;
	using json = nlohmann::json;

	// 엔진 애셋을 먼저 로드
	LoadCodeGeneratedRenderAssets();
	LoadDefaultStaticMeshAssets();

	const fs::path  AssetPath = EngineUtil::GetContentDirectory();

	bool bIsExist = fs::exists(AssetPath);
	bool bIsDirectory = fs::is_directory(AssetPath);

	if (!bIsExist || !bIsDirectory)
	{
		return;
	}

	const auto& Iterator = fs::recursive_directory_iterator(AssetPath);

	TArray<std::pair<FName, FArchive>> PipelineAssets;
	TArray<std::pair<FName, FArchive>> TextureAssets;
	TArray<std::pair<FName, FArchive>> MaterialAssets;
	TArray<std::pair<FName, FArchive>> FontAssets;
	TArray<std::pair<FName, FArchive>> StaticMeshAssets;

	for (const auto& Entry : Iterator)
	{
		if (!Entry.is_regular_file()) { continue; }
		if (Entry.path().extension() != ".json") { continue; }

		std::ifstream File{ Entry.path() };
		if (!File.is_open())
		{
			UE_LOG("[FResourceLoader::LoadAssets] 파일을 여는데 실패했습니다. %s", Entry.path().c_str());
			continue;
		}

		json data;

		try
		{
			data = json::parse(File);
		}
		catch (const json::parse_error&)
		{
			UE_LOG("[FResourceLoader::LoadAssets] JSON 파일을 파싱하는데 실패했습니다. %s", Entry.path().c_str());
			continue;
		}

		FArchive Archive{ data };
		const FString AssetID = Entry.path().lexically_relative(AssetPath).generic_string();
		Archive.SetString("AssetID", AssetID);

		int Version = Archive.GetInt32("Version");

		if (Version != CurrentSchemaVersion)
		{
			UE_LOG("[FResourceLoader::LoadAssets] 파일의 버전이 불일치합니다. %s", Entry.path().c_str());
			continue;
		}

		FString Type = Archive.GetString("AssetType");
		
		if (Type == "Pipeline") { PipelineAssets.emplace_back(Type, Archive); }
		else if (Type == "Texture") { TextureAssets.emplace_back(Type, Archive); }
		else if (Type == "Material") { MaterialAssets.emplace_back(Type, Archive); }
		else if (Type == "Font") { FontAssets.emplace_back(Type, Archive); }
		else if (Type == "StaticMesh") { StaticMeshAssets.emplace_back(Type, Archive); }
		else { UE_LOG("[FResourceLoader::LoadAssets] 알 수 없는 AssetType %s", AssetID.c_str()); }
	}

	const TArray<TArray<std::pair<FName, FArchive>>*> LoadOrder = {
		&PipelineAssets, &TextureAssets, &MaterialAssets, &FontAssets, &StaticMeshAssets
	};
	for (const auto* Assets : LoadOrder)
	{
		for (const auto& Item : *Assets)
		{
			const FName& Type = Item.first;
			const FArchive& Archive = Item.second;
			const FName AssetID = Archive.GetString("AssetID");
			if (Type == "Pipeline") { LoadPipelineAsset(Archive, AssetID); }
			else if (Type == "Texture") { LoadTextureAsset(Archive, AssetID); }
			else if (Type == "Material") { LoadMaterialAsset(Archive, AssetID); }
			else if (Type == "Font") { LoadFontAsset(Archive, AssetID); }
			else if (Type == "StaticMesh") { LoadStaticMeshAsset(Archive, AssetID); }
		}
	}
}

bool FResourceLoader::ImportObj(const std::filesystem::path& ObjFilePath)
{
	namespace fs = std::filesystem;

	if (!fs::exists(ObjFilePath))
	{
		return false;
	}
	
	fs::path ContentDir = EngineUtil::GetContentDirectory();
	std::string ModelName = ObjFilePath.stem().string();

	fs::path TargetObjPath;
	fs::path RelativeMeshPath;
	
	auto Rel = fs::relative(ObjFilePath, ContentDir);
	if (Rel.empty() || Rel.string().rfind("..", 0) == 0)
	{
		fs::path DestDir = ContentDir / "StaticMesh" / ModelName;
		fs::create_directories(DestDir);
		TargetObjPath = DestDir / ObjFilePath.filename();
		fs::copy_file(ObjFilePath, TargetObjPath, fs::copy_options::overwrite_existing);
		
		fs::path SourceParent = ObjFilePath.parent_path();
		for (const auto& Entry : fs::directory_iterator(SourceParent))
		{
			if (Entry.is_regular_file())
			{
				std::string Ext = Entry.path().extension().string();
				if (Ext == ".mtl" || Ext == ".png" || Ext == ".jpg" || Ext == ".dds")
				{
					fs::copy_file(Entry.path(), DestDir / Entry.path().filename(), fs::copy_options::overwrite_existing);
				}
			}
		}
		RelativeMeshPath = fs::relative(TargetObjPath, ContentDir);
	}
	else
	{
		TargetObjPath = ObjFilePath;
		RelativeMeshPath = Rel;
	}

	fs::path AssetPath = fs::path(RelativeMeshPath).replace_extension(".json");
	FName AssetID = FName(AssetPath.generic_string());	

	FArchive Archive;
	Archive.SetString("Name", ModelName);
	Archive.SetString("MeshFilePath", RelativeMeshPath.generic_string());

	LoadStaticMeshAsset(Archive, AssetID);

	return true;
}

void FResourceLoader::LoadPipelineAsset(const FArchive& Archive, const FName& ID)
{
	namespace fs = std::filesystem;

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	// 포인터만 생성..
	UPipeline* PipelineAsset = NewObject<UPipeline>();
	UPipelineDesc PipelineDesc{};
	FRenderPipelineDesc RenderPipelineDesc{};

	PipelineDesc.ID = ID;
	PipelineDesc.Name = Archive.GetString("Name");
	PipelineDesc.bIsInstancing = Archive.GetBool("Instancing");

	// FRenderPipelineDesc 생성
	const fs::path VertexShaderFilePath = fs::path(EngineUtil::GetContentDirectory()) / Archive.GetString("VertexShaderFilePath");
	const fs::path PixelShaderFilePath = fs::path(EngineUtil::GetContentDirectory()) / Archive.GetString("PixelShaderFilePath");

	RenderPipelineDesc.VertexShaderFilePath = VertexShaderFilePath.string();
	RenderPipelineDesc.PixelShaderFilePath = PixelShaderFilePath.string();
	RenderPipelineDesc.bIsInstancing = PipelineDesc.bIsInstancing;

	if (Archive.IsNull("Rasterizer"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadPipelineAsset] 'Rasterizer' 필드가 없습니다. {}", ID.ToString());
	}

	FArchive RasterizerArchive = Archive.GetArchive("Rasterizer");
	RenderPipelineDesc.Rasterizer.FillMode = RasterizerArchive.GetEnum("FillMode", RasterizerFillModeMap);
	RenderPipelineDesc.Rasterizer.CullMode = RasterizerArchive.GetEnum("CullMode", RasterizerCullModeMap);
	RenderPipelineDesc.Rasterizer.FrontFace = RasterizerArchive.GetEnum("FrontFaceMode", RasterizerFrontFaceModeMap);
	RenderPipelineDesc.Rasterizer.bUseMultisample = RasterizerArchive.GetBool("Multisample");
	RenderPipelineDesc.Rasterizer.bUseAntialiasedLine = RasterizerArchive.GetBool("AntialiasedLine");

	if (Archive.IsNull("DepthStencil"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadPipelineAsset] 'DepthStencil' 필드가 없습니다. {}", ID.ToString());
	}

	FArchive DepthStencilArchive = Archive.GetArchive("DepthStencil");
	RenderPipelineDesc.DepthStencil.bDepthEnable = DepthStencilArchive.GetBool("DepthEnable");
	RenderPipelineDesc.DepthStencil.bStencilEnable = DepthStencilArchive.GetBool("StencilEnable");
	RenderPipelineDesc.DepthStencil.DepthWrite = DepthStencilArchive.GetEnum("DepthWriteMode", DepthWriteModeMap);

	if (Archive.IsNull("Blend"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadPipelineAsset] 'Blend' 필드가 없습니다. {}", ID.ToString());
	}

	FArchive BlendArchive = Archive.GetArchive("Blend");
	RenderPipelineDesc.Blend.BlendMode = BlendArchive.GetEnum("BlendMode", BlendModeMap);

	FRenderResourceLibrary& ResourceLibrary = FRenderResourceLibrary::Get();
	FRenderer* Renderer = ResourceLibrary.GetRenderer();
	if (Renderer == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadPipelineAsset] 렌더러가 초기화되지 않았습니다. ID: {}",
			ID.ToString());
	}

	TSharedPtr<FRenderPipeline> Pipeline = Renderer->CreateRenderPipeline(RenderPipelineDesc);
	if (Pipeline == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadPipelineAsset] FRenderPipeline 생성에 실패했습니다. ID: {}",
			ID.ToString());
	}

	ResourceLibrary.RegisterPipeline(PipelineDesc.Name, Pipeline);
	PipelineDesc.Pipeline = Pipeline.get();

	PipelineAsset->Load(PipelineDesc);
	Registry.Register(ID, PipelineAsset);
}

void FResourceLoader::LoadMaterialAsset(const FArchive& Archive, const FName& ID)
{
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	// 포인터만 생성..
	UMaterial* Material = NewObject<UMaterial>();
	UMaterialDesc MaterialDesc{};

	MaterialDesc.ID = ID;
	MaterialDesc.Name = Archive.GetString("Name");

	if (Archive.IsNull("TextureSampler"))
	{
		throw EngineUtil::CreateError("[FResourceLoader::LoadMaterialAsset] 'TextureSampler' 필드가 없습니다. {}", ID.ToString());
	}

	FArchive TextureSamplerArchive = Archive.GetArchive("TextureSampler");
	MaterialDesc.TextureSamplerDesc.FilterMode = TextureSamplerArchive.GetEnum("FilterMode", TextureSamplerFilterModeMap);
	MaterialDesc.TextureSamplerDesc.WrapMode = TextureSamplerArchive.GetEnum("WrapMode", TextureSamplerWrapModeMap);


	const FName UPipelineID = Archive.GetString("UPipelineID");
	MaterialDesc.Pipeline = Registry.Get<UPipeline>(UPipelineID);
	if (MaterialDesc.Pipeline == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadMaterialAsset] Pipeline을 찾을 수 없습니다. ID: {}, Pipeline: {}",
			ID.ToString(), UPipelineID.ToString());
	}


	if (!Archive.IsNull("UTextureID"))
	{
		const FName UTextureID = Archive.GetString("UTextureID");
		MaterialDesc.Texture = Registry.Get<UTexture>(UTextureID);
		if (MaterialDesc.Texture == nullptr)
		{
			throw EngineUtil::CreateError(
				"[FResourceLoader::LoadMaterialAsset] Texture을 찾을 수 없습니다. ID: {}, Texture: {}",
				ID.ToString(), UTextureID.ToString());
		}
	}

	Material->Load(MaterialDesc);
	Registry.Register(ID, Material);

	TSharedPtr<FMaterial> RenderMaterial = MakeShared<FMaterial>();
	RenderMaterial->SetPipeLine(MaterialDesc.Pipeline->Get());
	if (MaterialDesc.Texture) { RenderMaterial->SetTexture(MaterialDesc.Texture->Get()); }
	RenderMaterial->SetSamplerDesc(MaterialDesc.TextureSamplerDesc);
	FRenderResourceLibrary::Get().RegisterMaterial(MaterialDesc.Name, RenderMaterial);
}

void FResourceLoader::LoadStaticMeshAsset(const FArchive& Archive, const FName& ID)
{
	namespace fs = std::filesystem;

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
	UStaticMeshDesc StaticMeshDesc{};
	StaticMeshDesc.ID = ID;
	StaticMeshDesc.Name = Archive.GetString("Name");
	fs::path MeshFilePath = fs::path(Archive.GetString("MeshFilePath")).lexically_normal();
	fs::path MeshRootPath = MeshFilePath.parent_path();
	fs::path MeshFileFullPath = fs::path(EngineUtil::GetContentDirectory()) / MeshFilePath;
	fs::path MeshBinPath = fs::path(MeshFileFullPath).replace_extension("bin");

	TArray<FVertexData> Vertices;
	TArray<uint32> Indices;
	TArray<FMeshSection> Sections;

	bool bValid = false;
	if (fs::exists(MeshBinPath))
	{
		bValid = FObjParser::ValidateBinary(MeshBinPath.string().c_str(), MeshFileFullPath.string().c_str());
	}
	
	if (bValid)
	{
		if (!FObjParser::LoadMeshFromBinary(MeshBinPath.string().c_str(), Vertices, Indices, Sections))
		{
			throw EngineUtil::CreateError(
				"[FResourceLoader::LoadStaticMeshAsset] BIN 데이터를 정점 데이터로 변환하는데 실패했습니다. ID: {}, Path: {}",
				ID.ToString(),
				MeshBinPath.string());
		}
	}
	else
	{
		FRawObjData RawObjData{};
		if (!FObjParser::LoadObj(MeshFileFullPath.string().c_str(), RawObjData))
		{
			throw EngineUtil::CreateError(
				"[FResourceLoader::LoadStaticMeshAsset] OBJ 파일을 불러오는데 실패했습니다. ID: {}, Path: {}",
				ID.ToString(),
				MeshFilePath.string());
		}

		if (!FObjParser::ConvertObjToVertex(RawObjData, Vertices, Indices, Sections))
		{
			throw EngineUtil::CreateError(
				"[FResourceLoader::LoadStaticMeshAsset] OBJ 데이터를 정점 데이터로 변환하는데 실패했습니다. ID: {}, Path: {}",
				ID.ToString(),
				MeshFilePath.string());
		}

		// bake 
		uint64 ObjHash = FObjParser::ComputeFileHash(MeshFileFullPath);
		FObjParser::SaveMeshToBinary(MeshBinPath.string().c_str(), ObjHash, Vertices, Indices, Sections);
	}

	// BIN에는 OBJ의 원본 material 이름을 저장하고, 런타임에서만 애셋 Root를 붙인다.
	for (FMeshSection& Section : Sections)
	{
		Section.SectionName = (MeshRootPath / fs::path(Section.SectionName)).generic_string();
	}

	FRenderResourceLibrary& ResourceLibrary = FRenderResourceLibrary::Get();
	FRenderer* Renderer = ResourceLibrary.GetRenderer();
	if (Renderer == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadStaticMeshAsset] 렌더러가 초기화되지 않았습니다. ID: {}",
			ID.ToString());
	}

	FMeshDesc MeshDesc
	{
		.VertexData = Vertices.data(),
		.VertexDataSize = static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
		.VertexStride = static_cast<uint32>(sizeof(FVertexData)),
		.VertexCount = static_cast<uint32>(Vertices.size()),
		.IndexData = Indices.data(),
		.IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
		.IndexCount = static_cast<uint32>(Indices.size()),
		.Sections = Sections
	};

	TSharedPtr<FMesh> Mesh = Renderer->CreateMesh(MeshDesc);
	if (Mesh == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadStaticMeshAsset] FMesh 생성에 실패했습니다. ID: {}, Path: {}",
			ID.ToString(),
			MeshFilePath.string());
	}

	ResourceLibrary.RegisterMesh(ID, Mesh);
	StaticMeshDesc.Mesh = Mesh.get();

	StaticMesh->Load(StaticMeshDesc);
	Registry.Register(ID, StaticMesh);

	const size_t VertexBufferSize = sizeof(FVertexData) * Vertices.size();
	const size_t IndexBufferSize = sizeof(uint32) * Indices.size();
	const size_t GPUResourceSize = VertexBufferSize + IndexBufferSize;

	FStatsManager::Get().AddMemory(EStatMemoryCategory::StaticMesh, GPUResourceSize);

	// Load mtl

	fs::path MeshMaterialPath = fs::path(MeshFileFullPath).replace_extension("mtl");
	if (fs::exists(MeshMaterialPath))
	{
		LoadMtlMaterial(MeshMaterialPath, MeshRootPath);
	}	
}

void FResourceLoader::LoadFontAsset(const FArchive& Archive, const FName& ID)
{
	namespace fs = std::filesystem;

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	UFont* FontAsset = NewObject<UFont>();

	const FName UTextureID = Archive.GetString("UTextureID");

	const FArchive GlyphDataArchive = Archive.GetArchive("GlyphData");
	TSharedPtr<FFont> Font = MakeShared<FFont>(GlyphDataArchive);

	UFontDesc FontDesc{};
	FontDesc.ID = ID;
	FontDesc.Name = Archive.GetString("Name");
	FontDesc.Font = Font.get();
	FontDesc.Texture = Registry.Get<UTexture>(UTextureID);
	if (FontDesc.Texture == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadFontAsset] Texture을 찾을 수 없습니다. ID: {}, Texture: {}",
			ID.ToString(), UTextureID.ToString());
	}

	// TODO: Setter 지정
	FRenderResourceLibrary::Get().AllFontMap[fs::path(ID.ToString()).stem().string()] = Font;

	FontAsset->Load(FontDesc);
	Registry.Register(ID, FontAsset);
}

void FResourceLoader::LoadTextureAsset(const FArchive& Archive, const FName& ID)
{
	namespace fs = std::filesystem;

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	UTexture* TextureAsset = NewObject<UTexture>();
	UTextureDesc TextureDesc{};

	TextureDesc.ID = ID;
	TextureDesc.Name = Archive.GetString("Name");

	fs::path RawTexturePath = fs::path(EngineUtil::GetContentDirectory()) / Archive.GetString("RawTextureFilePath");
	//if (RawTexturePath.extension() != ".dds")
	//{
	//	RawTexturePath.replace_extension(".dds");
	//}

	FRenderResourceLibrary& ResourceLibrary = FRenderResourceLibrary::Get();
	FRenderer* Renderer = ResourceLibrary.GetRenderer();
	if (Renderer == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadTextureAsset] 렌더러가 초기화되지 않았습니다. ID: {}",
			ID.ToString());
	}

	TSharedPtr<FTexture> Texture = Renderer->CreateTexture(RawTexturePath.wstring().c_str());
	if (Texture == nullptr)
	{
		throw EngineUtil::CreateError(
			"[FResourceLoader::LoadTextureAsset] FTexture 생성에 실패했습니다. ID: {}, Path: {}",
			ID.ToString(),
			RawTexturePath.string());
	}

	ResourceLibrary.RegisterTexture(fs::path(ID.ToString()).stem().string(), Texture);
	TextureDesc.Texture = Texture.get();

	TextureAsset->Load(TextureDesc);
	Registry.Register(ID, TextureAsset);
}

void FResourceLoader::LoadMtlMaterial(const std::filesystem::path& MtlFilePath, const std::filesystem::path& RootPath)
{
	namespace fs = std::filesystem;

	TArray<FMtlData> MtlData;
	if (!FObjParser::LoadMtl(MtlFilePath.string().c_str(), MtlData))
	{
		return;
	}

	std::filesystem::path ParentPath = std::filesystem::path(MtlFilePath).parent_path();

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	for (const auto& Mtl : MtlData)
	{		
		FName TextureId = "None";
		if (!Mtl.map_Kd.empty())
		{
			FArchive TextureArchive;
			const fs::path TextureAssetPath = RootPath / fs::path(Mtl.map_Kd);
			TextureArchive.SetString("Name", TextureAssetPath.generic_string());

			std::filesystem::path TexturePath = ParentPath / Mtl.map_Kd;
			TextureArchive.SetString("RawTextureFilePath", TexturePath.generic_string());

			TextureId = FName(TextureAssetPath.generic_string());

			if (!Registry.Get<UTexture>(TextureId))
			{
				LoadTextureAsset(TextureArchive, TextureId);
			}
		}
		else
		{
			const fs::path TextureAssetPath = RootPath / fs::path(Mtl.MaterialName + "_Solid");
			TextureId = FName(TextureAssetPath.generic_string());

			if (!Registry.Get<UTexture>(TextureId))
			{
				FRenderer* Renderer = FRenderResourceLibrary::Get().GetRenderer();
				FVector4 Color(Mtl.Kd.X, Mtl.Kd.Y, Mtl.Kd.Z, 1.0f);				
				auto RawTexture = Renderer->CreateSolidTexture(Color);

				UTexture* SolidTexture = NewObject<UTexture>();

				UTextureDesc TexDesc{};
				TexDesc.ID = TextureId;
				TexDesc.Name = TextureAssetPath.generic_string();
				TexDesc.Texture = RawTexture.get();

				FRenderResourceLibrary::Get().RegisterTexture(TextureId.ToString(), RawTexture);
				SolidTexture->Load(TexDesc);
				Registry.Register(TextureId, SolidTexture);
			}
		}

		const fs::path MaterialAssetPath = RootPath / fs::path(Mtl.MaterialName);
		FName MaterialId = FName(MaterialAssetPath.generic_string());
		if (!Registry.Get<UMaterial>(MaterialId))
		{
			FArchive SamplerArchive;
			SamplerArchive.SetString("FilterMode", "Bilinear");
			SamplerArchive.SetString("WrapMode", "Wrap");

			FArchive MaterialArchive;
			MaterialArchive.SetString("Name", MaterialAssetPath.generic_string());
			MaterialArchive.SetString("UPipelineID", "Pipeline/Textured.json");

			MaterialArchive.SetString("UTextureID", TextureId.ToString());			
			MaterialArchive.SetArchive("TextureSampler", SamplerArchive);

			LoadMaterialAsset(MaterialArchive, MaterialId);
		}
	}
}
