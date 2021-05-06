#include "Finalizer.h"

#include "GBuffer.h"
#include "Voxel.h"
#include "ShadowMap.h"

namespace Finalizer
{

	ID3D11PixelShader* m_Shader = nullptr;

	void Initialize()
	{
		ID3DBlob* blob;
		D3DReadFileToBlob(L"FinalizerPS.cso", &blob);
		Window::Device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_Shader);
	}

	void Shutdown()
	{
		m_Shader->Release();
	}

	void Draw()
	{
		Window::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		UINT offset = 0;
		Window::Context->OMSetRenderTargets(1, &Window::RenderTarget, nullptr);
		Window::Context->OMSetDepthStencilState(nullptr, 0);
		Window::Context->VSSetShader(GBuffer::ReadVS, nullptr, 0);
		Window::Context->GSSetShader(nullptr, nullptr, 0);
		Window::Context->PSSetShader(m_Shader, nullptr, 0);
		Window::Context->PSSetConstantBuffers(0, 1, &GBuffer::CameraBuffer);
		Window::Context->PSSetConstantBuffers(1, 1, &Voxel::ConstantBuffer);
		Window::Context->PSSetConstantBuffers(2, 1, &ShadowMap::LightBuffer);
		Window::Context->PSSetConstantBuffers(3, 1, &ShadowMap::LightMatrix);
		Window::Context->PSSetShaderResources(0, 3, GBuffer::Views);
		Window::Context->PSSetShaderResources(3, 1, &Voxel::DiffuseReadView);
		Window::Context->PSSetShaderResources(4, 1, &ShadowMap::ShadowMapView);
		Window::Context->PSSetSamplers(0, 1, &GBuffer::SamplerState);
		Window::Context->PSSetSamplers(2, 1, &ShadowMap::Sampler);
		Window::Context->IASetVertexBuffers(0, 0, nullptr, &offset, &offset);
		Window::Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		Window::Context->IASetInputLayout(nullptr);

		Window::Context->Draw(3, 0);

		ID3D11ShaderResourceView* views[] = { nullptr, nullptr, nullptr, nullptr };
		Window::Context->PSSetShaderResources(0, 4, views);
	}

}