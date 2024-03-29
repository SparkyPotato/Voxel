#pragma once

#include "Scene.h"

namespace Voxel 
{

	void Initialize();
	void Shutdown();

	void Voxelize(Scene* scene, DirectX::XMFLOAT3 cameraPosition);
	void DrawDebug();

	void Resize(uint32_t res, float halfExtent);

	extern ID3D11ShaderResourceView* DiffuseReadView;
	extern ID3D11Buffer* ConstantBuffer;

}
