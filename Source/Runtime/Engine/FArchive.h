#pragma once

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Math/FVector4.h"
#include "ThirdParty/Json/json.hpp"

#include "Runtime/Utility/EngineUtil.h"

/// <summary>
/// UObject의 데이터를 직렬화/역직렬화 하는 클래스입니다.
/// UObject의 데이터를 이 클래스에 담을 수도 있고, 이 데이터로 UObject를 만들 수도 있습니다.
/// </summary>
class FArchive
{
	// 언젠가 JSON이 아니라 네트워크 패킷에서 직렬화/역직렬화 데이터를 가져올 일이 있을지도 모름 (ex. 멀티플레이)
	// 그래서 JSON을 Serialize/Deserialize 함수에 때려박지 않고 이 클래스가 별도로 존재하는 것

	// 따라서, 언젠가는 이 코드가 JSON에 강하게 커플링된 문제를 해소해야할지도 모름

private:
	nlohmann::json Object;

public:
	FArchive();
	explicit FArchive(const nlohmann::json& InObject);

	nlohmann::json GetJSON() const { return Object; }

	int32 GetInt32(const FString& Key) const;
	void SetInt32(const FString& Key, int32 Value);

	float GetFloat(const FString& Key) const;
	void SetFloat(const FString& Key, float Value);

	uint32 GetUInt32(const FString& Key) const;
	void SetUInt32(const FString& Key, uint32 Value);

	double GetDouble(const FString& Key) const;
	void SetDouble(const FString& Key, double Value);

	bool GetBool(const FString& Key) const;
	void SetBool(const FString& Key, bool Value);

	FString GetString(const FString& Key) const;
	void SetString(const FString& Key, const FString& Value);

	FWString GetWString(const FString& Key) const;
	void SetWString(const FString& Key, const FWString& Value);

	bool IsNull(const FString& Key) const;
	void SetNull(const FString& Key);

	FVector GetVector(const FString& Key) const;
	void SetVector(const FString& Key, const FVector& Value);

	FVector2 GetVector2(const FString& Key) const;
	void SetVector2(const FString& Key, const FVector2& Value);

	FVector4 GetVector4(const FString& Key) const;
	void SetVector4(const FString& Key, const FVector4& Value);
	
	template <typename T>
	TArray<T> GetArray(const FString& Key) const;

	template <typename T>
	void SetArray(const FString& Key, const TArray<T>& Value);

	TArray<FArchive> GetArchiveArray(const FString& Key) const;
	void SetArchiveArray(const FString& Key, const TArray<FArchive>& Value);

	FArchive GetArchive(const FString& Key) const;
	void SetArchive(const FString& Key, const FArchive& Archive);

	template <typename T>
	T GetEnum(const FString& Key, TMap<FString, T>& EnumMap);

	template <typename T>
	void SetEnum(const FString& Key, T Value, TMap<T, FString>& EnumMap);
};

template<typename T>
inline TArray<T> FArchive::GetArray(const FString& Key) const
{
	TArray<T> Array;

	for (const auto& Item : Object.at(Key))
	{
		T Value = Item.get<T>();
		Array.push_back(Value);
	}

	return Array;
}

template<typename T>
inline void FArchive::SetArray(const FString& Key, const TArray<T>& Value)
{
	Object[Key] = nlohmann::json::array();

	for (int i = 0; i < Value.size(); ++i)
	{
		Object[Key].push_back(Value[i]);
	}
}

template<typename T>
inline T FArchive::GetEnum(const FString& Key, TMap<FString, T>& EnumMap)
{
	FString Value = GetString(Key);

	auto It = EnumMap.find(Value);
	if (It == EnumMap.end())
	{
		throw EngineUtil::CreateError("[FArchive::GetEnum] 키 {}에서 대해서 EnumMap에 없는 값이 있습니다. ({})", Key, Value);
	}

	return It->second;
}

template<typename T>
inline void FArchive::SetEnum(const FString& Key, T Value, TMap<T, FString>& EnumMap)
{
	auto& It = EnumMap.find(Key);

	if (It == EnumMap.end())
	{
		throw EngineUtil::CreateError("[FArchive::SetEnum] 키 {}에서 대해서 EnumMap에 없는 값이 있습니다. ({})", Key, Value);
	}

	SetString(Key, It.second);
}
