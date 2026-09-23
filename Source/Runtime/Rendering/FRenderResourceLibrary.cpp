#include "FRenderResourceLibrary.h"
#include "Runtime/Resource/FResourceLoader.h"

#include "FRenderer.h"
#include "FTexture.h"
#include <d3dcompiler.h>
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Material/FBlendDesc.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Utility/EngineUtil.h"
#include "ThirdParty/Json/json.hpp"

#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image.h"

FRenderResourceLibrary &FRenderResourceLibrary::Get() {
  static FRenderResourceLibrary Instance;
  return Instance;
}

bool FRenderResourceLibrary::CreateWireframePipeline(FRenderer &Renderer) {
  const FWString Path = EngineUtil::GetContentDirectory();
  const FWString VsPath = Path + L"/Shader/ExampleVS.cso";
  const FWString PsPath = Path + L"/Shader/ExamplePS.cso";

  if (!std::filesystem::exists(VsPath) || !std::filesystem::exists(PsPath)) {
    return false;
  }

  FRenderPipelineDesc Desc = {
      .VertexShaderFilePath = std::filesystem::path(VsPath).string(),
      .PixelShaderFilePath = std::filesystem::path(PsPath).string(),
  };

  TSharedPtr<FRenderPipeline> WireframePipeline =
      Renderer.CreateRenderPipeline(Desc, EViewModeIndex::VMI_Wireframe);
  if (WireframePipeline) {
    AllPipelineMap[FName("#Simple_Wireframe")] = WireframePipeline;
  }

  return WireframePipeline != nullptr;
}

bool FRenderResourceLibrary::CreateOutlinePipeline(FRenderer &Renderer) {
  ID3D11Device *Device = Renderer.GetDevice();
  if (!Device) {
    return false;
  }

  const FWString Path = EngineUtil::GetContentDirectory();
  const FWString VsPath = Path + L"/Shader/ExampleVS.cso";
  const FWString PsPath = Path + L"/Shader/ExamplePS.cso";

  if (!std::filesystem::exists(VsPath) || !std::filesystem::exists(PsPath)) {
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> RasterizerState;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DepthStencilState;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerState;
  Microsoft::WRL::ComPtr<ID3D11BlendState> BlendState;

  // 버텍스 셰이더 로드 및 생성
  Microsoft::WRL::ComPtr<ID3DBlob> Blob;
  HRESULT Result = D3DReadFileToBlob(VsPath.c_str(), &Blob);
  if (FAILED(Result)) {
    return false;
  }

  Result = Device->CreateVertexShader(Blob->GetBufferPointer(),
                                      Blob->GetBufferSize(), nullptr,
                                      &VertexShader);
  if (FAILED(Result)) {
    return false;
  }

  // 입력 레이아웃 생성
  Result = Device->CreateInputLayout(FVertexLayouts::Layout,
                                     FVertexLayouts::NumElements,
                                     Blob->GetBufferPointer(),
                                     Blob->GetBufferSize(),
                                     &InputLayout);
  if (FAILED(Result)) {
    return false;
  }

  // 픽셀 셰이더 로드 및 생성
  Result = D3DReadFileToBlob(PsPath.c_str(), &Blob);
  if (FAILED(Result)) {
    return false;
  }

  Result = Device->CreatePixelShader(Blob->GetBufferPointer(),
                                     Blob->GetBufferSize(), nullptr,
                                     &PixelShader);
  if (FAILED(Result)) {
    return false;
  }

  // 래스터라이저 상태 생성
  D3D11_RASTERIZER_DESC RasterizerDesc{
      .FillMode = D3D11_FILL_SOLID,
      .CullMode = D3D11_CULL_NONE,
      .FrontCounterClockwise = false,
  };
  Result = Device->CreateRasterizerState(&RasterizerDesc, &RasterizerState);
  if (FAILED(Result)) {
    return false;
  }

  // 스텐실 마스크 기록 설정
  D3D11_DEPTH_STENCIL_DESC DepthStencilDesc{};
  DepthStencilDesc.DepthEnable = FALSE;
  DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
  DepthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
  DepthStencilDesc.StencilEnable = TRUE;
  DepthStencilDesc.StencilReadMask = 0xFF;
  DepthStencilDesc.StencilWriteMask = 0xFF;
  DepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  DepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
  DepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
  DepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
  DepthStencilDesc.BackFace = DepthStencilDesc.FrontFace;
  Result = Device->CreateDepthStencilState(&DepthStencilDesc,
                                           &DepthStencilState);
  if (FAILED(Result)) {
    return false;
  }

  // 블렌드 상태 생성
  D3D11_BLEND_DESC BlendDesc{};
  BlendDesc.RenderTarget[0].BlendEnable = FALSE;
  BlendDesc.RenderTarget[0].RenderTargetWriteMask = 0;
  Result = Device->CreateBlendState(&BlendDesc, &BlendState);
  if (FAILED(Result)) {
    return false;
  }

  // 샘플러 상태 생성
  D3D11_SAMPLER_DESC SamplerDesc{
      .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
      .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
      .ComparisonFunc = D3D11_COMPARISON_NEVER,
      .MaxLOD = D3D11_FLOAT32_MAX,
  };
  Result = Device->CreateSamplerState(&SamplerDesc, &SamplerState);
  if (FAILED(Result)) {
    return false;
  }

  FRenderPipelineCreateInfo CreateInfo{
      .VertexShader         = std::move(VertexShader),
      .PixelShader          = std::move(PixelShader),
      .InputLayout          = std::move(InputLayout),
      .RasterizerState      = std::move(RasterizerState),
      .DepthStencilState    = std::move(DepthStencilState),
      .SamplerState         = std::move(SamplerState),
      .BlendState           = std::move(BlendState),
  };

  AllPipelineMap[FName("#Outline")] = MakeShared<FRenderPipeline>(std::move(CreateInfo));
  return true;
}

bool FRenderResourceLibrary::CreatePostProcessPipeline(FRenderer &Renderer) {
  ID3D11Device *Device = Renderer.GetDevice();
  if (!Device) {
    return false;
  }

  const FWString Path = EngineUtil::GetContentDirectory();
  const FWString VsPath = Path + L"/Shader/ScreenQuadVS.cso";
  const FWString PsPath = Path + L"/Shader/OutlinePostProcessPS.cso";

  if (!std::filesystem::exists(VsPath) || !std::filesystem::exists(PsPath)) {
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> RasterizerState;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DepthStencilState;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerState;
  Microsoft::WRL::ComPtr<ID3D11BlendState> BlendState;

  // 버텍스 셰이더 로드 및 생성
  Microsoft::WRL::ComPtr<ID3DBlob> Blob;
  HRESULT Result = D3DReadFileToBlob(VsPath.c_str(), &Blob);
  if (FAILED(Result)) {
    return false;
  }

  Result = Device->CreateVertexShader(Blob->GetBufferPointer(),
                                      Blob->GetBufferSize(), nullptr,
                                      &VertexShader);
  if (FAILED(Result)) {
    return false;
  }

  // 픽셀 셰이더 로드 및 생성
  Result = D3DReadFileToBlob(PsPath.c_str(), &Blob);
  if (FAILED(Result)) {
    return false;
  }

  Result = Device->CreatePixelShader(Blob->GetBufferPointer(),
                                     Blob->GetBufferSize(), nullptr,
                                     &PixelShader);
  if (FAILED(Result)) {
    return false;
  }

  // 래스터라이저 상태 생성
  D3D11_RASTERIZER_DESC RasterizerDesc{
      .FillMode = D3D11_FILL_SOLID,
      .CullMode = D3D11_CULL_NONE,
      .FrontCounterClockwise = false,
  };
  Result = Device->CreateRasterizerState(&RasterizerDesc, &RasterizerState);
  if (FAILED(Result)) {
    return false;
  }

  // 깊이 스텐실 상태 생성
  D3D11_DEPTH_STENCIL_DESC DepthStencilDesc{
      .DepthEnable = FALSE,
      .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO,
      .DepthFunc = D3D11_COMPARISON_ALWAYS,
      .StencilEnable = FALSE,
  };
  Result = Device->CreateDepthStencilState(&DepthStencilDesc,
                                           &DepthStencilState);
  if (FAILED(Result)) {
    return false;
  }

  // 블렌드 상태 생성
  D3D11_BLEND_DESC BlendDesc{};
  BlendDesc.RenderTarget[0].BlendEnable = FALSE;
  BlendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D11_COLOR_WRITE_ENABLE_ALL;
  Result = Device->CreateBlendState(&BlendDesc, &BlendState);
  if (FAILED(Result)) {
    return false;
  }

  // 샘플러 상태 생성
  D3D11_SAMPLER_DESC SamplerDesc{
      .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
      .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
      .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
      .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
      .ComparisonFunc = D3D11_COMPARISON_NEVER,
      .MaxLOD = D3D11_FLOAT32_MAX,
  };

  Result = Device->CreateSamplerState(&SamplerDesc, &SamplerState);

  if (FAILED(Result)) {
    return false;
  }

  FRenderPipelineCreateInfo CreateInfo{
      .VertexShader         = std::move(VertexShader),
      .PixelShader          = std::move(PixelShader),
      .RasterizerState      = std::move(RasterizerState),
      .DepthStencilState    = std::move(DepthStencilState),
      .SamplerState         = std::move(SamplerState),
      .BlendState           = std::move(BlendState),
  };

  AllPipelineMap[FName("#PostProcess")] = MakeShared<FRenderPipeline>(std::move(CreateInfo));
  return true;
}

bool FRenderResourceLibrary::InitializePipelines(FRenderer &Renderer) {
  return CreateWireframePipeline(Renderer) &&
         CreateOutlinePipeline(Renderer) &&
         CreatePostProcessPipeline(Renderer);
}

bool FRenderResourceLibrary::Initialize(FRenderer &Renderer) {
  RendererRef = &Renderer;
  if (!InitializePipelines(Renderer)) { // 파이프라인을 먼저 생성해야 뒤에 material 할당가능
    return false;
  }

  if (!CreateInstancingArrayMap()) {
    return false;
  }

  return true;
}

bool FRenderResourceLibrary::CreateInstancingArrayMap() {
  AllInstancingArrayMap.clear();
  return true;
}

TSharedPtr<FMaterial> FRenderResourceLibrary::RegisterMaterial(const FName& Id, TSharedPtr<FMaterial> inMaterial) {
  if (inMaterial) {
    inMaterial->MaterialId = Id;
  }
  AllMaterialMap[Id] = inMaterial;
  return inMaterial;
}

TSharedPtr<FMesh> FRenderResourceLibrary::GetOrCreateMesh(const FName &ID, const TArray<FVertexData> &vertices) {
  auto it = AllMeshMap.find(ID);
  if (it != AllMeshMap.end())
    return it->second;

  FMeshDesc Desc{.VertexData = vertices.data(),
                 .VertexDataSize =
                     static_cast<uint32>(sizeof(FVertexData) * vertices.size()),
                 .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
                 .VertexCount = static_cast<uint32>(vertices.size())};
  TSharedPtr<FMesh> newMesh =
      RendererRef ? RendererRef->CreateMesh(Desc) : nullptr;
  if (newMesh) {
    AllMeshMap[ID] = newMesh;
  }
  return newMesh;
}
