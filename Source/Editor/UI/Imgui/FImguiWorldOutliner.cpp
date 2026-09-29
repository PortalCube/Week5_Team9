#include "FImguiWorldOutliner.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/UScene.h"
#include "ThirdParty/Imgui/imgui.h"
#include <string>
#include <algorithm>

void FImguiWorldOutliner::Process(FEditor& Editor)
{
	ImGui::Begin("World Outliner");

	UScene* Scene = Editor.GetCurrentScene();
	if (!Scene)
	{
		ImGui::TextDisabled("No Active Scene");
		ImGui::End();
		return;
	}

	// 검색 필터 버퍼
	const std::string FilterStr = ShowSearchBar();
	ImGui::Separator();

	auto& Actors = Scene->GetActors();
	AActor* SelectedActor = Editor.GetSelectedActor();
	// 액터 목록 표시
	ImGui::BeginChild("ActorList", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()), false);

	for (AActor* Actor : Actors)
	{
		if (!Actor)
		{
			continue;
		}
		//액터 노드 표시
		ShowActorNode(Editor, Actor, FilterStr,SelectedActor);
	}

	ImGui::EndChild();

	ImGui::Separator();

	// 하단 컨트롤 영역
	if (SelectedActor)
	{
		if (ImGui::Button("Delete"))
		{
			AActor* ActorToDelete = SelectedActor;
			Editor.UnSelectActor();
			ActorToDelete->Destroy();
		}
	}
	else
	{
		ImGui::TextDisabled("No Selection");
	}

	ImGui::End();
}

void FImguiWorldOutliner::ShowActorNode(FEditor& Editor,AActor* Actor, const std::string& FilterStr, AActor* SelectedActor)
{
	if (!Actor->GetClass()) { return; }

	// 검색어 필터링
	if (!FilterStr.empty())
	{
		// 액터 이름 생성
		const FString& ActorName = Actor->GetClass()->GetDisplayName();
		if (ActorName.find(FilterStr) == FString::npos)
		{
			return;
		}
	}

	const bool bIsSelected = (Actor == SelectedActor);
	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (bIsSelected)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	const auto& Components = Actor->GetAttachedComponents();
	if (Components.empty())
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	// 트리 노드 렌더링
	const bool bNodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(Actor->GetUUID())), NodeFlags, "%s (ID: %u)", Actor->GetClass()->GetDisplayName().c_str(), Actor->GetUUID());

	// 클릭 시 액터 선택
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		Editor.SelectActor(Actor);
	}


	// 자식 컴포넌트 목록 전개
	if (bNodeOpen && !Components.empty())
	{
		for (USceneComponent* Comp : Components)
		{
			if (!Comp)
			{
				return;
			}
			ShowComponentNode(*Comp);

		}

		ImGui::TreePop();
	}

}

void FImguiWorldOutliner::ShowComponentNode(USceneComponent& Comp) const
{
	const char* CompClassName = Comp.GetClass() ? Comp.GetClass()->GetDisplayName().c_str() : "Component";
	
	ImGuiTreeNodeFlags CompFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(Comp.GetUUID())), CompFlags, "%s (ID: %u)", CompClassName, Comp.GetUUID());
}



std::string FImguiWorldOutliner::ShowSearchBar()
{
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputTextWithHint("##OutlinerFilter", "Search...", FilterBuffer, sizeof(FilterBuffer));

	std::string FilterStr = FilterBuffer;
	std::transform(FilterStr.begin(), FilterStr.end(), FilterStr.begin(), ::tolower);
	return FilterStr;
}
