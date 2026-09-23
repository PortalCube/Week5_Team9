#include "FUObjectArray.h"

#include <algorithm>
#include <cassert>

void FUObjectArray::SetNextUUID(uint32 UUID)
{
	NextUUID = UUID;
}

void FUObjectArray::AddObject(UObject* Object)
{
	Object->InternalIndex = static_cast<uint32>(Objects.size());
	Object->UUID = AcquireUUID();
	Objects.push_back(Object);
}

void FUObjectArray::RemoveObject(UObject* Object)
{
	const uint32 Index = Object->InternalIndex;
	assert(Index < Objects.size() && Objects[Index] == Object);

	UObject* LastObject = Objects.back();

	Objects[Index] = LastObject;
	LastObject->InternalIndex = Index;

	Objects.pop_back();
}

void FUObjectArray::DestroyObject(UObject* Object) {
	if (Object == nullptr) return;

	Object->Release();
	RemoveObject(Object);
	delete Object; // 오버라이드해서 통계 구현 필요
}

bool FUObjectArray::IsValid(const UObject* Object, uint32 UUID) const
{
	if (Object == nullptr || UUID == 0) return false;

	const auto It = std::find(Objects.begin(), Objects.end(), Object);
	return It != Objects.end() && (*It)->UUID == UUID;
}
