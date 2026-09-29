#pragma once

#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include <dxgi1_4.h>
#include <Runtime/Core/FName.h>

#define PP_CAT_INNER(A, B) A##B
#define PP_CAT(A, B)       PP_CAT_INNER(A, B)

class FStatsManager final
{
public:

    enum class EStatType : uint8
    {
        Cycle,      // 시간(ms). 매 프레임 리셋
        Counter,    // 개수. 매 프레임 리셋
        Memory,     // 바이트. 리셋하지 않고 계속 누적
    };

    struct FStatEntry
    {
        EStatType Type = EStatType::Counter;
        bool bEnabled = false;
        double Accum = 0.0;
        int32 Calls = 0;
        double Avg = 0.0, Max = 0.0;
        double Display = 0.0;
        int32 DisplayCalls = 0;
    };

    static FStatsManager& Get()
    {
        static FStatsManager Instance;
        return Instance;
    }

    FStatsManager(const FStatsManager&) = delete;
    FStatsManager& operator=(const FStatsManager&) = delete;

public:
    void Initialize(ID3D11Device* Device);

    size_t GetProcessMemoryUsed() const;
    size_t GetSystemMemoryUsed() const;
    size_t GetSystemMemoryAvailable() const;

    size_t GetGPUMemoryUsed() const;
    size_t GetGPUMemoryBudget() const;

    // 스탯을 등록하고 인덱스를 돌려준다. 이미 있으면 기존 인덱스를 준다.
    // 원소를 제거하지 않으므로 한 번 받은 인덱스는 영구히 유효하다.
    int32 Register(const FName& Name, EStatType Type);
    inline FStatEntry& GetEntry(int32 Index) { return Entries[Index]; }

    inline void Accumulate(int32 Index, double Value)
    {
        FStatEntry& Entry = Entries[Index];
        Entry.Accum += Value;
        Entry.Calls++;
    }

    // 직전 프레임의 완성된 값. 없으면 0.
    // 이름으로 찾는 느린 경로라 표시용으로만 쓴다.
    double GetDisplay(const FName& Name) const
    {
        auto It = NameToIndex.find(Name);
        return (It == NameToIndex.end()) ? 0.0 : Entries[It->second].Display;
    }

    void SetUnitStatsEnabled(bool bEnable);
    void ResetFrame();

private:
    FStatsManager() { Entries.reserve(64); }

    Microsoft::WRL::ComPtr<IDXGIAdapter3> Adapter;

    TArray<FStatEntry> Entries;
    TMap<FName, int32> NameToIndex;
};

// 매크로마다 하나씩 생기는 static. 최초 실행 때 한 번만 등록하고
// 그때 받은 인덱스를 계속 재사용한다.
struct FStatId
{
    FStatId(const char* InName, FStatsManager::EStatType InType)
        : Name(InName), Index(FStatsManager::Get().Register(Name, InType))
    {
    }

    FName Name;     // 디버깅용
    int32 Index;
};

inline double GetMsPerCount()
{
    static const double MsPerCount = []
        {
            LARGE_INTEGER Freq;
            QueryPerformanceFrequency(&Freq);
            return 1000.0 / static_cast<double>(Freq.QuadPart);
        }();
    return MsPerCount;
}

struct FScopeCycleCounter
{
    static inline FScopeCycleCounter* Current = nullptr;
    FScopeCycleCounter* Parent = nullptr;
    double ChildMs = 0.0;   // 내 직속 자식들이 먹은 시간
    bool bIsIndependent = false;

    // 스탯에 기록하지 않고 시간만 잰다. Finish()의 반환값으로 결과를 받는다.
    FScopeCycleCounter()
        : bIsIndependent(true)
        , StatIndex(INDEX_NONE_STAT)
        , bActive(true)
    {
        QueryPerformanceCounter(&Start);
    }

    FScopeCycleCounter(int32 InStatIndex, bool IsIndependent = false)
        : bIsIndependent(IsIndependent)
        , StatIndex(InStatIndex)
    {
        bActive = FStatsManager::Get().GetEntry(StatIndex).bEnabled;
        if (!bActive) return;
        if (!bIsIndependent)
        {
            Parent = Current;
            Current = this;
        }
        QueryPerformanceCounter(&Start);
    }

    ~FScopeCycleCounter()
    {
        Finish();
    }

    // 경과 시간(ms)을 돌려준다. 여러 번 불러도 기록은 한 번만 하고 같은 값을 준다.
    double Finish()
    {
        if (!bActive) return ElapsedMs;
        bActive = false;

        LARGE_INTEGER End; QueryPerformanceCounter(&End);
        const double Ms = (End.QuadPart - Start.QuadPart) * GetMsPerCount();
        ElapsedMs = Ms;

        if (StatIndex == INDEX_NONE_STAT)
        {
            return Ms;
        }

        if (!bIsIndependent)
        {
            Current = Parent;                                          // 원래대로 복구
            FStatsManager::Get().Accumulate(StatIndex, Ms - ChildMs);   // Exclusive
            if (Parent) Parent->ChildMs += Ms;                         // 부모에게 "나 이만큼 썼어" 보고
        }
        else
        {
            // Independent
            FStatsManager::Get().Accumulate(StatIndex, Ms);
        }
        return Ms;
    }

    static constexpr int32 INDEX_NONE_STAT = -1;

    int32 StatIndex; LARGE_INTEGER Start; bool bActive;
    double ElapsedMs = 0.0;
};


// Cycle
// 지역 변수가 스코프 끝까지 살아남아야 하므로 do-while로 감싸지 않는다.
#define SCOPE_CYCLE_COUNTER_IMPL(Tag, StatName, IsIndependent)                                     \
    static const FStatId PP_CAT(_StatId_, Tag)(StatName, FStatsManager::EStatType::Cycle);         \
    FScopeCycleCounter PP_CAT(_StatScope_, Tag)(PP_CAT(_StatId_, Tag).Index, IsIndependent)

//스코프를 사용하여 사용 시간 기록
#define SCOPE_CYCLE_COUNTER(StatName) SCOPE_CYCLE_COUNTER_IMPL(__COUNTER__, StatName, false)

//이미 계산된 ms를 대입해야 하는 경우 사용
#define SET_CYCLE_COUNTER_IMPL(Tag, StatName, Ms)                                                  \
    do {                                                                                           \
        static const FStatId PP_CAT(_StatId_, Tag)(StatName, FStatsManager::EStatType::Cycle);     \
        FStatsManager::FStatEntry& PP_CAT(_StatEntry_, Tag) =                                      \
            FStatsManager::Get().GetEntry(PP_CAT(_StatId_, Tag).Index);                            \
        if (PP_CAT(_StatEntry_, Tag).bEnabled)                                                     \
        {                                                                                          \
            PP_CAT(_StatEntry_, Tag).Accum += (Ms);                                                \
            PP_CAT(_StatEntry_, Tag).Calls++;                                                      \
        }                                                                                          \
    } while (0)

//계산된 사용시간을 기록
#define SET_CYCLE_COUNTER(StatName, Ms) SET_CYCLE_COUNTER_IMPL(__COUNTER__, StatName, Ms)

// Counter
// do-while(0)으로 감싸 전체를 문장 하나로 만든다. if/else 안에서도 안전.
#define INC_DWORD_STAT_BY_IMPL(Tag, StatName, N)                                                   \
    do {                                                                                           \
        static const FStatId PP_CAT(_StatId_, Tag)(StatName, FStatsManager::EStatType::Counter);   \
        FStatsManager::FStatEntry& PP_CAT(_StatEntry_, Tag) =                                      \
            FStatsManager::Get().GetEntry(PP_CAT(_StatId_, Tag).Index);                            \
        if (PP_CAT(_StatEntry_, Tag).bEnabled)                                                     \
        {                                                                                          \
            PP_CAT(_StatEntry_, Tag).Accum += static_cast<double>(N);                              \
            PP_CAT(_StatEntry_, Tag).Calls++;                                                      \
        }                                                                                          \
    } while (0)

//StatName의 스텟에 카운트 증가
#define INC_DWORD_STAT(StatName)       INC_DWORD_STAT_BY_IMPL(__COUNTER__, StatName, 1)

//StatName의 스텟에 카운트를 N만큼 증가
#define INC_DWORD_STAT_BY(StatName, N) INC_DWORD_STAT_BY_IMPL(__COUNTER__, StatName, N)


// Memory
// 메모리는 "현재 총량"이라 꺼져 있어도 계속 세야 한다. bEnabled 가드를 걸지 않는다.
// 한 번이라도 거르면 누락된 할당/해제만큼 총량이 영구히 틀어진다.
#define MEMORY_STAT_BY_IMPL(Tag, StatName, SignedSize)                                             \
    do {                                                                                           \
        static const FStatId PP_CAT(_StatId_, Tag)(StatName, FStatsManager::EStatType::Memory);    \
        FStatsManager::Get().Accumulate(PP_CAT(_StatId_, Tag).Index, (SignedSize));                \
    } while (0)

//StatName의 스텟에 사이즈 증가
#define INC_MEMORY_STAT_BY(StatName, Size) \
    MEMORY_STAT_BY_IMPL(__COUNTER__, StatName,  static_cast<double>(Size))

//StatName의 스텟에 사이즈 감소
#define DEC_MEMORY_STAT_BY(StatName, Size) \
    MEMORY_STAT_BY_IMPL(__COUNTER__, StatName, -static_cast<double>(Size))


struct FInputLatencyTimer
{
    static FInputLatencyTimer& Get()
    {
        static FInputLatencyTimer Instance;
        return Instance;
    }

    FInputLatencyTimer(const FInputLatencyTimer&) = delete;
    FInputLatencyTimer& operator=(const FInputLatencyTimer&) = delete;

    // 한 프레임에 입력이 여러 번 와도 첫 입력을 기준으로 삼는다.
    void Trigger()
    {
        if (PendingStart != 0) return;

        LARGE_INTEGER Now;
        QueryPerformanceCounter(&Now);
        PendingStart = Now.QuadPart;
    }

    // 이번 프레임이 반영한 입력의 시각. 입력이 없던 프레임은 0.
    int64 ConsumePendingStart()
    {
        const int64 Start = PendingStart;
        PendingStart = 0;
        return Start;
    }

    // 프레임의 GPU 작업이 끝났을 때, 그 프레임이 물고 있던 입력 시각으로 호출.
    void Report(int64 Start)
    {
        if (Start == 0) return;

        LARGE_INTEGER Now;
        QueryPerformanceCounter(&Now);
        DeltaMs = (Now.QuadPart - Start) * GetMsPerCount();
    }

    void Tick()
    {
        SET_CYCLE_COUNTER("Input", DeltaMs);
    }

private:
    FInputLatencyTimer() = default;

    int64 PendingStart = 0;
    double DeltaMs = 0.0;
};
