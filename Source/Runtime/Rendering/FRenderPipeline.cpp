#include "FRenderPipeline.h"

#include <utility>

FRenderPipeline::FRenderPipeline(FRenderPipelineCreateInfo CreateInfo)
    : desc              { std::move(CreateInfo.Desc) }
    , VertexShader      { std::move(CreateInfo.VertexShader) }
    , PixelShader       { std::move(CreateInfo.PixelShader) }
    , InputLayout       { std::move(CreateInfo.InputLayout) }
    , RasterizerState   { std::move(CreateInfo.RasterizerState) }
    , DepthStencilState { std::move(CreateInfo.DepthStencilState) }
    , SamplerState      { std::move(CreateInfo.SamplerState) }
    , BlendState        { std::move(CreateInfo.BlendState) }
{
}

void FRenderPipeline::Bind(ID3D11DeviceContext &Context) const {
  Context.IASetInputLayout(InputLayout.Get());

  Context.VSSetShader(VertexShader.Get(), nullptr, 0);
  Context.PSSetShader(PixelShader.Get(), nullptr, 0);

  Context.RSSetState(RasterizerState.Get());
  Context.OMSetDepthStencilState(DepthStencilState.Get(), StencilRef);
  Context.OMSetBlendState(BlendState.Get(), nullptr, 0xFFFFFFFF);

  Context.PSSetSamplers(0u, 1u, SamplerState.GetAddressOf());
}
