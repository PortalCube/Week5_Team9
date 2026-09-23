#include "FMaterial.h"
#include "FRenderer.h"
#include "FRenderResourceLibrary.h"
#include "Runtime/Core/Log.h"
#include <d3d11.h>
#include <algorithm>

void FMaterial::BindResources(ID3D11DeviceContext& Context) const
{
    // 텍스처가 없어도 반드시 바인딩한다.
    // D3D 상태는 끈끈해서, 건너뛰면 이전 드로우의 SRV가 슬롯에 남는다.
    ID3D11ShaderResourceView* SRVs[1] = { Texture ? Texture->GetSRV() : nullptr };
    Context.PSSetShaderResources(0u, 1u, SRVs);
}
