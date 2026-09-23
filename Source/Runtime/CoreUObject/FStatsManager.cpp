#include "FStatsManager.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Core/Log.h"
#include <d3d11.h>
#include <windows.h>
#include <psapi.h>
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

    for (size_t i = 0; i < static_cast<size_t>(EStatMemoryCategory::COUNT); ++i)
    {
        MemoryStats[static_cast<EStatMemoryCategory>(i)] = 0;
    }

    DxgiAdapter.As(&Adapter);
}

//  SYSTEM MEMORY
//  Total RAM       32.0 GB
//  Used RAM        13.6 GB
//  Available RAM   18.4 GB
//

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


    /*DXGI_QUERY_VIDEO_MEMORY_INFO Info{};

    if (FAILED(Adapter->QueryVideoMemoryInfo(
        0,
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
        &Info)))
    {
        return 0;
    }

    return static_cast<size_t>(Info.CurrentUsage);*/

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

    //UE_LOG("GPU Local: %.2f MB", LocalInfo.CurrentUsage / (1024.0 * 1024.0));
    //UE_LOG("GPU NonLocal: %.2f MB", NonLocalInfo.CurrentUsage / (1024.0 * 1024.0));

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

size_t FStatsManager::GetVertexShaderMemoryUsed() const
{
    return MemoryStats.at(EStatMemoryCategory::VertexShader);
}

size_t FStatsManager::GetPixelShaderMemoryUsed() const
{
    return MemoryStats.at(EStatMemoryCategory::PixelShader);
}

size_t FStatsManager::GetTextureMemoryUsed() const
{
    return MemoryStats.at(EStatMemoryCategory::Texture);
}

size_t FStatsManager::GetStaticMeshMemoryUsed() const
{
    return MemoryStats.at(EStatMemoryCategory::StaticMesh);
}

size_t FStatsManager::GetMemoryPool() const
{
    return MemoryStats.at(EStatMemoryCategory::MemoryPool);
}

size_t FStatsManager::GetMemoryPoolUsed() const
{
    return MemoryStats.at(EStatMemoryCategory::MemoryPoolUsed);
}

size_t FStatsManager::GetMemoryPoolFree() const
{
    return MemoryStats.at(EStatMemoryCategory::MemoryPoolFree);
}
