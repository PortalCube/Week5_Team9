#pragma once

#include "Runtime/Core/FPoolAllocator.h"
#include "Runtime/Core/Log.h"
#include "Runtime/CoreUObject/FStatsManager.h"

#include <malloc.h>
#include <array>


class FAllocator
{
public:

    [[nodiscard]]
    bool Init()
    {
        for (int i = 0; i < Pools.size(); ++i)
        {
            if (!Pools[i].Init(SizeClasses[i], 1024))
            {
                Shutdown();
                return false;
            }
            ++PoolCount;
        }

        return true;
    }

    void* Allocate(size_t Size)
    {
        for (int i = 0; i < 5; ++i)
        {
            if (Size <= SizeClasses[i])
            {
                if (void* Ptr = Pools[i].Allocate())
                {
                    return Ptr;
                }
                return std::malloc(Size);
            }
        }
        UE_LOG("System Allocate!");
        return std::malloc(Size);
    }

    void Free(void* Ptr)
    {
        for (int i = 0; i < 5; ++i)
        {
            if (Pools[i].Owns(Ptr))
            {
                Pools[i].Free(Ptr);
                return;
            }
        }
        std::free(Ptr);
    }

    void* Allocate(size_t Size, size_t Alignment)
    {
        if (Size == 0) { return nullptr; }
        //return _aligned_malloc(Size, Alignment);
        return std::malloc(Size);
    }

    void Free(void* Ptr, size_t Alignment)
    {
        if (Ptr == 0) { return ; }
        //_aligned_free(Ptr);
        std::free(Ptr);
    }

    void Shutdown()
    {
        for (size_t i = 0; i < PoolCount; ++i)
        {
            FPoolAllocator& Pool = Pools[i];
            Pool.Shutdown();
        }
    }

private:
    UINT PoolCount = 0;
    inline static constexpr std::array<size_t, 5> SizeClasses = { 16, 32, 64, 128, 256 };
    std::array<FPoolAllocator, 5> Pools;

};