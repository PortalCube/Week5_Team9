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

	ImGui::Checkbox("아웃라이너 최적화 적용", &bUseOptimized);
	if(ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("체크: 캐쉬된 라벨 및 화면에 보이는 일부 노드만 랜더\n"
						  "해제: 매 프레임 동적 생성 및 전체 순회");
	}
	ImGui::Separator();

	// 검색 필터 버퍼
	const bool bFilterChanged = ShowSearchBar();
	ImGui::Separator();

	auto& Actors = Scene->GetActors();
	AActor* SelectedActor = Editor.GetSelectedActor();
	// 액터 목록 표시
	ImGui::BeginChild("ActorList", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()), false);

	if (bUseOptimized)
	{
		if(bCacheDirty || Scene != LastScene || Actors.size() != LastActorCount)
		{
			RefreshCache(Scene);
			UpdateFilter(CurrentFilterStr.c_str());
			RebuildDisplayList();
			LastScene = Scene;
		}
		else if (bFilterChanged)
		{
			UpdateFilter(CurrentFilterStr.c_str());
			RebuildDisplayList();
		}

		ImGuiListClipper Clipper;
		Clipper.Begin(static_cast<int>(DisplayList.size()));

		while (Clipper.Step())
		{
			for (int i = Clipper.DisplayStart; i < Clipper.DisplayEnd; ++i)
			{
				ShowActorNode_Cached(Editor, DisplayList[i], SelectedActor);
			}
		}
	}
	else
	{
		for (AActor* Actor : Actors)
		{
			if (!Actor)
			{
				continue;
			}
			//액터 노드 표시
			ShowActorNode(Editor, Actor, CurrentFilterStr.c_str(), SelectedActor);
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
			bCacheDirty = true;
		}
	}
	else
	{
		ImGui::TextDisabled("No Selection");
	}

	ImGui::End();
}

void FImguiWorldOutliner::RefreshCache(UScene* Scene)
{
	CachedActors.clear();
	const auto& Actors = Scene->GetActors();
	CachedActors.reserve(Actors.size());

	for(AActor* Actor : Actors)
	{
		if (!Actor) { continue; }

		FOutlinerItem Item;
		Item.Type = EOutlinerItemRowType::Actor;
		Item.Actor = Actor;
		Item.UUID = Actor->GetUUID();

		const char* ClassName = Actor->GetClass() ? Actor->GetClass()->GetDisplayName().c_str() : "Actor";
		Item.DisplayLabel = FString(ClassName) + " (ID: " + std::to_string(Item.UUID) + ")";

		Item.LowerLabel = Item.DisplayLabel;
		std::transform(Item.LowerLabel.begin(), Item.LowerLabel.end(), Item.LowerLabel.begin(),
			[](unsigned char c) { return static_cast<char>(::tolower(c)); });

		CachedActors.push_back(std::move(Item));
	}

	LastActorCount = Actors.size();
	bCacheDirty = false;
}

void FImguiWorldOutliner::UpdateFilter(const FString& FilterStr)
{
	FilteredIndices.clear();
	FilteredIndices.reserve(CachedActors.size());

	const bool bHasFilter = !FilterStr.empty();

	for(int32 i = 0; i < static_cast<int32>(CachedActors.size()); ++i)
	{
		if(!bHasFilter || CachedActors[i].LowerLabel.find(FilterStr) != FString::npos)
		{
			FilteredIndices.push_back(i);
		}
	}

	LastFilterStr = FilterStr;
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

void FImguiWorldOutliner::ShowActorNode_Cached(FEditor& Editor, const FOutlinerItem& Item, AActor* SelectedActor)
{
	if (Item.Depth > 0)
	{
		ImGui::Indent(Item.Depth * 16.0f);
	}

	if (Item.Type == EOutlinerItemRowType::Actor)
	{
		AActor* Actor = Item.Actor;
		if (!Actor) return;

		const bool bIsOpen = ExpandedActorUUIDs.contains(Item.UUID);
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

		const bool bNodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(Item.UUID)), NodeFlags, "%s", Item.DisplayLabel.c_str());

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			Editor.SelectActor(Actor);
		}

		if(bNodeOpen != bIsOpen)
		{
			if(bNodeOpen)
			{
				ExpandedActorUUIDs.insert(Item.UUID);
			}
			else
			{
				ExpandedActorUUIDs.erase(Item.UUID);
			}
			RebuildDisplayList();
		}

		if (bNodeOpen)
		{
			ImGui::TreePop();
		}
	}
	else
	{
		ImGuiTreeNodeFlags CompFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(Item.UUID)), CompFlags, "%s", Item.DisplayLabel.c_str());
	}

	if(Item.Depth > 0)
	{
		ImGui::Unindent(Item.Depth * 16.0f);
	}
}

void FImguiWorldOutliner::ShowComponentNode(USceneComponent& Comp) const
{
	const char* CompClassName = Comp.GetClass() ? Comp.GetClass()->GetDisplayName().c_str() : "Component";
	
	ImGuiTreeNodeFlags CompFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(Comp.GetUUID())), CompFlags, "%s (ID: %u)", CompClassName, Comp.GetUUID());
}



bool FImguiWorldOutliner::ShowSearchBar()
{
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputTextWithHint("##OutlinerFilter", "Search...", FilterBuffer, sizeof(FilterBuffer)))
	{
		CurrentFilterStr = FilterBuffer;

		std::transform(CurrentFilterStr.begin(), CurrentFilterStr.end(), CurrentFilterStr.begin(), ::tolower);
		
		return true;;
	}

	return false;
}

void FImguiWorldOutliner::RebuildDisplayList()
{
	DisplayList.clear();

	for(int32 ItemIndex : FilteredIndices)
	{
		const FOutlinerItem& ActorItem = CachedActors[ItemIndex];
		AActor* Actor = ActorItem.Actor;
		if (!Actor) continue;

		FOutlinerItem Row = ActorItem;
		Row.Depth = 0;
		DisplayList.push_back(Row);

		if(ExpandedActorUUIDs.contains(ActorItem.UUID))
		{
			for(USceneComponent* Comp : Actor->GetAttachedComponents())
			{
				if (!Comp) continue;
				
				FOutlinerItem CompItem;
				CompItem.Type = EOutlinerItemRowType::Component;
				CompItem.UUID = Comp->GetUUID();
				const char* CompName = Comp->GetClass() ? Comp->GetClass()->GetDisplayName().c_str() : "Component";
				CompItem.DisplayLabel = FString(CompName) + " (ID: " + std::to_string(CompItem.UUID) + ")";
				CompItem.Depth = 1;
				CompItem.Component = Comp;
				CompItem.Actor = Actor;

				DisplayList.push_back(CompItem);
			}
		}
	}
}
