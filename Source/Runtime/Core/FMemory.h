#pragma once

#include "Runtime/Core/FAllocator.h"



struct FMemory
{
	enum AllocationHints
	{
		None = -1,
		Default,
		Temporary,
		SmallPool,

		Max
	};

public:
	static bool Init()
	{
		return Allocator.Init();
	}

	static void Shutdown()
	{
		Allocator.Shutdown();
	}

	static void* Malloc(size_t Size)
	{
		return Allocator.Allocate(Size);
	}

	static void Free(void* Ptr)
	{
		Allocator.Free(Ptr);
	}

	static void* Malloc(size_t Size, size_t Alignment)
	{
		return Allocator.Allocate(Size, Alignment);
	}

	static void Free(void* Ptr, size_t Alignment)
	{
		Allocator.Free(Ptr, Alignment);
	}

private:
	static inline FAllocator Allocator;

};