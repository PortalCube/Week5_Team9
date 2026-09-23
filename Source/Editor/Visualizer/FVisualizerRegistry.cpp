#include "FVisualizerRegistry.h"

#include "Editor/Visualizer/FPrimitiveVisualizer.h"
#include "Editor/Visualizer/FSpotlightVisualizer.h"
#include "Editor/Visualizer/FBillboardVisualizer.h"
#include "Editor/Visualizer/FTextVisualizer.h"

#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"
#include "Runtime/CoreUObject/UBillBoardComp.h"
#include "Runtime/CoreUObject/UAnimatedBillboardComp.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"

FVisualizerRegistry::FVisualizerRegistry()
{
	// 기본 프리미티브 비주얼라이저 등록
	Visualizers.push_back(MakeUnique<FPrimitiveVisualizer>());
	Map[UPrimitiveComponent::StaticClass()] = Visualizers.back().get();
	
	Visualizers.push_back(MakeUnique<FSpotlightVisualizer>());
	Map[USpotLightComponent::StaticClass()] = Visualizers.back().get();

	Visualizers.push_back(MakeUnique<FBillboardVisualizer>());
	Map[UBillBoardComp::StaticClass()] = Visualizers.back().get();
	Map[UAnimatedBillboardComp::StaticClass()] = Visualizers.back().get();

	Visualizers.push_back(MakeUnique<FTextVisualizer>());
	Map[UTextInstanceComponent::StaticClass()] = Visualizers.back().get();
}

IVisualizer* FVisualizerRegistry::FindVisualizer(UClass* ClassType)
{
	// 비트 마스크 연산을 냅두고 이걸 써도 되는걸까..
	while (ClassType != nullptr)
	{
		auto Item = Map.find(ClassType);

		if (Item == Map.end())
		{
			ClassType = ClassType->GetSuperClass();
			continue;
		}

		return Item->second;
	}

	return Visualizers[0].get(); // FPrimitiveVisualizer
}
