#include "Runtime/Core/FPoolAllocator.h"


bool FPoolAllocator::Init(size_t InBlockSize, size_t InBlockCount)
{
    if (InBlockSize == 0 || InBlockCount == 0)
    {
        return false;
    }

    if (InBlockSize < sizeof(FFreeBlock))
    {
        return false;
    }

    BlockSize = InBlockSize;
    BlockCount = InBlockCount;

    const size_t TotalSize = BlockSize * BlockCount;

    void* Memory = VirtualAlloc(
        nullptr,
        TotalSize,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE
    );

    if (!Memory)
    {
        BlockSize = 0;
        BlockCount = 0;
        return false;
    }

    // Free List 초기화
    char* Current = static_cast<char*>(Memory);

    for (size_t i = 0; i < BlockCount - 1; ++i)
    {
        FFreeBlock* Block = reinterpret_cast<FFreeBlock*>(Current);
        Block->Next = reinterpret_cast<FFreeBlock*>(Current + BlockSize);
        Current += BlockSize;
    }

    // 마지막 Block
    FFreeBlock* LastBlock = reinterpret_cast<FFreeBlock*>(Current);
    LastBlock->Next = nullptr;
    FreeList = static_cast<FFreeBlock*>(Memory);
    FreeBlockCount = BlockCount;

    Chunks.push_back({ Memory });

    FStatsManager::Get().AddMemory(
        EStatMemoryCategory::MemoryPool,
        BlockSize * BlockCount
    );

    FStatsManager::Get().AddMemory(
        EStatMemoryCategory::MemoryPoolUsed,
        0
    );

    FStatsManager::Get().AddMemory(
        EStatMemoryCategory::MemoryPoolFree,
        BlockSize * BlockCount
    );

    return true;
}

void* FPoolAllocator::Allocate()
{
    if (!FreeList)
    {
        if (!AddChunk())
        {
            return nullptr;
        }
    }

    UE_LOG("Memory Pool Alocate!");

    FFreeBlock* Block = FreeList;

    FreeList = FreeList->Next;

    --FreeBlockCount;

    FStatsManager::Get().AddMemory(
        EStatMemoryCategory::MemoryPoolUsed,
        BlockSize
    );

    FStatsManager::Get().RemoveMemory(
        EStatMemoryCategory::MemoryPoolFree,
        BlockSize
    );

    return Block;
}

void FPoolAllocator::Free(void* Ptr)
{
    if (!Ptr)
    {
        return;
    }

    FFreeBlock* Block = static_cast<FFreeBlock*>(Ptr);

    Block->Next = FreeList;

    FreeList = Block;

    ++FreeBlockCount;

    FStatsManager::Get().RemoveMemory(
        EStatMemoryCategory::MemoryPoolUsed,
        BlockSize
    );

    FStatsManager::Get().AddMemory(
        EStatMemoryCategory::MemoryPoolFree,
        BlockSize
    );
}

void FPoolAllocator::Shutdown()
{
    const size_t TotalMemory = BlockSize * BlockCountPerChunk * Chunks.size();

    FStatsManager::Get().RemoveMemory(
        EStatMemoryCategory::MemoryPool,
        TotalMemory
    );

    FStatsManager::Get().RemoveMemory(
        EStatMemoryCategory::MemoryPoolFree,
        BlockSize * FreeBlockCount
    );

    FStatsManager::Get().RemoveMemory(
        EStatMemoryCategory::MemoryPoolUsed,
        BlockSize * GetUsedBlockCount()
    );

    for (const FPoolChunk& Chunk : Chunks)
    {
        if (Chunk.Memory)
        {
            VirtualFree( Chunk.Memory, 0, MEM_RELEASE );
        }
    }

    Chunks.clear();

    FreeList = nullptr;

    BlockSize = 0;
    BlockCount = 0;
    FreeBlockCount = 0;
}

bool FPoolAllocator::Owns(void* Ptr) const
{
    if (!Ptr)
        return false;

    uintptr_t Address = reinterpret_cast<uintptr_t>(Ptr);

    for (const FPoolChunk& Chunk : Chunks)
    {
        uintptr_t Start = reinterpret_cast<uintptr_t>(Chunk.Memory);
        uintptr_t End = Start + BlockSize * BlockCountPerChunk;

        if (Address >= Start && Address < End)
        {
            return ((Address - Start) % BlockSize) == 0;
        }
    }

    return false;
}

bool FPoolAllocator::AddChunk()
{
    const size_t ChunkSize = BlockSize * BlockCountPerChunk;

    void* Memory = VirtualAlloc(
        nullptr,
        ChunkSize,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE
    );

    if (!Memory)
    {
        return false;
    }

    // 새 Chunk를 FreeList에 연결
    char* Current = static_cast<char*>(Memory);

    for (size_t i = 0; i < BlockCountPerChunk - 1; ++i)
    {
        FFreeBlock* Block = reinterpret_cast<FFreeBlock*>(Current);
        Block->Next = reinterpret_cast<FFreeBlock*>(Current + BlockSize);
        Current += BlockSize;
    }

    // 마지막 Block
    FFreeBlock* LastBlock = reinterpret_cast<FFreeBlock*>(Current);
    LastBlock->Next = FreeList;
    FreeList = static_cast<FFreeBlock*>(Memory);
    FreeBlockCount += BlockCountPerChunk;

    Chunks.push_back({ Memory });

    FStatsManager::Get().AddMemory(
        EStatMemoryCategory::MemoryPool,
        BlockSize * BlockCountPerChunk
    );

    FStatsManager::Get().AddMemory(
        EStatMemoryCategory::MemoryPoolFree,
        BlockSize * BlockCountPerChunk
    );

    return true;
}