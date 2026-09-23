#pragma once

#include "Runtime/Core/TMap.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/IntTypes.h"

struct FNameEntry
{
	int32 ComparisonBucketIndex = 0;
	int32 ComparisonIndex = 0;
	int32 DisplayBucketIndex = 0;
	int32 DisplayIndex = 0;
};

/// <summary>
/// 언리얼의 FName을 구현하는 문자열입니다.
/// </summary>
class FName
{

private:
	FNameEntry Entry;
	FString Name;

public:
	FName();
	FName(const char* CharPtr);
	FName(const FString& Str);

	bool IsNone() const;



	/// <summary>
	/// 두 FName의 문자열을 사전식으로 비교합니다.
	/// 두 문자열이 같지 않다면 전체 문자열 비교를 수행합니다.
	/// 대소문자를 구분하지 않습니다.
	/// </summary>
	/// <param name="Other">대소관계를 구하려는 상대 문자열</param>
	/// <returns>std::string::compare()가 반환하는 값</returns>
	int32 Compare(const FName& Other) const;

	/// <summary>
	/// 두 FName의 문자열을 사전식으로 비교합니다.
	/// 두 문자열이 같지 않다면 전체 문자열 비교를 수행합니다.
	/// 대소문자를 구분합니다.
	/// </summary>
	/// <param name="Other">대소관계를 구하려는 상대 문자열</param>
	/// <returns>std::string::compare()가 반환하는 값</returns>
	int32 CompareSensitive(const FName& Other) const;

	/// <summary>
	/// 두 FName이 동일한지 확인합니다. 단순한 정수 비교를 수행합니다.
	/// 대소문자를 구분하지 않습니다.
	/// </summary>
	/// <param name="Other">대소관계를 구하려는 상대 문자열</param>
	/// <returns>두 문자열이 같은지 여부</returns>
	bool operator==(const FName& Other) const;

	/// <summary>
	/// FName을 원본 문자열로 변환합니다.
	/// </summary>
	/// <returns>생성할 때 사용된 원본 문자열</returns>
	FString ToString() const;

	/// <summary>
	/// FName으로 해싱 값을 만듭니다.
	/// </summary>
	/// <returns>FName의 해싱 값</returns>
	size_t GetHash() const;
};

namespace std
{
	template <>
	struct hash<FName>
	{
		size_t operator()(const FName& Name) const noexcept
		{
			return Name.GetHash();
		}
	};
}