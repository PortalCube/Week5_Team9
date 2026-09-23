#include "FNamePool.h"


// 해시 함수
// 어쩌면 그냥 이미 있는 해시 함수 라이브러리를 쓰는게 편하고 더 좋을지도..

namespace
{
	uint64 Hash(FStringView Str)
	{
		constexpr uint64 MOD_NUM = 1'000'000'007;
		constexpr uint64 BASE = 257;

		// 반환될 해시 값
		uint64 Result = 0;

		const char* RawPtr = Str.data();
		for (size_t i = 0; i < Str.size(); ++i)
		{
			// uint64로 1'000'000'007 * 257 정도는 문제 없음
			uint64 byte = static_cast<uint64>(RawPtr[i]);
			Result = (Result * BASE + byte) % MOD_NUM;
		}

		return Result;
	}
}

FNameEntry FNamePool::AddEntry(const FString& Item)
{
	FString LowerItem{ Item };

	for (int i = 0; i < LowerItem.size(); ++i)
	{
		if (LowerItem[i] >= 65 && LowerItem[i] <= 90)
		{
			LowerItem[i] += 32;
		}
	}

	uint64 ComparisonHash = Hash(LowerItem);
	uint64 DisplayHash = Hash(Item);

	FNameEntry Entry;

	// 비트 마스크로 빠른 나머지 계산
	Entry.ComparisonBucketIndex = ComparisonHash & (BUCKET_COUNT - 1);
	Entry.DisplayBucketIndex = DisplayHash & (BUCKET_COUNT - 1);

	TArray<FString>& ComparisonBucket = ComparisonTable[Entry.ComparisonBucketIndex];
	TArray<FString>& DisplayBucket = DisplayTable[Entry.DisplayBucketIndex];

	bool bFound = false;
	for (int i = 0; i < ComparisonBucket.size(); ++i)
	{
		if (LowerItem == ComparisonBucket[i])
		{
			bFound = true;
			Entry.ComparisonIndex = i;
			break;
		}
	}

	if (!bFound)
	{
		Entry.ComparisonIndex = static_cast<int32>(ComparisonBucket.size());
		ComparisonBucket.push_back(LowerItem);
	}


	bFound = false;
	for (int i = 0; i < DisplayBucket.size(); ++i)
	{
		if (Item == DisplayBucket[i])
		{
			bFound = true;
			Entry.DisplayIndex = i;
			break;
		}
	}

	if (!bFound)
	{
		Entry.DisplayIndex = static_cast<int32>(DisplayBucket.size());
		DisplayBucket.push_back(Item);
	}

	return Entry;
}

const FString& FNamePool::GetComparisonString(const FNameEntry& Entry)
{
	return ComparisonTable[Entry.ComparisonBucketIndex][Entry.ComparisonIndex];
}

const FString& FNamePool::GetDisplayString(const FNameEntry& Entry)
{
	return DisplayTable[Entry.DisplayBucketIndex][Entry.DisplayIndex];
}