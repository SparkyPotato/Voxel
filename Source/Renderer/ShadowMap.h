#pragma once

#include "Scene.h"

struct Camera
{
	DirectX::XMVECTOR Position;
	DirectX::XMVECTOR Rotation;
};

struct Light
{
	DirectX::XMFLOAT3 Direction = { 0.f, 1.f, 0.f };
	float Intensity = 1.f;
	DirectX::XMFLOAT3 Color = { 1.f, 1.f, 1.f };
};

namespace ShadowMap
{
	void Initialize();
	void Shutdown();

	void SetLight(const Light& light);
	void Resize(uint32_t width);

	void Write(Scene* scene);

	extern ID3D11Buffer* LightBuffer;
	extern ID3D11Buffer* LightMatrix;
	extern ID3D11ShaderResourceView* ShadowMapView;
	extern ID3D11SamplerState* Sampler;
}
