#pragma once

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FVector2.h"
#include "ThirdParty/mIni/ini.h"

// 임시...
#undef GetObject

/// <summary>
/// FEditorState의 데이터를 직렬화/역직렬화 하는 클래스입니다.
/// FEditorState의 데이터를 이 클래스에 담을 수도 있고, 이 데이터로 FEditorState를 만들 수도 있습니다.
/// </summary>
class FConfigArchive
{
	// FArchive와 합쳐서 공통 인터페이스로 만들고 싶지만
	// JSON과 FArchive는 Object가 여러번 중첩된 구조를 가질 수 있지만
	// ini는 그게 불가능하니.. 명세가 완전 별개인 두개 오브젝트를 합치기 힘드므로 별도로 분리

private:
	mINI::INIStructure Object;

public:
	FConfigArchive();
	explicit FConfigArchive(const mINI::INIStructure& InObject);

	mINI::INIStructure GetConfig() const { return Object; }

	int32 GetInt32(const FString& Section, const FString& Key) const;
	void SetInt32(const FString& Section, const FString& Key, int32 Value);

	float GetFloat(const FString& Section, const FString& Key) const;
	void SetFloat(const FString& Section, const FString& Key, float Value);

	uint32 GetUInt32(const FString& Section, const FString& Key) const;
	void SetUInt32(const FString& Section, const FString& Key, uint32 Value);

	double GetDouble(const FString& Section, const FString& Key) const;
	void SetDouble(const FString& Section, const FString& Key, double Value);

	bool GetBool(const FString& Section, const FString& Key) const;
	void SetBool(const FString& Section, const FString& Key, bool Value);

	FString GetString(const FString& Section, const FString& Key) const;
	void SetString(const FString& Section, const FString& Key, const FString& Value);

	bool IsEmpty(const FString& Section, const FString& Key) const;

	FVector GetVector(const FString& Section, const FString& Key) const;
	void SetVector(const FString& Section, const FString& Key, const FVector& Value);

	FVector2 GetVector2(const FString& Section, const FString& Key) const;
	void SetVector2(const FString& Section, const FString& Key, const FVector2& Value);
};
