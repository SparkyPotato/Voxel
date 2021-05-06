#include "ShadowMap.h"

#include "GBuffer.h"

namespace ShadowMap
{

	ID3D11Buffer* LightMatrix = nullptr;
	ID3D11Buffer* LightBuffer = nullptr;
	D3D11_VIEWPORT m_Viewport;
	ID3D11ShaderResourceView* ShadowMapView = nullptr;
	ID3D11SamplerState* Sampler = nullptr;
	ID3D11DepthStencilView* m_ShadowMapDSV = nullptr;
	ID3D11RasterizerState* m_RasterizerState = nullptr;

	void Initialize()
	{
		D3D11_BUFFER_DESC cDesc{
			.ByteWidth = 32,
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
		};
		Window::Device->CreateBuffer(&cDesc, nullptr, &LightBuffer);
		cDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4) * 2;
		Window::Device->CreateBuffer(&cDesc, nullptr, &LightMatrix);

		D3D11_SAMPLER_DESC sDesc{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11_TEXTURE_ADDRESS_BORDER,
			.AddressV = D3D11_TEXTURE_ADDRESS_BORDER,
			.AddressW = D3D11_TEXTURE_ADDRESS_BORDER,
			.BorderColor = { 0.f, 0.f, 0.f, 0.f }
		};
		Window::Device->CreateSamplerState(&sDesc, &Sampler);

		D3D11_RASTERIZER_DESC rDesc{
			.FillMode = D3D11_FILL_SOLID,
			.CullMode = D3D11_CULL_FRONT,
			.FrontCounterClockwise = FALSE,
			.DepthBias = 0,
			.DepthBiasClamp = 0.f,
			.SlopeScaledDepthBias = 0.f,
			.DepthClipEnable = TRUE,
			.ScissorEnable = FALSE,
			.MultisampleEnable = FALSE,
			.AntialiasedLineEnable = FALSE
		};
		Window::Device->CreateRasterizerState(&rDesc, &m_RasterizerState);
	}

	void Shutdown()
	{
		LightBuffer->Release();
		LightMatrix->Release();
		ShadowMapView->Release();
		Sampler->Release();
		m_ShadowMapDSV->Release();
		m_RasterizerState->Release();
	}

	void SetLight(const Light& light)
	{
		D3D11_MAPPED_SUBRESOURCE sub;
		Window::Context->Map(LightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);
		memcpy(sub.pData, &light, sizeof(light));
		Window::Context->Unmap(LightBuffer, 0);

		using namespace DirectX;

		auto projection = XMMatrixOrthographicLH(3000.f, 3000.f, 8000.f, 0.1f);
		auto direction = XMLoadFloat3(&light.Direction);
		auto up = XMVector3Cross(XMVectorSet(1.f, 0.f, 0.f, 0.f), direction);
		auto view = XMMatrixLookToLH(direction * 5000.f, -direction, up);

		auto viewProj = view * projection;
		auto inverse = XMMatrixInverse(nullptr, viewProj);

		XMFLOAT4X4 data[2];
		XMStoreFloat4x4(&data[0], XMMatrixTranspose(viewProj));
		XMStoreFloat4x4(&data[1], XMMatrixTranspose(inverse));

		Window::Context->Map(LightMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);
		memcpy(sub.pData, &data, sizeof(data));
		Window::Context->Unmap(LightMatrix, 0);
	}

	void Resize(uint32_t width)
	{
		if (ShadowMapView) ShadowMapView->Release();
		if (m_ShadowMapDSV) m_ShadowMapDSV->Release();

		m_Viewport = D3D11_VIEWPORT{
			.TopLeftX = 0.f,
			.TopLeftY = 0.f,
			.Width = float(width),
			.Height = float(width),
			.MinDepth = 0.f,
			.MaxDepth = 1.f
		};

		ID3D11Texture2D* depth;

		D3D11_TEXTURE2D_DESC desc{
			.Width = width,
			.Height = width,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI_FORMAT_R32_TYPELESS,
			.SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0 },
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
		};
		Window::Device->CreateTexture2D(&desc, nullptr, &depth);

		D3D11_DEPTH_STENCIL_VIEW_DESC dDesc{
			.Format = DXGI_FORMAT_D32_FLOAT,
			.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
			.Texture2D = 0
		};
		Window::Device->CreateDepthStencilView(depth, &dDesc, &m_ShadowMapDSV);

		D3D11_SHADER_RESOURCE_VIEW_DESC rDesc{
			.Format = DXGI_FORMAT_R32_FLOAT,
			.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
			.Texture2D = D3D11_TEX2D_SRV{.MostDetailedMip = 0, .MipLevels = 1 }
		};
		Window::Device->CreateShaderResourceView(depth, &rDesc, &ShadowMapView);

		depth->Release();
	}

	void Write(Scene* scene)
	{
		Window::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		float clear[] = { 0.f, 0.f, 0.f, 0.f };
		Window::Context->ClearDepthStencilView(m_ShadowMapDSV, D3D11_CLEAR_DEPTH, 0.f, 0);

		ID3D11RenderTargetView* views[] = { nullptr, nullptr };
		Window::Context->OMSetRenderTargets(2, views, m_ShadowMapDSV);
		Window::Context->OMSetDepthStencilState(GBuffer::DepthState, 0);
		Window::Context->RSSetState(m_RasterizerState);
		Window::Context->VSSetShader(GBuffer::WriteVS, nullptr, 0);
		Window::Context->GSSetShader(nullptr, nullptr, 0);
		Window::Context->PSSetShader(nullptr, nullptr, 0);
		Window::Context->RSSetViewports(1, &m_Viewport);
		UINT offset = 0;
		UINT stride = sizeof(Vertex);
		Window::Context->IASetVertexBuffers(0, 1, &scene->VertexBuffer, &stride, &offset);
		Window::Context->IASetIndexBuffer(scene->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		Window::Context->IASetInputLayout(GBuffer::Layout);
		Window::Context->VSSetConstantBuffers(0, 1, &LightMatrix);

		for (const auto& material : scene->Materials)
		{
			Window::Context->DrawIndexed(material.IndexCount, material.BaseIndex, material.BaseVertex);
		}
		Window::Context->OMSetRenderTargets(0, nullptr, nullptr);
	}

}
