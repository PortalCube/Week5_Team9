#pragma once

#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Core/PointerTypes.h"
#include "Editor/Visualizer/IVisualizer.h"

class UClass;

class FVisualizerRegistry
{
private:

	TArray<TUniquePtr<IVisualizer>> Visualizers;
	TMap<UClass*, IVisualizer*> Map;

public:
	
	FVisualizerRegistry();

	IVisualizer* FindVisualizer(UClass* ClassType);

};