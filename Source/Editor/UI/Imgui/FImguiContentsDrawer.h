#pragma once
#include "Editor/Core/FEditor.h"



class UTexture;
class UPipeline;
class UMaterial;
class UStaticMesh;
class UFont;

class FImguiContentsDrawer final
{

public:
	FImguiContentsDrawer();
	~FImguiContentsDrawer() = default;

	//복사 생성 금지
	FImguiContentsDrawer(const FImguiContentsDrawer&) = delete;
	//복사 대입 금지
	FImguiContentsDrawer& operator=(const FImguiContentsDrawer&) = delete;

	void Process(FEditor& Editor);
	std::filesystem::path RootPath;
	std::filesystem::path CurrentPath;
	float LeftPanelWidth;

private:
	//폴더 트리 렌더
	void RenderFolderTree();
	void RenderFolderTreeNode(const std::filesystem::path& FolderPath);

	// 우측 파일 목록
	void RenderContentView();


	// 폴더 안의 항목 하나.
	// 이름은 표시용으로 미리 UTF-8로 변환해 둔다. ImGui는 UTF-8만 받는다.
	struct FContentEntry
	{
		std::filesystem::path Path;
		FString DisplayName;
		FString Extension;   // 소문자. 썸네일 조회와 필터에 쓴다
		bool bIsDirectory = false;
	};

	std::filesystem::path SelectedPath;
	float ThumbnailSize = 128.0f;
};
