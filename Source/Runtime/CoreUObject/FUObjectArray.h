#pragma once

#include "UObject.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/IntTypes.h"
#include <utility>

class UObject;

class FUObjectArray final
{
public:
	static FUObjectArray& Get() {
		static FUObjectArray Instance;
		return Instance;
	}

	void SetNextUUID(uint32 UUID);
	[[nodiscard]] uint32 GetNextUUID() const { return NextUUID; }
	[[nodiscard]] uint32 GetNumObjects() const { return static_cast<uint32>(Objects.size()); }
	[[nodiscard]] UObject* GetObjectByIndex(uint32 Index) const { return Objects[Index]; }
	[[nodiscard]] bool IsValid(const UObject* Object, uint32 UUID) const;

	FUObjectArray(const FUObjectArray&) = delete;
	FUObjectArray& operator=(const FUObjectArray&) = delete;

	FUObjectArray(FUObjectArray&&) = delete;
	FUObjectArray& operator=(FUObjectArray&&) = delete;

	class TIterator
	{
	public:
		explicit TIterator( uint32 InIndex) : Index(InIndex) {}
		TIterator& operator++() { ++Index; return *this;}
		TIterator operator++(int) { TIterator Temp = *this; ++Index; return Temp; }
		bool operator==(const TIterator& Other) const { return Index == Other.Index; }
		bool operator!=(const TIterator& Other) const { return Index != Other.Index; }
		UObject* operator*() const { return FUObjectArray::Get().GetObjectByIndex(Index); }
		UObject* operator->() const { return FUObjectArray::Get().GetObjectByIndex(Index); }
	private:
		uint32 Index;
	};

	TIterator begin() { return TIterator(0); }
	TIterator end() { return TIterator( GetNumObjects());} 

private:
	FUObjectArray() = default;
	~FUObjectArray() = default;

	void AddObject(UObject* Object);
	void RemoveObject(UObject* Object);

	[[nodiscard]] uint32 AcquireUUID() { return NextUUID++; }

	TArray<UObject*> Objects;
	uint32 NextUUID = 1u;

	template <typename TObject, typename ... TArgs>
		requires std::derived_from<TObject, UObject>
	friend TObject* NewObject(TArgs&&... Args);
	friend UObject* NewObject(UClass* ClassType);
	friend void DestroyObject(UObject* Object);

	void DestroyObject(UObject* Object);
};
