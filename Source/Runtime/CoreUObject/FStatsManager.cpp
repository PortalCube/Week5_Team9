#include "FStatsManager.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Core/Log.h"
#include <d3d11.h>
#include <windows.h>
#include <psapi.h>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "Psapi.lib")

void FStatsManager::Initialize(ID3D11Device* Device)
{
    if (!Device)
        return;

    Microsoft::WRL::ComPtr<IDXGIDevice> DxgiDevice;

    if (FAILED(Device->QueryInterface(
        IID_PPV_ARGS(&DxgiDevice))))
    {
        return;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter> DxgiAdapter;

    if (FAILED(DxgiDevice->GetAdapter(&DxgiAdapter)))
    {
        return;
    }
   
    // --- Cycle: 구간별 소요 시간(ms) ---
    Register(FName("Frame"), EStatType::Cycle);

    Register(FName("Game"), EStatType::Cycle);
    Register(FName("Draw"), EStatType::Cycle);
    Register(FName("GPU Time"), EStatType::Cycle);
    Register(FName("Input"), EStatType::Cycle);

    // --- Counter: 프레임당 개수 ---
    Register(FName("Draws"), EStatType::Counter);
    Register(FName("Prims"), EStatType::Counter);

    // --- Memory: 현재 총량(byte). 프레임마다 리셋되지 않는다 ---
    Register(FName("VertexShaderMemory"), EStatType::Memory);
    Register(FName("PixelShaderMemory"), EStatType::Memory);
    Register(FName("TextureMemory"), EStatType::Memory);
    Register(FName("StaticMeshMemory"), EStatType::Memory);
    Register(FName("MemoryPool"), EStatType::Memory);
    Register(FName("MemoryPoolUsed"), EStatType::Memory);
    Register(FName("MemoryPoolFree"), EStatType::Memory);

    DxgiAdapter.As(&Adapter);
}

size_t FStatsManager::GetProcessMemoryUsed() const
{
    PROCESS_MEMORY_COUNTERS_EX Counters{};

    if (!GetProcessMemoryInfo(
        GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&Counters),
        sizeof(Counters)))
    {
        return 0;
    }

    return static_cast<size_t>(Counters.WorkingSetSize);
}

size_t FStatsManager::GetSystemMemoryUsed() const
{
    MEMORYSTATUSEX MemoryStatus{};
    MemoryStatus.dwLength = sizeof(MemoryStatus);

    if (!GlobalMemoryStatusEx(&MemoryStatus))
    {
        return 0;
    }

    return static_cast<size_t>(
        MemoryStatus.ullTotalPhys -
        MemoryStatus.ullAvailPhys);
}

size_t FStatsManager::GetSystemMemoryAvailable() const
{
    MEMORYSTATUSEX MemoryStatus{};
    MemoryStatus.dwLength = sizeof(MemoryStatus);

    if (!GlobalMemoryStatusEx(&MemoryStatus))
    {
        return 0;
    }

    return static_cast<size_t>(MemoryStatus.ullAvailPhys);
}

size_t FStatsManager::GetGPUMemoryUsed() const
{
    if (!Adapter)
    {
        UE_LOG("GPU Memory: Adapter is null");
        return 0;
    }

    DXGI_QUERY_VIDEO_MEMORY_INFO LocalInfo{};
    DXGI_QUERY_VIDEO_MEMORY_INFO NonLocalInfo{};

    HRESULT LocalResult = Adapter->QueryVideoMemoryInfo(
        0,
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
        &LocalInfo
    );

    HRESULT NonLocalResult = Adapter->QueryVideoMemoryInfo(
        0,
        DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,
        &NonLocalInfo
    );

    if (FAILED(LocalResult) || FAILED(NonLocalResult))
    {
        return 0;
    }

    return static_cast<size_t>(LocalInfo.CurrentUsage + NonLocalInfo.CurrentUsage);
}

size_t FStatsManager::GetGPUMemoryBudget() const
{
    if (!Adapter)
        return 0;

    DXGI_QUERY_VIDEO_MEMORY_INFO Info{};

    if (FAILED(Adapter->QueryVideoMemoryInfo(
        0,
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
        &Info)))
    {
        return 0;
    }

    return static_cast<size_t>(Info.Budget);
}

int32 FStatsManager::Register(const FName& Name, EStatType Type)
{
    auto It = NameToIndex.find(Name);
    if (It != NameToIndex.end())
    {
        return It->second;
    }

    FStatEntry Entry;
    Entry.Type = Type;

    // 메모리는 총량이라 항상 수집한다. 나머지는 패널의 상태를 따른다.
    Entry.bEnabled = (Type == EStatType::Memory) || bUnitStatsEnabled;

    const int32 Index = static_cast<int32>(Entries.size());
    Entries.push_back(Entry);
    NameToIndex.insert({ Name, Index });

    return Index;
}

void FStatsManager::SetUnitStatsEnabled(bool bEnable)
{
    bUnitStatsEnabled = bEnable;

    for (FStatEntry& Entry : Entries)
    {
        // 메모리는 현재 총량이라 건드리지 않는다. 껐다 켜면 그 사이의
        // 할당/해제가 통째로 빠져 총량이 영구히 틀어진다.
        if (Entry.Type == EStatType::Memory)
        {
            continue;
        }

        Entry.bEnabled = bEnable;
    }
}

void FStatsManager::ResetFrame()
{
    for (FStatEntry& Entry : Entries)
    {
        // 메모리는 현재 총량이므로 평활하지 않고 리셋도 하지 않는다.
        if (Entry.Type == EStatType::Memory)
        {
            Entry.Max = std::max(Entry.Max, Entry.Accum);
            Entry.Display = Entry.Accum;
            Entry.DisplayCalls = Entry.Calls;
            continue;
        }

        // 꺼진 Cycle/Counter는 Accum이 계속 0이라, 그대로 두면 Avg만 매 프레임
        // 0.9배로 줄어 "0은 아닌 극소값"으로 남는다. 다시 켰을 때 그 값이 한 프레임
        // 동안 표시되면서 1000/FrameMs 같은 계산을 폭주시킨다. 아예 비워둔다.
        if (!Entry.bEnabled)
        {
            Entry.Avg = 0.0;
            Entry.Max = 0.0;
            Entry.Display = 0.0;
            Entry.DisplayCalls = 0;
            Entry.Accum = 0.0;
            Entry.Calls = 0;
            continue;
        }

        Entry.Avg = Entry.Avg * 0.9 + Entry.Accum * 0.1;
        Entry.Max = std::max(Entry.Max * 0.995, Entry.Accum);

        // 이번 프레임에 완성된 값을 표시용으로 넘긴다. HUD는 이걸 읽는다.
        // 평활은 시간에만. 개수는 정확한 값이라 평균을 내면 안 된다.
        Entry.Display = (Entry.Type == EStatType::Cycle) ? Entry.Avg : Entry.Accum;
        Entry.DisplayCalls = Entry.Calls;

        Entry.Accum = 0.0;
        Entry.Calls = 0;
    }
}
