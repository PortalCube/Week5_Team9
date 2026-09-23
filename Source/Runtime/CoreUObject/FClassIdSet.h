#pragma once
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/IntTypes.h"

struct FClassIdSet
{
	TArray<uint64> data;

	void Set(size_t idx)
	{
		size_t chunck = idx / 64; //해당 typeID에 대한 비트가 어느 청크에있는지
							      //0번 청크엔 ID 0~63에 대해 판단가능하고 1번 청크엔 ID 64~127까지 판단가능
		if(chunck >= data.size())
		{
			Resize(chunck + 1);
		}
	
		data[chunck] |= 1ULL << (idx % 64);
	}

	void Reset(size_t idx)
	{
		size_t chunk = idx / 64;
		if (chunk >= data.size())
		{
			Resize(chunk + 1);
		}

		data[chunk] &= ~(1ULL << (idx % 64));
	}

	bool Test(size_t idx) const
	{
		size_t chunk = idx / 64;
		if (chunk >= data.size()) return false;
		return (data[chunk] & (1ULL << (idx % 64))) != 0;
	}

	void Resize(size_t size)
	{
		if (data.size() < size)
		{
			data.resize(size, 0); // push_back 대신 resize 사용
		}
	}

	FClassIdSet& operator|=(const FClassIdSet& other)
	{
		if (other.data.size() > data.size())
		{
			Resize(other.data.size());
		}
		for (size_t i = 0; i < data.size(); ++i)
			data[i] |= other.data[i];
		return *this;
	}
};