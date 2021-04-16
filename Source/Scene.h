#pragma once

#include <vector>

#include <DirectXMath.h>

#include "Window.h"

struct Material
{
	uint32_t BaseIndex;
	uint32_t IndexCount;
	int32_t BaseVertex;
	ID3D11ShaderResourceView* Albedo;
	ID3D11ShaderResourceView* Specular;
	ID3D11ShaderResourceView* Bump;
	DirectX::XMFLOAT2 BumpMapSize;
};

struct Vertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 Tangent = {};
	DirectX::XMFLOAT3 Bitangent = {};
	DirectX::XMFLOAT2 UV = {};
};

class Scene
{
public:
	Scene() = default;
	Scene(const std::string& path);
	Scene(Scene&& other);
	Scene& operator=(Scene&& other);

	~Scene();

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
	std::vector<Material> Materials;
	float MaxLength;
};
