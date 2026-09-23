#include "FAssetRegistry.h"
#include "Runtime/Utility/EngineUtil.h"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace
{
	const fs::path InternalStaticMeshPath = "InternalStaticMesh";

	bool IsSubpath(fs::path& OutTargetPath, bool& bOutIsDirectChild, const fs::path& Parent, const fs::path& Child)
	{
		fs::path ParentNormal = Parent.lexically_normal();
		fs::path ChildNormal = Child.lexically_normal();

		fs::path Relative = ChildNormal.lexically_relative(ParentNormal);

		if (Relative.empty() || *Relative.begin() == ".." || *Relative.begin() == ".") { return false; }
		
		auto RelativeIt = Relative.begin();
		OutTargetPath = *RelativeIt;
		bOutIsDirectChild = ++RelativeIt == Relative.end();
		return true;
	}
}

FAssetRegistry& FAssetRegistry::GetInstance()
{
	static FAssetRegistry Instance;
	return Instance;
}

void FAssetRegistry::Register(const FName& Name, UAsset* Pipeline)
{
	auto It = AssetMap.find(Name);
	if (It != AssetMap.end())
	{
		throw EngineUtil::CreateError("UAsset 등록 실패. 이미 중복된 이름({})이 존재합니다.", Name.ToString());
	}

	AssetMap.insert({ Name, Pipeline });
	DirectoryCache.clear();
}

void FAssetRegistry::Clear()
{
	AssetMap.clear();
	DirectoryCache.clear();
}

FFolderView FAssetRegistry::GetAssetDirectory(const fs::path& ParentPath) const
{
	const auto& Item = DirectoryCache.find(ParentPath);

	if (Item != DirectoryCache.end())
	{
		return Item->second;
	}

	FFolderView Result;
	TSet<fs::path> FolderSet;

	if (ParentPath == InternalStaticMeshPath)
	{
		for (const auto& [AssetID, Asset] : GetAssetMap())
		{
			const FString AssetIDString = AssetID.ToString();
			if (!AssetIDString.empty() && AssetIDString.front() == '#' && Asset->IsA<UStaticMesh>())
			{
				Result.Assets.push_back(Asset);
			}
		}
	}
	else if (ParentPath.empty())
	{
		// 코드에서 생성한 내부 StaticMesh는 일반 애셋 트리와 분리해 보여준다.
		FolderSet.insert(InternalStaticMeshPath);
	}

	for (const auto& [AssetID, Asset] : GetAssetMap())
	{
		const FString AssetIDString = AssetID.ToString();
		if (ParentPath == InternalStaticMeshPath ||
			(!AssetIDString.empty() && AssetIDString.front() == '#'))
		{
			continue;
		}

		fs::path AssetPath{ AssetIDString };
		fs::path TargetPath;
		bool bIsDirectChild = false;

		if (!IsSubpath(TargetPath, bIsDirectChild, ParentPath, AssetPath))
		{
			continue;
		}

		if (bIsDirectChild)
		{
			Result.Assets.push_back(Asset);
		}
		else
		{
			FolderSet.insert(TargetPath);
		}
	}

	Result.Folders.assign(FolderSet.begin(), FolderSet.end());
	std::sort(Result.Folders.begin(), Result.Folders.end(), [](const fs::path& Left, const fs::path& Right)
	{
		return Left.generic_string() < Right.generic_string();
	});

	std::sort(Result.Assets.begin(), Result.Assets.end(), [](const UAsset* Left, const UAsset* Right)
	{
		const FString LeftName = Left->GetName().ToString();
		const FString RightName = Right->GetName().ToString();
		if (LeftName != RightName)
		{
			return LeftName < RightName;
		}

		return Left->GetID().ToString() < Right->GetID().ToString();
	});

	DirectoryCache[ParentPath] = Result;

	return Result;
}
