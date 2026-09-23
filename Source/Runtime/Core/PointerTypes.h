#pragma once

#include <memory>
#include <concepts>
#include <utility>
#include "Runtime/Core/FMemory.h"

#if (false)

template <typename T>
 using TUniquePtr = std::unique_ptr<T>;

template <typename T, typename ...Args>
TUniquePtr<T> MakeUnique(Args&&... InArgs)
{
	return std::make_unique<T>(std::forward<Args>(InArgs)...);
}

template <typename T>
using TSharedPtr = std::shared_ptr<T>;

template <typename T, typename ...Args>
TSharedPtr<T> MakeShared(Args&&... InArgs)
{
	return std::make_shared<T>(std::forward<Args>(InArgs)...);
}

template <typename T>
using TWeakPtr = std::weak_ptr<T>;

#else

template<typename T>
struct TSharedControlBlock
{
    T* Ptr = nullptr;
    size_t StrongCount = 1;
    size_t WeakCount = 0;
};

template<typename T>
class TUniquePtr
{
public:
    TUniquePtr() = default;
    explicit TUniquePtr(T* InPtr) : Ptr(InPtr) {}
    ~TUniquePtr() { Reset();}

    // 복사 금지
    TUniquePtr(const TUniquePtr&) = delete;
    TUniquePtr& operator=(const TUniquePtr&) = delete;

    // 이동 허용
    TUniquePtr(TUniquePtr&& Other) noexcept : Ptr(Other.Ptr) { Other.Ptr = nullptr; }

    template<typename U>
        requires std::derived_from<U, T>
    TUniquePtr(TUniquePtr<U>&& Other) noexcept : Ptr(Other.Release()) {}

    TUniquePtr& operator=(TUniquePtr&& Other) noexcept
    {
        if (this != &Other)
        {
            Reset();

            Ptr = Other.Ptr;
            Other.Ptr = nullptr;
        }

        return *this;
    }

public:
    T* get() const { return Ptr; }
    T& operator*() const { return *Ptr; }
    T* operator->() const { return Ptr; }
    explicit operator bool() const { return Ptr != nullptr; }

    void Reset(T* InPtr = nullptr)
    {
        if (Ptr)
        {
            Ptr->~T();
            FMemory::Free(Ptr);
        }

        Ptr = InPtr;
    }

    T* Release()
    {
        T* Result = Ptr;
        Ptr = nullptr;
        return Result;
    }

private:
    T* Ptr = nullptr;
};

template<typename T, typename... Args>
TUniquePtr<T> MakeUnique(Args&&... InArgs)
{
    void* Memory = FMemory::Malloc(sizeof(T), alignof(T));

    if (!Memory)
    {
        return TUniquePtr<T>();
    }

    T* Object = new (Memory) T( std::forward<Args>(InArgs)... );

    return TUniquePtr<T>(Object);
}

template<typename T>
class TSharedPtr
{
    template<typename U>
    friend class TSharedPtr;

    template<typename U>
    friend class TWeakPtr;

public:
    TSharedPtr() = default;
    explicit TSharedPtr(T* InPtr)
    {
        if (!InPtr) { return; }
        Ptr = InPtr;
        void* Memory = FMemory::Malloc( sizeof(TSharedControlBlock<T>));
        
        if (!Memory)
        {
            InPtr->~T();
            FMemory::Free(InPtr);
            return;
        }
        
        ControlBlock = new (Memory)TSharedControlBlock<T>();
        ControlBlock->Ptr = InPtr;
    }

    TSharedPtr(T* InPtr, TSharedControlBlock<T>* InControlBlock) : Ptr(InPtr), ControlBlock(InControlBlock) {}

    // Copy
    TSharedPtr(const TSharedPtr& Other) : Ptr(Other.Ptr), ControlBlock(Other.ControlBlock)
    {
        if (ControlBlock) { ++ControlBlock->StrongCount;}
    }

    TSharedPtr(std::nullptr_t) : Ptr(nullptr), ControlBlock(nullptr) {}

    template<typename U>
        requires std::derived_from<U, T>
    TSharedPtr(TSharedPtr<U>&& Other) noexcept : Ptr(Other.Release()), ControlBlock(Other.ControlBlock) 
    {
        Other.Ptr = nullptr;
        Other.ControlBlock = nullptr;
    }

    ~TSharedPtr() { Release(); }

    TSharedPtr& operator=(const TSharedPtr& Other)
    {
        if (this != &Other)
        {
            Release();

            Ptr = Other.Ptr;
            ControlBlock = Other.ControlBlock;

            if (ControlBlock)
            {
                ++ControlBlock->StrongCount;
            }
        }

        return *this;
    }

    // Move
    TSharedPtr(TSharedPtr&& Other) noexcept : Ptr(Other.Ptr), ControlBlock(Other.ControlBlock)
    {
        Other.Ptr = nullptr;
        Other.ControlBlock = nullptr;
    }

    TSharedPtr& operator=(TSharedPtr&& Other) noexcept
    {
        if (this != &Other)
        {
            Release();

            Ptr = Other.Ptr;
            ControlBlock = Other.ControlBlock;

            Other.Ptr = nullptr;
            Other.ControlBlock = nullptr;
        }

        return *this;
    }

    bool operator==(std::nullptr_t) const { return Ptr == nullptr; }
    bool operator!=(std::nullptr_t) const { return Ptr != nullptr; }

public:
    T* get() const { return Ptr; }
    T& operator*() const { return *Ptr; }
    T* operator->() const { return Ptr; }
    explicit operator bool() const { return Ptr != nullptr; }

    size_t UseCount() const
    {
        return ControlBlock ? ControlBlock->StrongCount : 0;
    }

    void Reset()
    {
        Release();
    }

private:
    void Release()
    {
        if (!ControlBlock)
        {
            return;
        }

        --ControlBlock->StrongCount;

        if (ControlBlock->StrongCount == 0)
        {
            Ptr->~T();
            FMemory::Free(Ptr);

            ControlBlock->Ptr = nullptr;
        }

        if (ControlBlock->StrongCount == 0 && ControlBlock->WeakCount == 0)
        {
            ControlBlock->~TSharedControlBlock<T>();
            FMemory::Free(ControlBlock);
        }

        Ptr = nullptr;
        ControlBlock = nullptr;
    }

private:
    T* Ptr = nullptr;
    TSharedControlBlock<T>* ControlBlock = nullptr;
};

template<typename T, typename... Args>
TSharedPtr<T> MakeShared(Args&&... InArgs)
{
    void* Memory =
        FMemory::Malloc(sizeof(T), alignof(T));

    if (!Memory)
    {
        return TSharedPtr<T>();
    }

    T* Object =
        new (Memory) T(
            std::forward<Args>(InArgs)...
        );

    return TSharedPtr<T>(Object);
}

template<typename T>
class TWeakPtr
{
public:
    TWeakPtr() = default;

    // Copy
    TWeakPtr(const TSharedPtr<T>& Other) : Ptr(Other.Ptr), ControlBlock(Other.ControlBlock)
    {
        if (ControlBlock) { ++ControlBlock->WeakCount; }
    }

    TWeakPtr(std::nullptr_t) : Ptr(nullptr), ControlBlock(nullptr) {}

    template<typename U>
        requires std::derived_from<U, T>
    TWeakPtr(TWeakPtr<U>&& Other) noexcept : Ptr(Other.Release()) {}

    ~TWeakPtr() { Release(); }

    TWeakPtr& operator=(const TWeakPtr& Other)
    {
        if (this != &Other)
        {
            Release();

            Ptr = Other.Ptr;
            ControlBlock = Other.ControlBlock;

            if (ControlBlock)
            {
                ++ControlBlock->WeakCount;
            }
        }

        return *this;
    }

    // Move
    TWeakPtr(TWeakPtr&& Other) noexcept : Ptr(Other.Ptr), ControlBlock(Other.ControlBlock)
    {
        Other.Ptr = nullptr;
        Other.ControlBlock = nullptr;
    }

    TWeakPtr& operator=(TWeakPtr&& Other) noexcept
    {
        if (this != &Other)
        {
            Release();

            Ptr = Other.Ptr;
            ControlBlock = Other.ControlBlock;

            Other.Ptr = nullptr;
            Other.ControlBlock = nullptr;
        }

        return *this;
    }

    bool operator==(std::nullptr_t) const { return Ptr == nullptr; }
    bool operator!=(std::nullptr_t) const { return Ptr != nullptr; }

    TSharedPtr<T> Lock() const
    {
        if (!ControlBlock || ControlBlock->StrongCount == 0)
        {
            return TSharedPtr<T>();
        }

        ++ControlBlock->StrongCount;

        return TSharedPtr<T>(Ptr, ControlBlock);
    }

public:
    T* get() const { return Ptr; }
    T& operator*() const { return *Ptr; }
    T* operator->() const { return Ptr; }
    explicit operator bool() const { return Ptr != nullptr; }

    size_t UseCount() const
    {
        return ControlBlock ? ControlBlock->StrongCount : 0;
    }

    void Reset()
    {
        Release();
    }

private:
    void Release()
    {
        if (!ControlBlock)
        {
            return;
        }

        --ControlBlock->WeakCount;

        if (ControlBlock->StrongCount == 0 && ControlBlock->WeekCount == 0)
        {
            Ptr->~T();
            FMemory::Free(Ptr);
            ControlBlock->~TSharedControlBlock<T>();
            FMemory::Free(ControlBlock);
        }

        Ptr = nullptr;
        ControlBlock = nullptr;
    }

private:
    T* Ptr = nullptr;
    TSharedControlBlock<T>* ControlBlock = nullptr;
};

#endif