#pragma once

#include "Scene.h"

namespace GBuffer
{

	void Initialize();
	void Shutdown();

	void SetDebugMode(uint32_t mode);
	void SetViewMatrix(const DirectX::XMMATRIX& matrix);

	void Write(Scene* scene);
	void DrawDebug();

	void Resize(uint32_t width, uint32_t height);

	extern ID3D11Buffer* CameraBuffer;
	extern ID3D11InputLayout* Layout;
	extern ID3D11ShaderResourceView* Views[3];
	extern ID3D11VertexShader* ReadVS;
	extern D3D11_VIEWPORT Viewport;
	extern ID3D11SamplerState* SamplerState;

}
