#include "Voxel.h"

#include "GBuffer.h"
#include "ShadowMap.h"

namespace Voxel 
{

	ID3D11Texture3D* m_DiffuseTexture = nullptr;
	ID3D11ShaderResourceView* DiffuseReadView = nullptr;
	ID3D11UnorderedAccessView* m_DiffuseTextureView = nullptr;
	ID3D11Texture3D* m_MidTexture = nullptr;
	ID3D11ShaderResourceView* m_MidReadView = nullptr;
	ID3D11UnorderedAccessView* m_MidTextureView = nullptr;
	ID3D11Buffer* m_VoxelBuffer = nullptr;
	ID3D11UnorderedAccessView* m_VoxelView = nullptr;
	ID3D11RasterizerState2* m_RasterizerState = nullptr;
	D3D11_VIEWPORT m_Viewport;

	ID3D11Buffer* ConstantBuffer = nullptr;
	ID3D11VertexShader* m_VS = nullptr;
	ID3D11GeometryShader* m_GS = nullptr;
	ID3D11PixelShader* m_PS = nullptr;
	ID3D11ComputeShader* m_CopyCS = nullptr;
	ID3D11ComputeShader* m_BounceCS = nullptr;

	ID3D11VertexShader* m_DebugVS = nullptr;
	ID3D11GeometryShader* m_DebugGS = nullptr;
	ID3D11PixelShader* m_DebugPS = nullptr;

	struct
	{
		DirectX::XMFLOAT3 CameraPosition;
		float VoxelHalfExtent;
		float VoxelGridRes;
	} m_CBuffer;

	void Initialize()
	{
		D3D11_RASTERIZER_DESC2 desc{
			.FillMode = D3D11_FILL_SOLID,
			.CullMode = D3D11_CULL_BACK,
			.FrontCounterClockwise = FALSE,
			.DepthBias = 0,
			.DepthBiasClamp = 0.f,
			.SlopeScaledDepthBias = 0.f,
			.DepthClipEnable = TRUE,
			.ScissorEnable = FALSE,
			.MultisampleEnable = FALSE,
			.AntialiasedLineEnable = FALSE,
			.ForcedSampleCount = 0,
			.ConservativeRaster = D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON
		};
		Window::Device->CreateRasterizerState2(&desc, &m_RasterizerState);

		ID3DBlob* blob;
		D3DReadFileToBlob(L"VoxelizationVS.cso", &blob);
		Window::Device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_VS);
		D3DReadFileToBlob(L"VoxelizationGS.cso", &blob);
		Window::Device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_GS);
		D3DReadFileToBlob(L"VoxelizationPS.cso", &blob);
		Window::Device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_PS);
		D3DReadFileToBlob(L"VoxelCopyCS.cso", &blob);
		Window::Device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_CopyCS);
		D3DReadFileToBlob(L"VoxelBounceCS.cso", &blob);
		Window::Device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_BounceCS);

		D3DReadFileToBlob(L"VoxelDebugVS.cso", &blob);
		Window::Device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_DebugVS);
		D3DReadFileToBlob(L"VoxelDebugGS.cso", &blob);
		Window::Device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_DebugGS);
		D3DReadFileToBlob(L"VoxelDebugPS.cso", &blob);
		Window::Device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_DebugPS);

		D3D11_BUFFER_DESC cDesc{
			.ByteWidth = 32,
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
		};
		Window::Device->CreateBuffer(&cDesc, nullptr, &ConstantBuffer);
	}

	void Shutdown()
	{
		m_RasterizerState->Release();
		ConstantBuffer->Release();
		m_DiffuseTexture->Release();
		DiffuseReadView->Release();
		m_DiffuseTextureView->Release();
		if (m_MidTexture) m_MidTexture->Release();
		if (m_MidReadView) m_MidReadView->Release();
		if (m_MidTextureView) m_MidTextureView->Release();
		m_VoxelBuffer->Release();
		m_VoxelView->Release();

		m_VS->Release();
		m_GS->Release();
		m_PS->Release();
		m_CopyCS->Release();
		m_BounceCS->Release();
		m_DebugVS->Release();
		m_DebugGS->Release();
		m_DebugPS->Release();
	}

	void Voxelize(Scene* scene, DirectX::XMFLOAT3 cameraPosition)
	{
		m_CBuffer.CameraPosition = cameraPosition;

		D3D11_MAPPED_SUBRESOURCE sub;
		Window::Context->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);
		memcpy(sub.pData, &m_CBuffer, sizeof(m_CBuffer));
		Window::Context->Unmap(ConstantBuffer, 0);

		Window::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		Window::Context->RSSetViewports(1, &m_Viewport);
		Window::Context->RSSetState(m_RasterizerState);
		Window::Context->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 1, &m_VoxelView, nullptr);

		Window::Context->VSSetShader(m_VS, nullptr, 0);
		Window::Context->GSSetShader(m_GS, nullptr, 0);
		Window::Context->PSSetShader(m_PS, nullptr, 0);

		UINT offset = 0;
		UINT stride = sizeof(Vertex);
		Window::Context->IASetVertexBuffers(0, 1, &scene->VertexBuffer, &stride, &offset);
		Window::Context->IASetIndexBuffer(scene->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		Window::Context->IASetInputLayout(GBuffer::Layout);
		Window::Context->GSSetConstantBuffers(1, 1, &ConstantBuffer);
		Window::Context->PSSetConstantBuffers(1, 1, &ConstantBuffer);
		Window::Context->PSSetConstantBuffers(2, 1, &ShadowMap::LightBuffer);
		Window::Context->PSSetConstantBuffers(3, 1, &ShadowMap::LightMatrix);
		Window::Context->PSSetShaderResources(4, 1, &ShadowMap::ShadowMapView);
		Window::Context->PSSetSamplers(0, 1, &GBuffer::SamplerState);
		Window::Context->PSSetSamplers(2, 1, &ShadowMap::Sampler);

		for (const auto& material : scene->Materials)
		{
			Window::Context->PSSetShaderResources(0, 1, &material.Albedo);
			Window::Context->DrawIndexed(material.IndexCount, material.BaseIndex, material.BaseVertex);
		}

		Window::Context->RSSetState(nullptr);
		Window::Context->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 0, nullptr, nullptr);

		Window::Context->CSSetShader(m_CopyCS, nullptr, 0);
		Window::Context->CSSetConstantBuffers(1, 1, &ConstantBuffer);
		Window::Context->CSSetUnorderedAccessViews(0, 1, &m_VoxelView, nullptr);
		ID3D11ShaderResourceView* view = nullptr;
		Window::Context->CSSetShaderResources(3, 1, &view);
		Window::Context->CSSetUnorderedAccessViews(1, 1, &m_MidTextureView, nullptr);
		auto dimensions = uint32_t(m_CBuffer.VoxelGridRes) / 8 + 1;
		Window::Context->Dispatch(dimensions, dimensions, dimensions);

		Window::Context->CSSetShader(m_BounceCS, nullptr, 0);
		Window::Context->CSSetUnorderedAccessViews(1, 1, &m_DiffuseTextureView, nullptr);
		Window::Context->CSSetShaderResources(3, 1, &Voxel::m_MidReadView);
		Window::Context->CSSetSamplers(0, 1, &GBuffer::SamplerState);
		Window::Context->Dispatch(dimensions, dimensions, dimensions);

		ID3D11UnorderedAccessView* views[] = { nullptr, nullptr };
		Window::Context->CSSetUnorderedAccessViews(0, 2, views, nullptr);

		Window::Context->GenerateMips(DiffuseReadView);
	}

	void DrawDebug()
	{
		UINT offset = 0;
		Window::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		Window::Context->OMSetRenderTargets(1, &Window::RenderTarget, nullptr);
		Window::Context->OMSetDepthStencilState(nullptr, 0);
		Window::Context->VSSetShader(m_DebugVS, nullptr, 0);
		Window::Context->GSSetShader(m_DebugGS, nullptr, 0);
		Window::Context->PSSetShader(m_DebugPS, nullptr, 0);
		Window::Context->VSSetConstantBuffers(1, 1, &ConstantBuffer);
		Window::Context->VSSetShaderResources(3, 1, &DiffuseReadView);
		ID3D11Buffer* bufs[] = { ConstantBuffer, GBuffer::CameraBuffer };
		Window::Context->GSSetConstantBuffers(0, 1, &GBuffer::CameraBuffer);
		Window::Context->GSSetConstantBuffers(1, 1, &ConstantBuffer);
		Window::Context->IASetVertexBuffers(0, 0, nullptr, &offset, &offset);
		Window::Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		Window::Context->IASetInputLayout(nullptr);

		auto res = uint32_t(m_CBuffer.VoxelGridRes);
		Window::Context->Draw(res * res * res, 0);

		ID3D11ShaderResourceView* views[] = { nullptr };
		Window::Context->VSSetShaderResources(0, 1, views);
	}

	void Resize(uint32_t res, float halfExtent)
	{
		if (m_DiffuseTexture) m_DiffuseTexture->Release();
		if (DiffuseReadView) DiffuseReadView->Release();
		if (m_DiffuseTextureView) m_DiffuseTextureView->Release();
		if (m_MidTexture) m_MidTexture->Release();
		if (m_MidReadView) m_MidReadView->Release();
		if (m_MidTextureView) m_MidTextureView->Release();
		if (m_VoxelBuffer) m_VoxelBuffer->Release();
		if (m_VoxelView) m_VoxelView->Release();

		D3D11_TEXTURE3D_DESC desc{
			.Width = res,
			.Height = res,
			.Depth = res,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS,
			.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS
		};
		Window::Device->CreateTexture3D(&desc, nullptr, &m_DiffuseTexture);
		Window::Device->CreateTexture3D(&desc, nullptr, &m_MidTexture);

		D3D11_UNORDERED_ACCESS_VIEW_DESC utDesc{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D,
			.Texture3D = D3D11_TEX3D_UAV{.MipSlice = 0, .FirstWSlice = 0, .WSize = res }
		};
		Window::Device->CreateUnorderedAccessView(m_DiffuseTexture, &utDesc, &m_DiffuseTextureView);
		Window::Device->CreateUnorderedAccessView(m_MidTexture, &utDesc, &m_MidTextureView);

		D3D11_SHADER_RESOURCE_VIEW_DESC rDesc{
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D,
			.Texture3D = D3D11_TEX3D_SRV{.MostDetailedMip = 0, .MipLevels = UINT(-1) }
		};
		Window::Device->CreateShaderResourceView(m_DiffuseTexture, &rDesc, &DiffuseReadView);
		Window::Device->CreateShaderResourceView(m_MidTexture, &rDesc, &m_MidReadView);

		D3D11_BUFFER_DESC bDesc{
			.ByteWidth = 12 * res * res * res,
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_UNORDERED_ACCESS,
			.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
			.StructureByteStride = 12
		};
		Window::Device->CreateBuffer(&bDesc, nullptr, &m_VoxelBuffer);

		D3D11_UNORDERED_ACCESS_VIEW_DESC uDesc{
			.Format = DXGI_FORMAT_UNKNOWN,
			.ViewDimension = D3D11_UAV_DIMENSION_BUFFER,
			.Buffer = D3D11_BUFFER_UAV{.FirstElement = 0, .NumElements = res * res * res}
		};
		Window::Device->CreateUnorderedAccessView(m_VoxelBuffer, &uDesc, &m_VoxelView);

		m_Viewport.Width = float(res) * 2.f;
		m_Viewport.Height = float(res) * 2.f;
		m_Viewport.MinDepth = 0.f;
		m_Viewport.MaxDepth = 1.f;
		m_Viewport.TopLeftX = 0.f;
		m_Viewport.TopLeftY = 0.f;

		m_CBuffer.VoxelGridRes = float(res);
		m_CBuffer.VoxelHalfExtent = halfExtent;
	}

}
