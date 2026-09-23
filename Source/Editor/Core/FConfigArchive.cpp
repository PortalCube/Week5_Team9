#include "FConfigArchive.h"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <format>


FString GetValue(const mINI::INIStructure& Object, const FString& Section, const FString& Key)
{
	if (!Object.has(Section))
	{
		throw std::out_of_range("INI section not found: " + Section);
	}

	const auto& SectionObject = Object.get(Section);
	if (!SectionObject.has(Key))
	{
		throw std::out_of_range("INI key not found: " + Section + "." + Key);
	}

	return SectionObject.get(Key);
}

FConfigArchive::FConfigArchive()
	: Object()
{
}

FConfigArchive::FConfigArchive(const mINI::INIStructure& InObject)
	: Object(InObject)
{
}

int32 FConfigArchive::GetInt32(const FString& Section, const FString& Key) const
{
	FString Value = GetValue(Object, Section, Key);
	return std::stoi(Value);
}

void FConfigArchive::SetInt32(const FString& Section, const FString& Key, int32 Value)
{
	Object[Section][Key] = std::to_string(Value);
}

float FConfigArchive::GetFloat(const FString& Section, const FString& Key) const
{
	FString Value = GetValue(Object, Section, Key);
	return std::stof(Value);
}

void FConfigArchive::SetFloat(const FString& Section, const FString& Key, float Value)
{
	Object[Section][Key] = std::to_string(Value);
}

uint32 FConfigArchive::GetUInt32(const FString& Section, const FString& Key) const
{
	FString Value = GetValue(Object, Section, Key);
	return std::stoul(Value);
}

void FConfigArchive::SetUInt32(const FString& Section, const FString& Key, uint32 Value)
{
	Object[Section][Key] = std::to_string(Value);
}

double FConfigArchive::GetDouble(const FString& Section, const FString& Key) const
{
	FString Value = GetValue(Object, Section, Key);
	return std::stod(Value);
}

void FConfigArchive::SetDouble(const FString& Section, const FString& Key, double Value)
{
	Object[Section][Key] = std::to_string(Value);
}

bool FConfigArchive::GetBool(const FString& Section, const FString& Key) const
{
	const FString Value = GetValue(Object, Section, Key);
	if (Value == "0" || Value == "") { return false; }
	else { return true; }
}

void FConfigArchive::SetBool(const FString& Section, const FString& Key, bool Value)
{
	Object[Section][Key] = Value ? "true" : "false";
}

FString FConfigArchive::GetString(const FString& Section, const FString& Key) const
{
	return GetValue(Object, Section, Key);
}

void FConfigArchive::SetString(const FString& Section, const FString& Key, const FString& Value)
{
	Object[Section][Key] = Value;
}

bool FConfigArchive::IsEmpty(const FString& Section, const FString& Key) const
{
	if (!Object.has(Section)) { return true; }

	const auto SectionObject = Object.get(Section);

	if (!SectionObject.has(Key)) { return true; }
	if (SectionObject.get(Key).empty()) { return true; }
	
	return false;
}

FVector FConfigArchive::GetVector(const FString& Section, const FString& Key) const
{
	FVector Value{};

	for (int i = 0; i < 3; ++i)
	{
		FString ItemKey = std::format("{}.{}", Key, i);
		if (IsEmpty(Section, ItemKey)) { return FVector{}; }

		Value[i] = GetFloat(Section, ItemKey);
	}

	return Value;
}

void FConfigArchive::SetVector(const FString& Section, const FString& Key, const FVector& Value)
{
	for (int i = 0; i < 3; ++i)
	{
		FString ItemKey = std::format("{}.{}", Key, i);
		Object[Section][ItemKey] = std::to_string(Value[i]);
	}
}

FVector2 FConfigArchive::GetVector2(const FString& Section, const FString& Key) const
{
	FVector2 Value{};

	for (int i = 0; i < 2; ++i)
	{
		FString ItemKey = std::format("{}.{}", Key, i);
		if (IsEmpty(Section, ItemKey)) { return FVector2{}; }

		Value[i] = GetFloat(Section, ItemKey);
	}

	return Value;
}

void FConfigArchive::SetVector2(const FString& Section, const FString& Key, const FVector2& Value)
{
	for (int i = 0; i < 2; ++i)
	{
		FString ItemKey = std::format("{}.{}", Key, i);
		Object[Section][ItemKey] = std::to_string(Value[i]);
	}
}
