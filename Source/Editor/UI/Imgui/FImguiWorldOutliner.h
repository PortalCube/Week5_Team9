#pragma once
#include "Editor/Core/FEditor.h"
#include "Runtime/Core/TArray.h"

// 월드 아웃라이너 창 클래스
class FImguiWorldOutliner final 
{

public:
	void Process(FEditor& Editor);

private:
	//액터 한 개의 트리노드, 펼쳐지면 컴포넌트까지
	void ShowActorNode(FEditor& Editor, AActor* Actor, const std::string& FilterStr, AActor* SelectedActor);
	void ShowComponentNode(USceneComponent& Component) const;


	// 검색 입력 칸을 그리고, 입력된 문자열을 소문자로 정규화해 돌려준다.
	std::string ShowSearchBar();
	char FilterBuffer[128] = {};

};
