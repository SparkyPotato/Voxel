#include "GBuffer.h"

namespace GBuffer
{

	ID3D11RenderTargetView* m_Buffers[2] = { nullptr, nullptr };
	ID3D11DepthStencilView* m_DepthBuffer = nullptr;
	ID3D11DepthStencilState* DepthState = nullptr;
	ID3D11SamplerState* SamplerState = nullptr;
	ID3D11ShaderResourceView* Views[3] = { nullptr, nullptr, nullptr };

	D3D11_VIEWPORT Viewport;
	ID3D11InputLayout* Layout = nullptr;
	ID3D11VertexShader* WriteVS = nullptr;
	ID3D11PixelShader* m_WritePS = nullptr;
	ID3D11Buffer* CameraBuffer = nullptr;
	ID3D11Buffer* m_WritePSBuffer = nullptr;

	ID3D11VertexShader* ReadVS = nullptr;
	ID3D11PixelShader* m_ReadPS = nullptr;
	ID3D11Buffer* m_ReadPSBuffer = nullptr;

	DirectX::XMMATRIX m_Projection;
	uint32_t m_RenderMode;

	void Initialize()
	{
		ID3DBlob* blob;
		D3DReadFileToBlob(L"GBufferWriteVS.cso", &blob);
		Window::Device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &WriteVS);

		D3D11_INPUT_ELEMENT_DESC iaDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

			{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT,
			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		Window::Device->CreateInputLayout(
			iaDesc, sizeof(iaDesc) / sizeof(iaDesc[0]),
			blob->GetBufferPointer(), blob->GetBufferSize(), &Layout
		);

		D3DReadFileToBlob(L"GBufferWritePS.cso", &blob);
		Window::Device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_WritePS);
		D3DReadFileToBlob(L"GBufferReadVS.cso", &blob);
		Window::Device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &ReadVS);
		D3DReadFileToBlob(L"GBufferReadPS.cso", &blob);
		Window::Device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_ReadPS);

		D3D11_DEPTH_STENCIL_DESC desc{
			.DepthEnable = TRUE,
			.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL
		};
		Window::Device->CreateDepthStencilState(&desc, &DepthState);

		D3D11_BUFFER_DESC cDesc{
			.ByteWidth = sizeof(DirectX::XMFLOAT4X4) * 2,
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
			.MiscFlags = 0,
			.StructureByteStride = 0
		};
		Window::Device->CreateBuffer(&cDesc, nullptr, &CameraBuffer);
		cDesc.ByteWidth = 16;
		Window::Device->CreateBuffer(&cDesc, nullptr, &m_WritePSBuffer);
		Window::Device->CreateBuffer(&cDesc, nullptr, &m_ReadPSBuffer);

		D3D11_SAMPLER_DESC sDesc{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11_TEXTURE_ADDRESS_WRAP
		};
		Window::Device->CreateSamplerState(&sDesc, &SamplerState);

		Window::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void Shutdown()
	{
		Layout->Release();
		SamplerState->Release();
		WriteVS->Release();
		m_WritePS->Release();
		CameraBuffer->Release();
		m_WritePSBuffer->Release();

		ReadVS->Release();
		m_ReadPS->Release();
		m_ReadPSBuffer->Release();

		DepthState->Release();

		for (auto target : m_Buffers)
		{
			target->Release();
		}
		m_DepthBuffer->Release();

		for (auto view : Views)
		{
			view->Release();
		}
	}

	void SetDebugMode(uint32_t mode)
	{
		D3D11_MAPPED_SUBRESOURCE sub;
		Window::Context->Map(m_ReadPSBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);
		memcpy(sub.pData, &mode, sizeof(mode));
		Window::Context->Unmap(m_ReadPSBuffer, 0);
	}

	void SetViewMatrix(const DirectX::XMMATRIX& matrix)
	{
		using namespace DirectX;

		XMMATRIX viewProj = matrix * m_Projection;
		XMFLOAT4X4 viewProjection;
		XMStoreFloat4x4(&viewProjection, XMMatrixTranspose(viewProj));
		XMMATRIX inv = XMMatrixInverse(nullptr, viewProj);
		XMFLOAT4X4 inverse;
		XMStoreFloat4x4(&inverse, XMMatrixTranspose(inv));

		D3D11_MAPPED_SUBRESOURCE sub;
		Window::Context->Map(CameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);
		memcpy(sub.pData, &viewProjection, sizeof(viewProjection));
		memcpy((uint8_t*)sub.pData + sizeof(viewProjection), &inverse, sizeof(viewProjection));
		Window::Context->Unmap(CameraBuffer, 0);
	}

	void Write(Scene* scene)
	{
		Window::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		float clear[] = { 0.f, 0.f, 0.f, 0.f };
		Window::Context->ClearDepthStencilView(m_DepthBuffer, D3D11_CLEAR_DEPTH, 0.f, 0);
		Window::Context->ClearRenderTargetView(m_Buffers[0], clear);
		Window::Context->ClearRenderTargetView(m_Buffers[1], clear);

		Window::Context->OMSetRenderTargets(2, m_Buffers, m_DepthBuffer);
		Window::Context->OMSetDepthStencilState(DepthState, 0);
		Window::Context->VSSetShader(WriteVS, nullptr, 0);
		Window::Context->GSSetShader(nullptr, nullptr, 0);
		Window::Context->PSSetShader(m_WritePS, nullptr, 0);
		Window::Context->RSSetViewports(1, &Viewport);
		UINT offset = 0;
		UINT stride = sizeof(Vertex);
		Window::Context->IASetVertexBuffers(0, 1, &scene->VertexBuffer, &stride, &offset);
		Window::Context->IASetIndexBuffer(scene->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		Window::Context->IASetInputLayout(Layout);
		Window::Context->VSSetConstantBuffers(0, 1, &CameraBuffer);
		Window::Context->PSSetConstantBuffers(0, 1, &m_WritePSBuffer);
		Window::Context->PSSetSamplers(0, 1, &SamplerState);

		for (const auto& material : scene->Materials)
		{
			D3D11_MAPPED_SUBRESOURCE sub;
			Window::Context->Map(m_WritePSBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);
			memcpy(sub.pData, &material.BumpMapSize, sizeof(material.BumpMapSize));
			Window::Context->Unmap(m_WritePSBuffer, 0);

			Window::Context->PSSetShaderResources(0, 3, &material.Albedo);
			Window::Context->DrawIndexed(material.IndexCount, material.BaseIndex, material.BaseVertex);
		}
		Window::Context->OMSetRenderTargets(0, nullptr, nullptr);
	}

	void DrawDebug()
	{
		Window::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		UINT offset = 0;
		Window::Context->OMSetRenderTargets(1, &Window::RenderTarget, nullptr);
		Window::Context->OMSetDepthStencilState(nullptr, 0);
		Window::Context->VSSetShader(ReadVS, nullptr, 0);
		Window::Context->GSSetShader(nullptr, nullptr, 0);
		Window::Context->PSSetShader(m_ReadPS, nullptr, 0);
		Window::Context->PSSetConstantBuffers(0, 1, &m_ReadPSBuffer);
		Window::Context->PSSetSamplers(0, 1, &SamplerState);
		Window::Context->PSSetShaderResources(0, 3, Views);
		Window::Context->IASetVertexBuffers(0, 0, nullptr, &offset, &offset);
		Window::Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		Window::Context->IASetInputLayout(nullptr);

		Window::Context->Draw(3, 0);
		ID3D11ShaderResourceView* views[] = { nullptr, nullptr, nullptr };
		Window::Context->PSSetShaderResources(0, 3, views);
	}

	void Resize(uint32_t width, uint32_t height)
	{
		if (!width || !height) return;

		m_Projection = DirectX::XMMatrixPerspectiveFovLH(45.f, float(width) / height, 2000.f, 0.1f);

		// GBuffer format:
		// 0: RGB - Albedo, A - Specular
		// 1: RGB - Normal
		// 2: Depth
		for (auto target : m_Buffers)
		{
			if (target) target->Release();
		}
		if (m_DepthBuffer) m_DepthBuffer->Release();

		for (auto view : Views)
		{
			if (view) view->Release();
		}

		ID3D11Texture2D* albedo;
		ID3D11Texture2D* normal;
		ID3D11Texture2D* depth;

		D3D11_TEXTURE2D_DESC desc{
			.Width = width,
			.Height = height,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
			.SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0 },
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE
		};
		Window::Device->CreateTexture2D(&desc, nullptr, &albedo);
		Window::Device->CreateTexture2D(&desc, nullptr, &normal);

		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		auto hr = Window::Device->CreateTexture2D(&desc, nullptr, &depth);

		Window::Device->CreateRenderTargetView(albedo, nullptr, &m_Buffers[0]);
		Window::Device->CreateRenderTargetView(normal, nullptr, &m_Buffers[1]);

		D3D11_DEPTH_STENCIL_VIEW_DESC dDesc{
			.Format = DXGI_FORMAT_D32_FLOAT,
			.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
			.Texture2D = 0
		};
		Window::Device->CreateDepthStencilView(depth, &dDesc, &m_DepthBuffer);

		Window::Device->CreateShaderResourceView(albedo, nullptr, &Views[0]);
		Window::Device->CreateShaderResourceView(normal, nullptr, &Views[1]);

		D3D11_SHADER_RESOURCE_VIEW_DESC rDesc{
			.Format = DXGI_FORMAT_R32_FLOAT,
			.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
			.Texture2D = D3D11_TEX2D_SRV{.MostDetailedMip = 0, .MipLevels = 1 }
		};
		Window::Device->CreateShaderResourceView(depth, &rDesc, &Views[2]);

		Viewport = {
			.TopLeftX = 0.f,
			.TopLeftY = 0.f,
			.Width = float(width),
			.Height = float(height),
			.MinDepth = 0.f,
			.MaxDepth = 1.f
		};

		albedo->Release();
		normal->Release();
		depth->Release();
	}

}
