#include "FImguiWorldOutliner.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/UScene.h"
#include "ThirdParty/Imgui/imgui.h"
#include <algorithm>
#include <cctype>
#include <string>

void FImguiWorldOutliner::Process(FEditor& Editor)
{
	if (!ImGui::Begin("World Outliner"))
	{
		ImGui::End();
		return;
	}

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

	TArray<FWorldOutlinerRow> VisibleRows;
	BuildVisibleRows(VisibleRows, Actors, FilterStr);

	// 액터들 중 현재 스크롤에 보이는 부분만 렌더링
	ImGuiListClipper Clipper;
	Clipper.Begin(static_cast<int>(VisibleRows.size()));
	while (Clipper.Step())
	{
		for (int32 RowIndex = Clipper.DisplayStart; RowIndex < Clipper.DisplayEnd; ++RowIndex)
		{
			const FWorldOutlinerRow& Row = VisibleRows[RowIndex];

			if (Row.Depth > 0)
			{
				ImGui::Indent();
			}

			if (Row.Type == FWorldOutlinerRow::EType::Actor)
			{
				ShowActorNode(Editor, static_cast<AActor*>(Row.Object), SelectedActor);
			}
			else
			{
				ShowComponentNode(*static_cast<USceneComponent*>(Row.Object));
			}

			if (Row.Depth > 0)
			{
				ImGui::Unindent();
			}
		}
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

void FImguiWorldOutliner::BuildVisibleRows(
	TArray<FWorldOutlinerRow>& VisibleRows,
	const TArray<AActor*>& Actors,
	const std::string& FilterStr) const
{
	VisibleRows.reserve(Actors.size());

	for (AActor* Actor : Actors)
	{
		if (!Actor || !Actor->GetClass())
		{
			continue;
		}

		if (!FilterStr.empty())
		{
			FString LowerName = Actor->GetClass()->GetDisplayName();

			std::transform(LowerName.begin(), LowerName.end(), LowerName.begin(),
				[](unsigned char Character)
				{
					return static_cast<char>(std::tolower(Character));
				}
			);

			if (LowerName.find(FilterStr) == FString::npos)
			{
				continue;
			}
		}

		VisibleRows.push_back({ FWorldOutlinerRow::EType::Actor, Actor, 0 });

		const auto& Components = Actor->GetAttachedComponents();
		if (Components.empty())
		{
			continue;
		}

		const void* ActorId = reinterpret_cast<void*>(static_cast<uintptr_t>(Actor->GetUUID()));
		const bool bIsOpen = ImGui::GetStateStorage()->GetBool(ImGui::GetID(ActorId));
		if (!bIsOpen)
		{
			continue;
		}

		for (USceneComponent* Component : Components)
		{
			if (Component)
			{
				VisibleRows.push_back({ FWorldOutlinerRow::EType::Component, Component, 1 });
			}
		}
	}
}

void FImguiWorldOutliner::ShowActorNode(FEditor& Editor, AActor* Actor, AActor* SelectedActor)
{
	if (!Actor->GetClass()) { return; }

	const bool bIsSelected = (Actor == SelectedActor);
	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	if (bIsSelected)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	const auto& Components = Actor->GetAttachedComponents();
	if (Components.empty())
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf;
	}

	uint32 UUID = Actor->GetUUID();
	uintptr_t UUIDPtr = static_cast<uintptr_t>(Actor->GetUUID());
	void* Ptr = reinterpret_cast<void*>(UUIDPtr);

	const char* Name = Actor->GetClass()->GetDisplayName().c_str();

	// 트리 노드 렌더링
	ImGui::TreeNodeEx(Ptr, NodeFlags, "%s (ID: %u)", Name, UUID);

	// 클릭 시 액터 선택
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		Editor.SelectActor(Actor);
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
