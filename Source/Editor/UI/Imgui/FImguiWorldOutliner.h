#pragma once
#include "Editor/Core/FEditor.h"
#include "Runtime/Core/TArray.h"

struct FWorldOutlinerRow
{
	enum class EType : uint8
	{
		Actor,
		Component,
	};

	EType Type;
	UObject* Object;
	int32 Depth;
};

// 월드 아웃라이너 창 클래스
class FImguiWorldOutliner final 
{

public:
	void Process(FEditor& Editor);

private:

	void ShowActorHierarchy();

	void BuildVisibleRows(
		TArray<FWorldOutlinerRow>& VisibleRows,
		const TArray<AActor*>& Actors,
		const std::string& FilterStr
	) const;

	// Clipper가 각 항목을 동일한 높이의 한 줄로 취급할 수 있도록
	// 액터와 컴포넌트를 각각 독립된 행으로 그린다.
	void ShowActorNode(FEditor& Editor, AActor* Actor, AActor* SelectedActor);
	void ShowComponentNode(USceneComponent& Component) const;


	// 검색 입력 칸을 그리고, 입력된 문자열을 소문자로 정규화해 돌려준다.
	std::string ShowSearchBar();
	char FilterBuffer[128] = {};

};
