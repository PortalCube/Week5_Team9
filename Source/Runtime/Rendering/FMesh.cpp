#include "FMesh.h"

#include <d3d11.h>
#include <wrl/client.h>

FMesh::~FMesh()
{
	if (VertexBuffer)
	{
		D3D11_BUFFER_DESC Desc{};
		VertexBuffer->GetDesc(&Desc);
	}

	if (IndexBuffer)
	{
		D3D11_BUFFER_DESC Desc{};
		IndexBuffer->GetDesc(&Desc);
	}
}

void FMesh::BindResources(ID3D11DeviceContext& Context) const
{
	constexpr UINT Offset = 0;

	Context.IASetPrimitiveTopology(Topology);
	Context.IASetVertexBuffers(0, 1, VertexBuffer.GetAddressOf(), &VertexStride, &Offset);
	Context.IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}

bool FMesh::UpdateBuffers(ID3D11Device* Device, ID3D11DeviceContext* Context, const FMeshDesc& Desc)
{
	if (!Device || !Context || !Desc.VertexData || Desc.VertexCount == 0)
	{
		return false;
	}

	// 정점 버퍼 갱신
	if (VertexBuffer && Desc.VertexDataSize <= VertexBufferSize)
	{
		D3D11_MAPPED_SUBRESOURCE Mapped;
		HRESULT hr = Context->Map(VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
		if (SUCCEEDED(hr))
		{
			memcpy(Mapped.pData, Desc.VertexData, Desc.VertexDataSize);
			Context->Unmap(VertexBuffer.Get(), 0);
		}
	}
	else
	{
		Microsoft::WRL::ComPtr<ID3D11Buffer> NewVertexBuffer;

		D3D11_BUFFER_DESC VbDesc = {
			.ByteWidth = Desc.VertexDataSize,
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		};
		D3D11_SUBRESOURCE_DATA VData = { .pSysMem = Desc.VertexData };
		if (FAILED(Device->CreateBuffer(&VbDesc, &VData, &NewVertexBuffer)))
		{
			return false;
		}

		const size_t OldSize = VertexBufferSize;

		VertexBuffer = NewVertexBuffer;
		VertexBufferSize = Desc.VertexDataSize;
	}

	VertexCount = Desc.VertexCount;
	VertexStride = Desc.VertexStride;

	// 인덱스 버퍼 갱신
	if (Desc.IndexCount > 0 && Desc.IndexData)
	{
		if (IndexBuffer && Desc.IndexDataSize <= IndexBufferSize)
		{
			Context->UpdateSubresource(IndexBuffer.Get(), 0, nullptr, Desc.IndexData, 0, 0);
		}
		else
		{
			Microsoft::WRL::ComPtr<ID3D11Buffer> NewIndexBuffer;

			D3D11_BUFFER_DESC IbDesc = {
				.ByteWidth = Desc.IndexDataSize,
				.Usage = D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11_BIND_INDEX_BUFFER,
			};
			D3D11_SUBRESOURCE_DATA IData = { .pSysMem = Desc.IndexData };
			if (FAILED(Device->CreateBuffer(&IbDesc, &IData, &NewIndexBuffer)))
			{
				return false;
			}

			const size_t OldSize = IndexBufferSize;

			IndexBuffer = NewIndexBuffer;
			IndexBufferSize = Desc.IndexDataSize;
		}
		IndexCount = Desc.IndexCount;
	}
	else
	{
		IndexCount = 0;
	}

	// 위치와 인덱스 복사
	Positions.clear();
	Positions.reserve(Desc.VertexCount);
	const auto* vertices = static_cast<const FVertexData*>(Desc.VertexData);
	for (uint32 i = 0; i < Desc.VertexCount; ++i)
	{
		Positions.push_back(FVector{ vertices[i].x, vertices[i].y, vertices[i].z });
	}

	Indices.clear();
	if (Desc.IndexCount > 0 && Desc.IndexData)
	{
		const auto* indices = static_cast<const uint32*>(Desc.IndexData);
		Indices.assign(indices, indices + Desc.IndexCount);
	}

	// 바운딩 박스 갱신
	LocalBounds = FAxisAlignedBoundingBox{ *this };
	return true;
}


