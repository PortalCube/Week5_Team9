#pragma once

#include "Runtime/Core/TArray.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/FName.h"

// 값을 적절하게 조절할 것
constexpr int32 BUCKET_COUNT = 1 << 13; // 8192

// 스태틱 전역 클래스
class FNamePool
{
	friend class FName;

	static inline TArray<TArray<FString>> ComparisonTable{ BUCKET_COUNT };
	static inline TArray<TArray<FString>> DisplayTable{ BUCKET_COUNT };

	static FNameEntry AddEntry(const FString& Item);
	static const FString& GetComparisonString(const FNameEntry& Entry);
	static const FString& GetDisplayString(const FNameEntry& Entry);
};