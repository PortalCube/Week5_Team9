#pragma once
#include "Editor/Core/FEditor.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TSet.h"

enum class EOutlinerItemRowType : uint8
{
	Actor,
	Component
};

struct FOutlinerItem
{
	EOutlinerItemRowType Type = EOutlinerItemRowType::Actor;
	uint64 UUID = 0;
	FString DisplayLabel;
	FString LowerLabel;
	int32 Depth = 0;
	AActor* Actor = nullptr;
	USceneComponent* Component = nullptr;
};

// 월드 아웃라이너 창 클래스
class FImguiWorldOutliner final 
{

public:
	void Process(FEditor& Editor);
	void RefreshCache(UScene* Scene);
	void UpdateFilter(const FString& FilterStr);

private:
	//액터 한 개의 트리노드, 펼쳐지면 컴포넌트까지
	void ShowActorNode(FEditor& Editor, AActor* Actor, const std::string& FilterStr, AActor* SelectedActor);
	void ShowActorNode_Cached(FEditor& Editor, const FOutlinerItem& Item, AActor* SelectedActor);
	void ShowComponentNode(USceneComponent& Component) const;


	// 검색 입력 칸을 그리고, 입력된 문자열을 소문자로 정규화해 돌려준다.
	bool ShowSearchBar();
	char FilterBuffer[128] = {};

	// 펼쳐진 상태를 연속해서 저장하고 삽입한다.
	void RebuildDisplayList();

	UScene* LastScene = nullptr;
	// 원본 액터 데이터 캐시
	TArray<FOutlinerItem> CachedActors;
	TArray<int32> FilteredIndices;
	// 화면에 1줄씩 순서대로 그릴 목록
	TArray<FOutlinerItem> DisplayList;
	// 펼쳐진 액터들의 UUID를 저장하는 집합
	TSet<uint64> ExpandedActorUUIDs;

	size_t LastActorCount = 0;
	FString LastFilterStr = "";
	FString CurrentFilterStr = "";
	bool bCacheDirty = true;
	bool bUseOptimized = true;

};
