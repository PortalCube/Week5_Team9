#pragma once

#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/Core/Log.h"
#include "RunTime/Core/TArray.h"
#include <Windows.h>

struct FPoolChunk
{
    void* Memory = nullptr;
};

class FPoolAllocator
{
private:
    struct FFreeBlock
    {
        FFreeBlock* Next = nullptr;
    };

public:
    [[nodiscard]]
    bool Init(size_t InBlockSize, size_t InBlockCount);
    void* Allocate();
    void Free(void* Ptr);
    void Shutdown();
    bool Owns(void* Ptr) const;
    bool AddChunk();

    size_t GetFreeBlockCount() const { return FreeBlockCount;} 
    size_t GetUsedBlockCount() const{ return BlockCount - FreeBlockCount;}
    size_t GetBlockCount() const { return BlockCount; }

private:
    const size_t BlockCountPerChunk = 1024;
    // void* Memory = nullptr;
    TArray<FPoolChunk> Chunks;


    size_t BlockSize = 0;
    size_t BlockCount = 0;
    size_t FreeBlockCount = 0;
    FFreeBlock* FreeList = nullptr;

    size_t TotalSize = 0;
    size_t CommittedSize = 0;

};