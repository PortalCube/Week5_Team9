#include "UClass.h"
#include "Runtime/Core/PointerTypes.h"

UClass* UClass::RegisterToFactory(const FString& typeName, const TFunction<UObject* ()>& createFunction, const FString& superClassTypeName)
{

    TUniquePtr<UClass> classType = MakeUnique<UClass>();
    classType->className = typeName;
    classType->superClassTypeName = superClassTypeName;
    classType->createFunction = createFunction;
    classType->typeId = registeredCount++;

    nameToId[typeName] = classType->typeId;

    UClass* rawPtr = classType.get();


    classList.push_back(std::move(classType));
    return rawPtr;
}
UClass* UClass::FindByName(const FString& Name)
{
    auto it = nameToId.find(Name);
    return (it != nameToId.end()) ? classList[it->second].get() : nullptr;
}

const FString& UClass::GetDisplayName() const
{
    auto itr = metadata.find("DisplayName");
    if (itr != metadata.end())
    {
        return itr->second;
    }

    return className;
}

void UClass::SetMeta(const FString& key, const FString& value)
{
    metadata[key] = value;

    if (key == "DisplayName")
    {
        displayNameToId[value] = typeId;  // typeId는 인스턴스 멤버
    }

}

UObject* UClass::CreateDefaultObject() const
{
    return createFunction ? createFunction() : nullptr;
}

bool UClass::IsChildOrSelfOf(UClass* baseClass) const {
    return baseClass && classIdSet.Test(baseClass->typeId);
}

void UClass::ResolveTypeBitsets()
{
    for (const TUniquePtr<UClass>& _class : classList)
    {
        if (!_class->superClassTypeName.empty()) {
            auto it = nameToId.find(_class->superClassTypeName);
            _class->superClass = (it != nameToId.end()) ? classList[it->second].get() : nullptr;
        }
    }
    for (const TUniquePtr<UClass>& _class : classList)
    {
        if (_class->processed) continue;

        _class->ResolveTypeBitset(_class.get());
    }
}

void UClass::ResolveTypeBitset(UClass* classPtr)
{
    TArray<UClass*> stack;
    stack.push_back(classPtr);

    while (!stack.empty())
    {
        UClass* cur = stack.back();

        // 부모가 아직 처리되지 않았다면 먼저 스택에 push
        while (cur->superClass && !cur->superClass->processed)
        {
            stack.push_back(cur->superClass);
            cur = stack.back();
        }

        // 현재 노드 처리
        //cur->classIdSet.Clear();
        if (cur->superClass) 
            cur->classIdSet =  cur->classIdSet |= cur->superClass->classIdSet; //부모 비트 | 자식 비트 = 
        cur->classIdSet.Set(cur->typeId);  // 자신의 비트 추가
        cur->processed = true;

        stack.pop_back();
    }
}