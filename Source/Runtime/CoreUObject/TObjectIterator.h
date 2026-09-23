#pragma once

#include "Runtime/CoreUObject/FUObjectArray.h"

template<typename TObject>
	requires std::derived_from<TObject, UObject>
class TObjectIterator
{
public:
	TObjectIterator() : Iterator(FUObjectArray::Get().begin())
	{
		SkipInvalid();
	}

	TObjectIterator& operator++()
	{
		if (Iterator != FUObjectArray::Get().end()) 
		{
			++Iterator;
			SkipInvalid();
		}
		return *this;
	}

	TObjectIterator operator++(int)
	{
		TObjectIterator Temp = *this;
		++(*this);
		return Temp;
	}

	explicit operator bool() const 
	{
		return Iterator != FUObjectArray::Get().end();
	}

	TObject* operator*() const
	{
		if (Iterator == FUObjectArray::Get().end())
		{
			return nullptr;
		}

		return static_cast<TObject*>(*Iterator);
	}

	TObject* operator->() const 
	{
		return operator*();
	}

private:
	void SkipInvalid() 
	{
		while (Iterator != FUObjectArray::Get().end()) 
		{
			UObject* Object = *Iterator;
			if (Object && Object->IsA(TObject::StaticClass()))
			{
				return;
			}
			++Iterator;
		}
	}

	FUObjectArray::TIterator Iterator;
};

