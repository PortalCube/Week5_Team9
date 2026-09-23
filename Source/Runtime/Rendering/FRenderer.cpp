#include "FRenderer.h"
#include "FRenderResourceLibrary.h"

#include "FMaterial.h"
#include "FMesh.h"
#include "FRenderPipeline.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Rendering/FRenderQueue.h"
#include "Runtime/Rendering/FTexture.h"
#include "Runtime/Engine/FSceneView.h"
#include "ShaderConstants.h"
#include "ThirdParty/DirectXTK/Inc/DDSTextureLoader.h"
#include "ThirdParty/DirectXTK/Inc/WICTextureLoader.h"
#include "Vertices.h"
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>


bool FRenderer::Initialize(HWND Window) {
  if (!InitializeDeviceAndSwapChain(Window) ||
      !InitializeBackBufferAndDepthStencil() ||
      !InitializeEditorViewportRenderTarget() || !InitializeConstantBuffers()) {
    Shutdown();
    return false;
  }

  LineBatcher.Initialize(Device.Get()); // batch line

  return true;
}

void FRenderer::Shutdown() {
  if (Context) {
    Context->ClearState();
    Context->Flush();
  }

  LineBatcher.Shutdown();

  RasterizerStateMap.clear();
  DepthStencilStateMap.clear();
  BlendStateMap.clear();
  SamplerStateMap.clear();

  ObjectConstantBuffer.Reset();
  ViewConstantBuffer.Reset();

  BackBufferRTV.Reset();
  DepthStencilView.Reset();
  DepthStencilBuffer.Reset();

  SwapChain.Reset();
  Context.Reset();
  Device.Reset();
}

void FRenderer::BeginFrame() {
  Context->RSSetViewports(1, &Viewport);
  BindEditorViewportRenderTargets();

  constexpr float ClearColor[] = {0.5f, 0.5f, 0.5f, 1.0f};
  //constexpr float ClearColor[] = {0.05f, 0.05f, 0.08f, 1.0f};
  Context->ClearRenderTargetView(EditorViewPortRTV.Get(), ClearColor);
  Context->ClearDepthStencilView(
      DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void FRenderer::BindEditorViewportRenderTargets() {
  Context->OMSetRenderTargets(1, EditorViewPortRTV.GetAddressOf(),
                              DepthStencilView.Get());
}

void FRenderer::SetViewportUV(FVector2 TopLeftUV, FVector2 LengthUV) {
  // Viewport는 전체 백버퍼 크기를 유지하고, UV는 그리기 직전에 픽셀로 변환한다.
  D3D11_VIEWPORT RenderViewport = Viewport;
  RenderViewport.TopLeftX = TopLeftUV.X * Viewport.Width;
  RenderViewport.TopLeftY = TopLeftUV.Y * Viewport.Height;
  RenderViewport.Width = LengthUV.X * Viewport.Width;
  RenderViewport.Height = LengthUV.Y * Viewport.Height;
  Context->RSSetViewports(1, &RenderViewport);
};

// void FRenderer::Draw(const FMesh &Mesh, const FMaterial &Material,
//                      const FObjectConstants &ObjectConstants) {
//   UpdateObjectConstants(ObjectConstants);
//
//   TSharedPtr<FRenderPipeline> Pipeline = Material.Pipeline;
//   if (CurrentRenderMode == EViewModeIndex::VMI_Wireframe) {
//     Pipeline = GetPipeline(EBuiltinPipeline::Simple_Wireframe);
//   }
//
//   if (Pipeline) {
//     Pipeline->Bind(*Context.GetInstance());
//   }
//
//   Material.BindResources(*Context.GetInstance());
//   Mesh.BindResources(*Context.GetInstance());
//
//   Context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
//
//   if (Mesh.HasIndices()) {
//     Context->DrawIndexed(Mesh.IndexCount, 0, 0);
//   } else {
//     Context->Draw(Mesh.VertexCount, 0);
//   }
// }
//
// void FRenderer::DrawGrid(const FMesh &Mesh, const FMaterial &Material,
//                          const FGridConstants &GridConstants) {
//   UpdateGridConstants(GridConstants);
//   const auto &Pipeline = Material.Pipeline;
//
//   Pipeline->Bind(*Context.GetInstance());
//   Material.BindResources(*Context.GetInstance());
//   Mesh.BindResources(*Context.GetInstance());
//
//   Context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
//
//   if (Mesh.HasIndices()) {
//     Context->DrawIndexed(Mesh.IndexCount, 0, 0);
//   } else {
//     Context->Draw(Mesh.VertexCount, 0);
//   }
// }

void FRenderer::ClearDepth() {
  Context->ClearDepthStencilView(
      DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void FRenderer::SwapBuffer() { SwapChain->Present(0u, 0u); }

void FRenderer::OnWindowSize(UINT Width, UINT Height) {
  Context->OMSetRenderTargets(0, nullptr, nullptr);
  BackBufferRTV.Reset();
  DepthStencilView.Reset();
  DepthStencilSRV.Reset();
  DepthStencilBuffer.Reset();
  EditorViewPortRTV.Reset();
  EditorViewPortSRV.Reset();
  renderTexture.Reset();

  SwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, 0);
  Viewport.Width = static_cast<float>(Width);
  Viewport.Height = static_cast<float>(Height);

  InitializeBackBufferAndDepthStencil();
  InitializeEditorViewportRenderTarget();
}

TSharedPtr<FMesh> FRenderer::CreateMesh(const FMeshDesc &Desc) {
  if (!Desc.VertexData || Desc.VertexCount == 0 || Desc.VertexDataSize == 0 ||
      Desc.VertexStride == 0) {
    return nullptr;
  }
  if (Desc.IndexCount > 0 && (!Desc.IndexData || Desc.IndexDataSize == 0)) {
    return nullptr;
  }

  auto Mesh = TSharedPtr<FMesh>{new FMesh()};
  D3D11_BUFFER_DESC VertexBufferDesc = {
      .ByteWidth = Desc.VertexDataSize,
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_VERTEX_BUFFER,
  };

  D3D11_SUBRESOURCE_DATA VertexData = {
      .pSysMem = Desc.VertexData,
  };

  HRESULT Result =
      Device->CreateBuffer(&VertexBufferDesc, &VertexData, &Mesh->VertexBuffer);
  if (FAILED(Result)) {
    return nullptr;
  }
  Mesh->VertexBufferSize = Desc.VertexDataSize;
  Mesh->VertexCount = Desc.VertexCount;
  Mesh->VertexStride = Desc.VertexStride;

  if (Desc.IndexCount > 0 && Desc.IndexData) {
    D3D11_BUFFER_DESC IndexBufferDesc = {
        .ByteWidth = Desc.IndexDataSize,
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_INDEX_BUFFER,
    };

    D3D11_SUBRESOURCE_DATA IndexData = {
        .pSysMem = Desc.IndexData,
    };

    Result =
        Device->CreateBuffer(&IndexBufferDesc, &IndexData, &Mesh->IndexBuffer);
    if (FAILED(Result)) {
      return nullptr;
    }
  }
  Mesh->IndexBufferSize = Desc.IndexDataSize;
  Mesh->IndexCount = Desc.IndexCount;

  const auto *vertices = static_cast<const FVertexData *>(Desc.VertexData);

  Mesh->Positions.reserve(Desc.VertexCount);
  for (uint32 i = 0; i < Desc.VertexCount; ++i) {
    Mesh->Positions.push_back(
        FVector{vertices[i].x, vertices[i].y, vertices[i].z});
  }

  if (Desc.IndexCount > 0) {
    const auto *indices = static_cast<const uint32 *>(Desc.IndexData);
    Mesh->Indices.assign(indices, indices + Desc.IndexCount);
  }

  Mesh->Topology = Desc.bIsLine ? D3D11_PRIMITIVE_TOPOLOGY_LINELIST
                                : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

  Mesh->LocalBounds = FAxisAlignedBoundingBox{*Mesh.get()};

  Mesh->Sections = Desc.Sections;

  return Mesh;
}

TSharedPtr<FMesh> FRenderer::CreateDynamicMesh(const FMeshDesc &Desc) {
  if (!Desc.VertexData || Desc.VertexCount == 0 || Desc.VertexDataSize == 0 ||
      Desc.VertexStride == 0) {
    return nullptr;
  }
  if (Desc.IndexCount > 0 && (!Desc.IndexData || Desc.IndexDataSize == 0)) {
    return nullptr;
  }

  auto Mesh = TSharedPtr<FMesh>{new FMesh()};
  D3D11_BUFFER_DESC VertexBufferDesc = {
      .ByteWidth = Desc.VertexDataSize,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_VERTEX_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
  };

  D3D11_SUBRESOURCE_DATA VertexData = {
      .pSysMem = Desc.VertexData,
  };

  HRESULT Result =
      Device->CreateBuffer(&VertexBufferDesc, &VertexData, &Mesh->VertexBuffer);
  if (FAILED(Result)) {
    return nullptr;
  }

  Mesh->VertexCount = Desc.VertexCount;
  Mesh->VertexStride = Desc.VertexStride;
  Mesh->VertexBufferSize = Desc.VertexDataSize;

  if (Desc.IndexCount > 0 && Desc.IndexData) {
    D3D11_BUFFER_DESC IndexBufferDesc = {
        .ByteWidth = Desc.IndexDataSize,
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_INDEX_BUFFER,
    };

    D3D11_SUBRESOURCE_DATA IndexData = {
        .pSysMem = Desc.IndexData,
    };

    Result =
        Device->CreateBuffer(&IndexBufferDesc, &IndexData, &Mesh->IndexBuffer);
    if (FAILED(Result)) {
      return nullptr;
    }
  }
  Mesh->IndexCount = Desc.IndexCount;
  Mesh->IndexBufferSize = Desc.IndexDataSize;

  const auto *vertices = static_cast<const FVertexData *>(Desc.VertexData);

  Mesh->Positions.reserve(Desc.VertexCount);
  for (uint32 i = 0; i < Desc.VertexCount; ++i) {
    Mesh->Positions.push_back(
        FVector{vertices[i].x, vertices[i].y, vertices[i].z});
  }

  if (Desc.IndexCount > 0) {
    const auto *indices = static_cast<const uint32 *>(Desc.IndexData);
    Mesh->Indices.assign(indices, indices + Desc.IndexCount);
  }

  Mesh->Topology = Desc.bIsLine ? D3D11_PRIMITIVE_TOPOLOGY_LINELIST
                                : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

  Mesh->LocalBounds = FAxisAlignedBoundingBox{*Mesh.get()};
  return Mesh;
}

void FRenderer::GetDeviceAndContext_ImplDX11(ID3D11Device *&DeviceOut,
                                             ID3D11DeviceContext *&ContextOut) {
  DeviceOut = Device.Get();
  ContextOut = Context.Get();
}

TSharedPtr<FRenderPipeline>
FRenderer::CreateRenderPipeline(const FRenderPipelineDesc &Desc, EViewModeIndex RenderMode) {
  namespace fs = std::filesystem;

  FRenderPipelineDesc ClonedDesc = Desc;

  if (RenderMode == EViewModeIndex::VMI_Wireframe) {
    ClonedDesc.Rasterizer.FillMode = ERasterizerFillMode::Wireframe;
  }

  Microsoft::WRL::ComPtr<ID3DBlob> Blob;
  const fs::path VertexShaderPath{ ClonedDesc.VertexShaderFilePath };
  HRESULT Result = D3DReadFileToBlob(VertexShaderPath.wstring().c_str(), &Blob);
  if (FAILED(Result)) {
    return nullptr;
  }

  Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
  Result = Device->CreateVertexShader(Blob->GetBufferPointer(),
                                      Blob->GetBufferSize(), nullptr,
                                      &VertexShader);
  if (FAILED(Result)) {
    return nullptr;
  }

  size_t VSSize = Blob->GetBufferSize();
  FStatsManager::Get().AddMemory(EStatMemoryCategory::VertexShader, Blob->GetBufferSize());
  
  Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
  if (ClonedDesc.bIsInstancing) {
    Result = Device->CreateInputLayout(
        FVertexInstanceLayouts::Layout, FVertexInstanceLayouts::NumElements,
        Blob->GetBufferPointer(), Blob->GetBufferSize(),
        &InputLayout);
  } else {
    Result = Device->CreateInputLayout(
        FVertexLayouts::Layout, FVertexLayouts::NumElements,
        Blob->GetBufferPointer(), Blob->GetBufferSize(),
        &InputLayout);
  }

  if (FAILED(Result)) {
    return nullptr;
  }

  const fs::path PixelShaderPath{ ClonedDesc.PixelShaderFilePath };
  Result = D3DReadFileToBlob(PixelShaderPath.wstring().c_str(), &Blob);
  if (FAILED(Result)) {
    return nullptr;
  }

  Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
  Result = Device->CreatePixelShader(Blob->GetBufferPointer(), Blob->GetBufferSize(), nullptr, &PixelShader);
  if (FAILED(Result)) {
    return nullptr;
  }

  size_t PSSize = Blob->GetBufferSize();
  FStatsManager::Get().AddMemory(EStatMemoryCategory::PixelShader, Blob->GetBufferSize());

  auto RasterizerState = GetOrCreateRasterizerState(ClonedDesc.Rasterizer);
  auto DepthStencilState = GetOrCreateDepthStencilState(ClonedDesc.DepthStencil);
  auto BlendState = GetOrCreateBlendState(ClonedDesc.Blend);
  auto SamplerState = GetOrCreateSamplerState(FTextureSamplerDesc{});

  if (
      !RasterizerState ||
      !DepthStencilState ||
      !BlendState ||
      !SamplerState
      ) {
    return nullptr;
  }

  FRenderPipelineCreateInfo CreateInfo{
      .Desc                 = std::move(ClonedDesc),
      .VertexShader         = std::move(VertexShader),
      .PixelShader          = std::move(PixelShader),
      .InputLayout          = std::move(InputLayout),
      .RasterizerState      = std::move(RasterizerState),
      .DepthStencilState    = std::move(DepthStencilState),
      .SamplerState         = std::move(SamplerState),
      .BlendState           = std::move(BlendState),
  };

  TSharedPtr<FRenderPipeline> Pipeline = MakeShared<FRenderPipeline>(std::move(CreateInfo));

  Pipeline->SetPixelShaderSize(VSSize);
  Pipeline->SetPixelShaderSize(PSSize);

  return Pipeline;
}

Microsoft::WRL::ComPtr<ID3D11RasterizerState>
FRenderer::GetOrCreateRasterizerState(const FRasterizerDesc& Desc) {
  if (const auto It = RasterizerStateMap.find(Desc); It != RasterizerStateMap.end()) {
    return It->second;
  }

  static const TMap<ERasterizerFillMode, D3D11_FILL_MODE> FillModeMap{
      {ERasterizerFillMode::Solid, D3D11_FILL_SOLID},
      {ERasterizerFillMode::Wireframe, D3D11_FILL_WIREFRAME},
  };

  static const TMap<ERasterizerCullMode, D3D11_CULL_MODE> CullModeMap{
      {ERasterizerCullMode::None, D3D11_CULL_NONE},
      {ERasterizerCullMode::Front, D3D11_CULL_FRONT},
      {ERasterizerCullMode::Back, D3D11_CULL_BACK},
  };
  static const TMap<ERasterizerFrontFaceMode, BOOL> FrontFaceMap{
      {ERasterizerFrontFaceMode::CounterClockwise, TRUE},
      {ERasterizerFrontFaceMode::Clockwise, FALSE},
  };

  const D3D11_RASTERIZER_DESC NativeDesc{
      .FillMode = FillModeMap.at(Desc.FillMode),
      .CullMode = CullModeMap.at(Desc.CullMode),
      .FrontCounterClockwise = FrontFaceMap.at(Desc.FrontFace),
      .MultisampleEnable = Desc.bUseMultisample,
      .AntialiasedLineEnable = Desc.bUseAntialiasedLine,
  };

  Microsoft::WRL::ComPtr<ID3D11RasterizerState> State;
  if (FAILED(Device->CreateRasterizerState(&NativeDesc, &State))) {
    return nullptr;
  }
  RasterizerStateMap.emplace(Desc, State);
  return State;
}

Microsoft::WRL::ComPtr<ID3D11DepthStencilState>
FRenderer::GetOrCreateDepthStencilState(const FDepthStencilDesc& Desc) {
  if (const auto It = DepthStencilStateMap.find(Desc); It != DepthStencilStateMap.end()) {
    return It->second;
  }

  static const TMap<EDepthWriteMode, D3D11_DEPTH_WRITE_MASK> DepthWriteMap{
      {EDepthWriteMode::Disable, D3D11_DEPTH_WRITE_MASK_ZERO},
      {EDepthWriteMode::Enable, D3D11_DEPTH_WRITE_MASK_ALL},
  };

  D3D11_DEPTH_STENCIL_DESC NativeDesc{};
  NativeDesc.DepthEnable = Desc.bDepthEnable;
  NativeDesc.DepthWriteMask = DepthWriteMap.at(Desc.DepthWrite);
  NativeDesc.DepthFunc = D3D11_COMPARISON_LESS;
  NativeDesc.StencilEnable = Desc.bStencilEnable;
  NativeDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
  NativeDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
  NativeDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  NativeDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
  NativeDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  NativeDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
  NativeDesc.BackFace = NativeDesc.FrontFace;

  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> State;
  if (FAILED(Device->CreateDepthStencilState(&NativeDesc, &State))) {
    return nullptr;
  }
  DepthStencilStateMap.emplace(Desc, State);
  return State;
}

Microsoft::WRL::ComPtr<ID3D11BlendState>
FRenderer::GetOrCreateBlendState(const FBlendDesc& Desc) {
  if (const auto It = BlendStateMap.find(Desc); It != BlendStateMap.end()) {
    return It->second;
  }

  static const TMap<EBlendMode, D3D11_RENDER_TARGET_BLEND_DESC> BlendModeMap{
      {EBlendMode::Opaque,
       {.BlendEnable = FALSE,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL}},
      {EBlendMode::Masked,
       {.BlendEnable = FALSE,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL}},
      {EBlendMode::Translucent,
       {.BlendEnable = TRUE,
        .SrcBlend = D3D11_BLEND_SRC_ALPHA,
        .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
        .BlendOp = D3D11_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D11_BLEND_ONE,
        .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL}},
      {EBlendMode::Additive,
       {.BlendEnable = TRUE,
        .SrcBlend = D3D11_BLEND_ONE,
        .DestBlend = D3D11_BLEND_ONE,
        .BlendOp = D3D11_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D11_BLEND_ONE,
        .DestBlendAlpha = D3D11_BLEND_ZERO,
        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL}},
      {EBlendMode::PremultipliedAlpha,
       {.BlendEnable = TRUE,
        .SrcBlend = D3D11_BLEND_ONE,
        .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
        .BlendOp = D3D11_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D11_BLEND_ONE,
        .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL}},
  };

  D3D11_BLEND_DESC NativeDesc{};
  NativeDesc.RenderTarget[0] = BlendModeMap.at(Desc.BlendMode);

  Microsoft::WRL::ComPtr<ID3D11BlendState> State;

  if (FAILED(Device->CreateBlendState(&NativeDesc, &State))) {
    return nullptr;
  }

  BlendStateMap.emplace(Desc, State);
  return State;
}

Microsoft::WRL::ComPtr<ID3D11SamplerState>
FRenderer::GetOrCreateSamplerState(const FTextureSamplerDesc& Desc) {
  if (const auto It = SamplerStateMap.find(Desc); It != SamplerStateMap.end()) {
    return It->second;
  }

  static const TMap<ETextureSamplerFilterMode, D3D11_FILTER> FilterModeMap{
      {ETextureSamplerFilterMode::Point, D3D11_FILTER_MIN_MAG_MIP_POINT},
      {ETextureSamplerFilterMode::Bilinear,
       D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT},
      {ETextureSamplerFilterMode::Trilinear, D3D11_FILTER_MIN_MAG_MIP_LINEAR},
      {ETextureSamplerFilterMode::Anisotropic, D3D11_FILTER_ANISOTROPIC},
  };
  static const TMap<ETextureSamplerWrapMode, D3D11_TEXTURE_ADDRESS_MODE>
      WrapModeMap{
          {ETextureSamplerWrapMode::Wrap, D3D11_TEXTURE_ADDRESS_WRAP},
          {ETextureSamplerWrapMode::Mirror, D3D11_TEXTURE_ADDRESS_MIRROR},
          {ETextureSamplerWrapMode::Clamp, D3D11_TEXTURE_ADDRESS_CLAMP},
      };
  static const TMap<ETextureSamplerFilterMode, UINT> MaxAnisotropyMap{
      {ETextureSamplerFilterMode::Point, 1u},
      {ETextureSamplerFilterMode::Bilinear, 1u},
      {ETextureSamplerFilterMode::Trilinear, 1u},
      {ETextureSamplerFilterMode::Anisotropic, 16u},
  };

  const D3D11_TEXTURE_ADDRESS_MODE Address = WrapModeMap.at(Desc.WrapMode);
  const D3D11_SAMPLER_DESC NativeDesc{
      .Filter = FilterModeMap.at(Desc.FilterMode),
      .AddressU = Address,
      .AddressV = Address,
      .AddressW = Address,
      .MaxAnisotropy = MaxAnisotropyMap.at(Desc.FilterMode),
      .ComparisonFunc = D3D11_COMPARISON_NEVER,
      .MaxLOD = D3D11_FLOAT32_MAX,
  };

  Microsoft::WRL::ComPtr<ID3D11SamplerState> State;
  if (FAILED(Device->CreateSamplerState(&NativeDesc, &State))) {
    return nullptr;
  }
  SamplerStateMap.emplace(Desc, State);
  return State;
}

TSharedPtr<FTexture> FRenderer::CreateTexture(const wchar_t *path) {
  auto Texture = TSharedPtr<FTexture>{new FTexture()};
  Microsoft::WRL::ComPtr<ID3D11Resource> TempResource;

  // dds first
  HRESULT hr = DirectX::CreateDDSTextureFromFile(
      Device.Get(), path, TempResource.GetAddressOf(),
      Texture->TextureSRV.GetAddressOf());  

  // If dds failed  
  if (FAILED(hr)) 
  {
      // Grayscale images are otherwise created as R8_UNORM. Sampling an R8
      // texture as RGBA yields (gray, 0, 0, 1), which makes grayscale base
      // color textures appear red. Convert WIC textures to RGBA explicitly.
      hr = DirectX::CreateWICTextureFromFileEx(
          Device.Get(), path, 0,
          D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
          DirectX::WIC_LOADER_FORCE_RGBA32,
          TempResource.GetAddressOf(), Texture->TextureSRV.GetAddressOf());
  }

  hr = TempResource.As(&Texture->Texture2D);
  if (FAILED(hr))
  {
      return nullptr;
  }

  D3D11_TEXTURE2D_DESC desc;
  Texture->Texture2D->GetDesc(&desc);
  Texture->Width = desc.Width;
  Texture->Height = desc.Height;
  Texture->Format = desc.Format;
  Texture->MipLevels = desc.MipLevels;
  Texture->MemorySize = Texture->GetMemorySize();

  FStatsManager::Get().AddMemory(EStatMemoryCategory::Texture, Texture->MemorySize);

  return Texture;
}

TSharedPtr<FTexture> FRenderer::CreateSolidTexture(const FVector4& Color)
{
    auto Texture = TSharedPtr<FTexture>{ new FTexture() };

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = 1;
    desc.Height = 1;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    uint8_t pixel[4] = {
        static_cast<uint8_t>(std::clamp(Color.X, 0.0f, 1.0f) * 255.0f),
        static_cast<uint8_t>(std::clamp(Color.Y, 0.0f, 1.0f) * 255.0f),
        static_cast<uint8_t>(std::clamp(Color.Z, 0.0f, 1.0f) * 255.0f),
        static_cast<uint8_t>(std::clamp(Color.W, 0.0f, 1.0f) * 255.0f)
    };

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = pixel;
    InitData.SysMemPitch = 4;

    if (FAILED(Device->CreateTexture2D(&desc, &InitData, Texture->Texture2D.GetAddressOf())))
    {
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    if (FAILED(Device->CreateShaderResourceView(Texture->Texture2D.Get(), &srvDesc, Texture->TextureSRV.GetAddressOf())))
    {
        return nullptr;
    }

    Texture->Width = 1;
    Texture->Height = 1;
    Texture->Format = desc.Format;
    Texture->MipLevels = 1;

    return Texture;
}

TSharedPtr<FRenderPipeline> FRenderer::GetPipeline(const FName &Id) const {
  return FRenderResourceLibrary::Get().GetPipeline(Id);
}

bool FRenderer::InitializeDeviceAndSwapChain(HWND Window) {
  constexpr D3D_FEATURE_LEVEL FeatureLevels[] = {D3D_FEATURE_LEVEL_11_0};

  DXGI_SWAP_CHAIN_DESC SwapChainDesc{
      .BufferDesc =
          {
              .Width = 0u,
              .Height = 0u,
              .Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
          },
      .SampleDesc =
          {
              .Count = 1u,
          },
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = 2u,
      .OutputWindow = Window,
      .Windowed = true,
      .SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
  };

  UINT CreateDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifndef NDEBUG
  CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  HRESULT Result = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, CreateDeviceFlags,
      FeatureLevels, ARRAYSIZE(FeatureLevels), D3D11_SDK_VERSION,
      &SwapChainDesc, &SwapChain, &Device, nullptr, &Context);
  if (FAILED(Result)) {
    return false;
  }

  RECT ClientRect{};
  GetClientRect(Window, &ClientRect);

  Viewport = {
      .TopLeftX = 0.0f,
      .TopLeftY = 0.0f,
      .Width = static_cast<float>(ClientRect.right - ClientRect.left),
      .Height = static_cast<float>(ClientRect.bottom - ClientRect.top),
      .MinDepth = 0.0f,
      .MaxDepth = 1.0f,
  };

  return true;
}

bool FRenderer::InitializeBackBufferAndDepthStencil() {
  Microsoft::WRL::ComPtr<ID3D11Texture2D> BackBuffer;
  HRESULT Result = SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
  if (FAILED(Result)) {
    return false;
  }

  Result =
      Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, &BackBufferRTV);
  if (FAILED(Result)) {
    return false;
  }

  const UINT Width = static_cast<UINT>(Viewport.Width);
  const UINT Height = static_cast<UINT>(Viewport.Height);

  D3D11_TEXTURE2D_DESC DepthStencilDesc = {
      .Width = Width,
      .Height = Height,
      .MipLevels = 1u,
      .ArraySize = 1u,
      .Format = DXGI_FORMAT_R24G8_TYPELESS,
      .SampleDesc =
          {
              .Count = 1u,
          },
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
  };

  Result =
      Device->CreateTexture2D(&DepthStencilDesc, nullptr, &DepthStencilBuffer);
  if (FAILED(Result)) {
    return false;
  }

  D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc{
      .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
      .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
  };
  Result = Device->CreateDepthStencilView(DepthStencilBuffer.Get(), &DsvDesc,
                                          &DepthStencilView);
  if (FAILED(Result)) {
    return false;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC StencilSrvDesc{
      .Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT,
      .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
      .Texture2D = {.MostDetailedMip = 0, .MipLevels = 1},
  };
  Result = Device->CreateShaderResourceView(DepthStencilBuffer.Get(),
                                            &StencilSrvDesc, &DepthStencilSRV);
  if (FAILED(Result)) {
    return false;
  }

  return true;
}

bool FRenderer::InitializeEditorViewportRenderTarget() {
  if (!Device) {
    return false;
  }

  const UINT Width = static_cast<UINT>(Viewport.Width);
  const UINT Height = static_cast<UINT>(Viewport.Height);
  if (Width == 0 || Height == 0) {
    return false;
  }

  D3D11_TEXTURE2D_DESC ColorTexDesc{
      .Width = Width,
      .Height = Height,
      .MipLevels = 1u,
      .ArraySize = 1u,
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .SampleDesc = {.Count = 1u},
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
  };

  HRESULT Result =
      Device->CreateTexture2D(&ColorTexDesc, nullptr, &renderTexture);
  if (FAILED(Result)) {
    return false;
  }

  Result = Device->CreateRenderTargetView(renderTexture.Get(), nullptr,
                                          &EditorViewPortRTV);
  if (FAILED(Result)) {
    return false;
  }

  Result = Device->CreateShaderResourceView(renderTexture.Get(), nullptr,
                                            &EditorViewPortSRV);
  if (FAILED(Result)) {
    return false;
  }

  return true;
}

bool FRenderer::InitializeConstantBuffers() {
  // b2를 쓰는 Object/Grid 상수 타입이 공유하는 버퍼.
  // 가장 큰 구조체보다 크게 잡아두고, 초과 여부는 UpdateBuffer의
  // static_assert가 잡는다.
  D3D11_BUFFER_DESC ObjectConstantBufferDesc = {
      .ByteWidth = ConstantBufferSize,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
  };

  HRESULT Result = Device->CreateBuffer(&ObjectConstantBufferDesc, nullptr, &ObjectConstantBuffer);
  if (FAILED(Result)) {
    return false;
  }

  D3D11_BUFFER_DESC FrameConstantBufferDesc = {
      .ByteWidth = sizeof(FFrameConstants),
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
  };

  Result = Device->CreateBuffer(&FrameConstantBufferDesc, nullptr,
                                &FrameConstantBuffer);

  if (FAILED(Result)) {
    return false;
  }

  D3D11_BUFFER_DESC ViewConstantBufferDesc = {
      .ByteWidth = sizeof(FViewConstants),
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
  };

  Result = Device->CreateBuffer(&ViewConstantBufferDesc, nullptr,
                                &ViewConstantBuffer);

  if (FAILED(Result)) {
    return false;
  }


  D3D11_BUFFER_DESC LightConstantBufferDesc = {
      .ByteWidth = sizeof(FLightConstants),
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
  };

  Result =
      Device->CreateBuffer(&LightConstantBufferDesc, nullptr, &LightConstantBuffer);

  if (FAILED(Result)) {
    return false;
  }

  return true;
}

void FRenderer::UpdateLightConstants(const FLightConstants &Constants, const EViewModeIndex InMode) {
  Context->UpdateSubresource(LightConstantBuffer.Get(), 0, nullptr, &Constants, 0, 0);
  Context->PSSetConstantBuffers(4, 1, LightConstantBuffer.GetAddressOf());
}

void FRenderer::UpdateFrameConstants(const FFrameConstants &Constants) {
  Context->UpdateSubresource(FrameConstantBuffer.Get(), 0, nullptr, &Constants, 0, 0);
  Context->VSSetConstantBuffers(0, 1, FrameConstantBuffer.GetAddressOf());
  Context->PSSetConstantBuffers(0, 1, FrameConstantBuffer.GetAddressOf());
}

void FRenderer::Draw(const FDrawCommand &Command, uint32 Slot,
                     bool bApplyViewMode) {
  if (!Command.Mesh || Command.Materials.empty()) {
    return;
  }

  if (!Command.Mesh->Sections.empty())
  {
      for (size_t i = 0; i < Command.Mesh->Sections.size(); i++)
      {
          const auto& Section = Command.Mesh->Sections[i];

          const FMaterial& Mat = (i < Command.Materials.size()) ? Command.Materials[i] : Command.Materials[0];         

          DrawSection(*Command.Mesh, Mat, Command.Constants, Section.StartIndex, Section.IndexCount, Slot, bApplyViewMode);
      }
  }
  else
  {
      Draw(*Command.Mesh, Command.Materials[0], Command.Constants, Slot, bApplyViewMode);
  }

}

void FRenderer::AddTextInstanceArray(const FDrawCommand &Command) {
  // 빈 데이터 전달 시 조기 반환
  if (!Command.Mesh || Command.Materials.empty() || Command.Instances.empty()) {
    return;
  }
  auto &ResLib = FRenderResourceLibrary::Get();

  auto &TargetArray =
      ResLib.GetInstancingArray(Command.Mesh, &Command.Materials[0]);
  TargetArray.reserve(TargetArray.size() + Command.Instances.size());
  TargetArray.insert(TargetArray.end(), Command.Instances.begin(),
                     Command.Instances.end());
}

void FRenderer::DrawInstances(const FCamera &Camera) {
  auto &ResLib = FRenderResourceLibrary::Get();

  FObjectConstants SC{};
  SC.MVP = Camera.CreateViewProjectionMatrix();

  // 배치 키(MaterialID, MeshID) 순회
  for (const auto &[BatchKey, InstanceData] : ResLib.AllInstancingArrayMap) {
    if (InstanceData.empty())
      continue;

    SC.DisableShading =
        CurrentRenderMode == EViewModeIndex::VMI_Unlit ? 1.0f : 0.0f;
    UpdateBuffer(SC, 2);

    const UINT InstanceCount = static_cast<UINT>(InstanceData.size());
    const UINT RequiredSize = InstanceCount * sizeof(FInstanceData);

    // 버퍼 크기 부족 시 동적 확장
    if (RequiredSize > TextInstanceBufferSize) {
      const size_t OldSize = TextInstanceBufferSize;
      Microsoft::WRL::ComPtr<ID3D11Buffer> NewBuffer;

      // InstanceBuffer.Reset();
      D3D11_BUFFER_DESC Desc{};
      Desc.ByteWidth = RequiredSize;
      Desc.Usage = D3D11_USAGE_DYNAMIC;
      Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
      Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

      if (FAILED(Device->CreateBuffer(&Desc, nullptr, &NewBuffer))) {
          continue;
      }
      InstanceBuffer = NewBuffer;
      TextInstanceBufferSize = RequiredSize;
    }

    // 인스턴스 데이터 업로드
    D3D11_MAPPED_SUBRESOURCE MappedResource{};
    if (FAILED(Context->Map(InstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                            &MappedResource)))
      continue;
    std::memcpy(MappedResource.pData, InstanceData.data(), RequiredSize);
    Context->Unmap(InstanceBuffer.Get(), 0);

    // 머티리얼 및 파이프라인 바인딩
    const FMaterial *Material = BatchKey.Material;
    if (!Material)
      continue;

    FRenderPipeline *Pipeline = Material->GetPipeline();
    if (Pipeline) {
      Pipeline->Bind(*Context.Get());
    }
    Material->BindResources(*Context.Get());

    // 메시 조회 및 바인딩
    const FMesh *Mesh = BatchKey.Mesh;
    if (!Mesh)
      continue;
    Mesh->BindResources(*Context.Get());

    // 슬롯 1에 인스턴스 버퍼 바인딩
    UINT Stride = sizeof(FInstanceData);
    UINT Offset = 0;
    Context->IASetVertexBuffers(1, 1, InstanceBuffer.GetAddressOf(), &Stride,
                                &Offset);

    // 인스턴스 렌더링 호출
    if (Mesh->HasIndices()) {
      Context->DrawIndexedInstanced(Mesh->GetIndexCount(), InstanceCount, 0, 0,
                                    0);
    } else {
      Context->DrawInstanced(Mesh->VertexCount, InstanceCount, 0, 0);
    }
  }
}

void FRenderer::DrawTextInstances(const FDrawCommand &Command) {
  if (!Command.Mesh || Command.Materials.empty()) {
    return;
  }

  auto &ResLib = FRenderResourceLibrary::Get();

  UpdateBuffer(Command.Constants, 2);

  TArray<FInstanceData> InstanceData =
      ResLib.GetInstancingArray(Command.Mesh, &Command.Materials[0]);

  if (InstanceData.empty())
    return;

  const UINT InstanceCount = static_cast<UINT>(InstanceData.size());
  const UINT RequiredSize = InstanceCount * sizeof(FInstanceData);

  // 버퍼 크기 부족 시 동적 확장
  if (RequiredSize > TextInstanceBufferSize) {
    const size_t OldSize = TextInstanceBufferSize;
    Microsoft::WRL::ComPtr<ID3D11Buffer> NewBuffer;

    // InstanceBuffer.Reset();
    D3D11_BUFFER_DESC Desc{};
    Desc.ByteWidth = RequiredSize;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(Device->CreateBuffer(&Desc, nullptr, &NewBuffer))) {
        return;
    }
    InstanceBuffer = NewBuffer;
    TextInstanceBufferSize = RequiredSize;
  }

  // 인스턴스 데이터 업로드
  D3D11_MAPPED_SUBRESOURCE MappedResource{};
  if (FAILED(Context->Map(InstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                          &MappedResource)))
    return;
  std::memcpy(MappedResource.pData, InstanceData.data(), RequiredSize);
  Context->Unmap(InstanceBuffer.Get(), 0);

  // 머티리얼 및 파이프라인 바인딩
  const FMaterial *Material = &Command.Materials[0];

  FRenderPipeline *Pipeline = Material->GetPipeline();
  if (Pipeline) {
    Pipeline->Bind(*Context.Get());
  }
  Material->BindResources(*Context.Get());

  // 메시 조회 및 바인딩
  const FMesh *Mesh = Command.Mesh;
  Mesh->BindResources(*Context.Get());

  // 슬롯 1에 인스턴스 버퍼 바인딩
  UINT Stride = sizeof(FInstanceData);
  UINT Offset = 0;
  Context->IASetVertexBuffers(1, 1, InstanceBuffer.GetAddressOf(), &Stride,
                              &Offset);

  // 인스턴스 렌더링 호출
  if (Mesh->HasIndices()) {
    Context->DrawIndexedInstanced(Mesh->GetIndexCount(), InstanceCount, 0, 0,
                                  0);
  } else {
    Context->DrawInstanced(Mesh->VertexCount, InstanceCount, 0, 0);
  }
}

void FRenderer::ClearTextInstances() {
  for (auto &[BatchKey, InstanceArray] :
       FRenderResourceLibrary::Get().AllInstancingArrayMap) {
    InstanceArray.clear();
  }

  FRenderResourceLibrary::Get().DestroyAllInstancingArray();
}

void FRenderer::RenderOutline() {
  // 백버퍼 뷰포트 및 토폴로지 복구

  Context->RSSetViewports(1, &Viewport);
  Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  Context->IASetInputLayout(nullptr);

  ID3D11Buffer *NullVB = nullptr;
  UINT Zero = 0;
  Context->IASetVertexBuffers(0, 1, &NullVB, &Zero, &Zero);

  Context->OMSetRenderTargets(1, BackBufferRTV.GetAddressOf(), nullptr);
  // 씬 텍스처와 스텐실 텍스처 바인딩
  ID3D11ShaderResourceView *SRVs[] = {EditorViewPortSRV.Get(),
                                      DepthStencilSRV.Get()};
  Context->PSSetShaderResources(0, 2, SRVs);

  FRenderResourceLibrary::Get()
      .GetPipeline(FName("#PostProcess"))
      ->Bind(*Context.Get());
  Context->Draw(3, 0);

  // 슬롯 해제
  ID3D11ShaderResourceView *NullSRVs[] = {nullptr, nullptr};
  Context->PSSetShaderResources(0, 2, NullSRVs);
}
