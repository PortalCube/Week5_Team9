#pragma once

#include "Runtime/Core/TMap.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Resource/FResourceLoader.h"

#include <algorithm>
#include <filesystem>
#include <numbers>

// 임시 코드 - 대회 끝나고 지울 것
namespace Converter
{
	inline TMap<FString, FString> LoadedObjs;

	inline void ApplyPerspectiveCamera(const FArchive& Archive, FCamera* OutCamera)
	{
		if (!OutCamera || Archive.IsNull("PerspectiveCamera"))
		{
			return;
		}

		const FArchive CameraArchive = Archive.GetArchive("PerspectiveCamera");
		const TArray<float> FOV = CameraArchive.GetArray<float>("FOV");
		const TArray<float> NearClip = CameraArchive.GetArray<float>("NearClip");
		const TArray<float> FarClip = CameraArchive.GetArray<float>("FarClip");
		const FVector Rotation = CameraArchive.GetVector("Rotation");

		constexpr float RadToDeg = 180.0f / std::numbers::pi_v<float>;
		OutCamera->SetProjectionType(EProjectionType::Perspective);
		OutCamera->SetPosition(CameraArchive.GetVector("Location"));
		OutCamera->SetRotation(Rotation.Y * RadToDeg, Rotation.Z * RadToDeg);

		if (!FOV.empty()) { OutCamera->SetFOV(FOV[0]); }
		if (!NearClip.empty()) { OutCamera->SetNearPlane(NearClip[0]); }
		if (!FarClip.empty()) { OutCamera->SetFarPlane(FarClip[0]); }

		OutCamera->SetYaw(45.0f);
		OutCamera->SetPitch(-25.0f);
	}

	inline FArchive GetStandardArchive(
		const FArchive& Archive,
		const std::filesystem::path& SceneFilePath,
		FCamera* OutCamera = nullptr)
	{
		ApplyPerspectiveCamera(Archive, OutCamera);

		int32 NextUUID = Archive.GetInt32("NextUUID");

		FArchive SceneArchive;
		SceneArchive.SetInt32("UUID", NextUUID++);
		SceneArchive.SetString("Type", "UScene");

		TArray<FArchive> Actors;
		const nlohmann::json Primitives = Archive.GetArchive("Primitives").GetJSON();

		for (const auto& [PrimitiveKey, PrimitiveJson] : Primitives.items())
		{
			const FArchive Primitive{ PrimitiveJson };
			const FString Type = Primitive.GetString("Type");

			if (Type != "StaticMeshComp")
			{
				UE_LOG("[Converter] Invalid Type: %s", Type.c_str());
				continue;
			}

			int32 ActorUUID = 0;
			try
			{
				ActorUUID = std::stoi(PrimitiveKey);
			}
			catch (std::exception e)
			{
				UE_LOG("[Converter] Invalid primitive UUID: %s", PrimitiveKey.c_str());
				continue;
			}

			NextUUID = std::max(NextUUID, ActorUUID + 1);

			const std::filesystem::path ObjPath = (SceneFilePath.parent_path() / Primitive.GetString("ObjStaticMeshAsset")).lexically_normal();
			const FString ObjKey = ObjPath.generic_string();
			FString AssetId;

			const auto LoadedIt = LoadedObjs.find(ObjKey);

			if (LoadedIt != LoadedObjs.end())
			{
				AssetId = LoadedIt->second;
			}
			else
			{
				FResourceLoader::ImportObj(ObjPath, &AssetId);
				LoadedObjs.emplace(ObjKey, AssetId);
			}

			FArchive RootComponent;
			RootComponent.SetString("Type", "UStaticMeshComponent");
			RootComponent.SetInt32("UUID", NextUUID++);
			RootComponent.SetVector("Location", Primitive.GetVector("Location"));
			RootComponent.SetVector("Rotation", Primitive.GetVector("Rotation"));
			RootComponent.SetVector("Scale", Primitive.GetVector("Scale"));
			RootComponent.SetString("MeshAsset", AssetId);

			FArchive Actor;
			Actor.SetString("Type", "AActor");
			Actor.SetInt32("UUID", ActorUUID);
			Actor.SetArchive("RootComponent", RootComponent);
			Actors.push_back(Actor);
		}

		SceneArchive.SetArchiveArray("Actors", Actors);

		FArchive NewArchive;
		NewArchive.SetInt32("Version", 2);
		NewArchive.SetInt32("NextUUID", NextUUID);
		NewArchive.SetArchive("Scene", SceneArchive);
		return NewArchive;
	}
}
