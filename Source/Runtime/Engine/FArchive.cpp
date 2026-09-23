#include "FArchive.h"

#include "Runtime/Utility/WindowsUtil.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

FArchive::FArchive()
	: Object()
{
}

FArchive::FArchive(const nlohmann::json& InObject)
	: Object(InObject)
{
}

int32 FArchive::GetInt32(const FString& Key) const
{
	return Object.at(Key).get<int32>();
}

void FArchive::SetInt32(const FString& Key, int32 Value)
{
	Object[Key] = Value;
}

float FArchive::GetFloat(const FString& Key) const
{
	return Object.at(Key).get<float>();
}

void FArchive::SetFloat(const FString& Key, float Value)
{
	Object[Key] = Value;
}

uint32 FArchive::GetUInt32(const FString& Key) const
{
	return Object.at(Key).get<uint32>();
}

void FArchive::SetUInt32(const FString& Key, uint32 Value)
{
	Object[Key] = Value;
}

double FArchive::GetDouble(const FString& Key) const
{
	return Object.at(Key).get<double>();
}

void FArchive::SetDouble(const FString& Key, double Value)
{
	Object[Key] = Value;
}

bool FArchive::GetBool(const FString& Key) const
{
	return Object.at(Key).get<bool>();
}

void FArchive::SetBool(const FString& Key, bool Value)
{
	Object[Key] = Value;
}

FString FArchive::GetString(const FString& Key) const
{
	return Object.at(Key).get<FString>();
}

void FArchive::SetString(const FString& Key, const FString& Value)
{
	Object[Key] = Value;
}

FWString FArchive::GetWString(const FString& Key) const
{
	FString Result = Object.at(Key).get<FString>();
	return WindowsUtil::ToWString(Result);
}

void FArchive::SetWString(const FString& Key, const FWString& Value)
{
	FString Result = WindowsUtil::ToString(Value);
	Object[Key] = Result;
}

bool FArchive::IsNull(const FString& Key) const
{
	// 주어진 키 자체가 존재하지 않음
	if (!Object.contains(Key)) { return true; }

	// 주어진 키의 value가 null 값임
	if (Object.at(Key).is_null()) { return true; }

	// 값이 있음
	return false;
}

void FArchive::SetNull(const FString& Key)
{
	// 참고: IsNull과는 다르게, SetNull은 반드시 명시적인 null을 지정함
	Object[Key] = nullptr;
}

FVector FArchive::GetVector(const FString& Key) const
{
	TArray<float> Array = GetArray<float>(Key);

	return FVector
	{
		Array[0],
		Array[1],
		Array[2],
	};
}

void FArchive::SetVector(const FString& Key, const FVector& Value)
{
	TArray<float> Array
	{
		Value.X,
		Value.Y,
		Value.Z,
	};

	SetArray(Key, Array);
}

FVector2 FArchive::GetVector2(const FString& Key) const
{
	TArray<float> Array = GetArray<float>(Key);

	return FVector2
	{
		Array[0],
		Array[1],
	};
}

void FArchive::SetVector2(const FString& Key, const FVector2& Value)
{
	TArray<float> Array
	{
		Value.X,
		Value.Y,
	};

	SetArray(Key, Array);
}

FVector4 FArchive::GetVector4(const FString& Key) const
{
	TArray<float> Array = GetArray<float>(Key);

	return FVector4
	{
		Array[0],
		Array[1],
		Array[2],
		Array[3],
	};
}

void FArchive::SetVector4(const FString& Key, const FVector4& Value)
{
	TArray<float> Array
	{
		Value.X,
		Value.Y,
		Value.Z,
		Value.W,
	};

	SetArray(Key, Array);
}

TArray<FArchive> FArchive::GetArchiveArray(const FString& Key) const
{
	TArray<FArchive> Array;

	for (const auto& Item : Object.at(Key))
	{
		FArchive ItemArchive{ Item };
		Array.push_back(ItemArchive);
	}

	return Array;
}

void FArchive::SetArchiveArray(const FString& Key, const TArray<FArchive>& Value)
{
	Object[Key] = nlohmann::json::array();

	for (const auto& Item : Value)
	{
		Object.at(Key).push_back(Item.GetJSON());
	}
}

FArchive FArchive::GetArchive(const FString& Key) const
{
	return FArchive{ Object.at(Key) };
}

void FArchive::SetArchive(const FString& Key, const FArchive& Archive)
{
	Object[Key] = Archive.GetJSON();
}
