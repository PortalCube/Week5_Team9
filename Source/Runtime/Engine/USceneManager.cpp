#include "USceneManager.h"
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include "ThirdParty/Json/json.hpp"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/CoreUObject/FUObjectArray.h"
#include "Runtime/Core/Log.h"

void USceneManager::SaveScene(const FString& path) const
{
	std::filesystem::path fsPath(path);
	std::filesystem::path directory = fsPath.parent_path();

	if (!directory.empty() && !std::filesystem::exists(directory))
		std::filesystem::create_directories(directory);

	FUObjectArray& ObjectArray = FUObjectArray::Get();
	int32 UUID = ObjectArray.GetNextUUID();

	FArchive Archive;
	Archive.SetInt32("Version", 2);
	Archive.SetInt32("NextUUID", UUID);

	FArchive SceneArchive;
	CurrentScene->Serialize(SceneArchive);
	Archive.SetArchive("Scene", SceneArchive);

	std::ofstream file(path);
	if (!file)
	{
		UE_LOG("[SaveScene] 현재 씬을 파일로 저장하는데 실패했습니다. 파일에 쓸 수 없습니다.");
		return;
	}

	file << Archive.GetJSON().dump(4);
}

void USceneManager::LoadScene(const FString& path)
{

	std::ifstream file(path);
	if (!file)
	{
		UE_LOG("[LoadScene] 씬을 파일에서 불러오는데 실패했습니다. 파일을 읽을 수 없습니다.");
		return;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	nlohmann::json JSON = nlohmann::json::parse(buffer.str());
	FArchive Archive{ JSON };

	int32 Version = Archive.GetInt32("Version");
	if (Version != 2)
	{
		UE_LOG("[LoadScene] 로드하려는 파일의 Scene Schema 버전이 다릅니다. 파일의 버전: %d, 지원하는 버전: %d", Version, 2);
		return;
	}

	int32 NextUUID = Archive.GetInt32("NextUUID");
	FUObjectArray& ObjectArray = FUObjectArray::Get();
	ObjectArray.SetNextUUID(NextUUID);

	if (Archive.IsNull("Scene"))
	{
		UE_LOG("[LoadScene] 로드하려는 파일에서 Scene 항목이 없습니다. 파일 형식이 올바르지 않습니다.");
		return;
	}

	FArchive SceneArchive = Archive.GetArchive("Scene");

	UScene* Scene = NewObject<UScene>();
	Scene->Initialize();
	Scene->SetRenderResourceLibrary(&FRenderResourceLibrary::Get());
	Scene->Deserialize(SceneArchive);

	SetScene(Scene);
}

void USceneManager::SetScene(UScene* scene)
{
	if (scene == nullptr) { return; }
	if (scene == CurrentScene) { return; }
	scene->Initialize();
	scene->SetRenderResourceLibrary(&FRenderResourceLibrary::Get());

	if (CurrentScene)
	{
		CurrentScene->EndPlay();
		CurrentScene->Deactivate();
		DestroyObject(CurrentScene);
	}
	CurrentScene = scene;
	CurrentScene->Activate();
	CurrentScene->BeginPlay();
}

void USceneManager::Release()
{
	if (CurrentScene)
	{
		DestroyObject(CurrentScene);
		CurrentScene = nullptr;
	}
}
