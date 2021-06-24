#include "Scene.h"

#undef max
#undef min
#include <algorithm>
#include <filesystem>
#include <iterator>
#include <set>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#define STBI_WINDOWS_UTF8
#include "stb_image.h"

struct Model
{
	std::vector<Vertex> Vertices;
	std::vector<uint32_t> Indices;
	uint32_t MaterialID;
};

struct Texture
{
	void* Data;
	uint32_t Width, Height;

	static Texture Load(const std::string& path)
	{
		int width, height, comp;
		uint8_t* data = stbi_load(path.c_str(), &width, &height, &comp, 4);
		return { data, uint32_t(width), uint32_t(height) };
	}
};

bool operator<(const Model& first, const Model& second)
{
	return first.MaterialID < second.MaterialID;
}

Scene::Scene(const std::string& path)
{
	Assimp::Importer importer;
	importer.ReadFile(path, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_OptimizeGraph |
		aiProcess_ConvertToLeftHanded | aiProcess_TransformUVCoords | aiProcess_GenUVCoords |
		aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals |
		aiProcess_Triangulate);

	auto scene = importer.GetScene();
	if (!scene)
	{
		throw std::exception("Invalid scene file");
	}
	std::multiset<Model> models;

	for (uint32_t i = 0; i < scene->mNumMeshes; i++)
	{
		Model model;
		aiMesh* mesh = scene->mMeshes[i];
		
		if (!mesh->HasNormals())
		{
			throw std::exception("Mesh does not have normals");
		}

		model.Vertices.reserve(mesh->mNumVertices);
		for (uint32_t j = 0; j < mesh->mNumVertices; j++)
		{
			Vertex vertex;
			vertex.Position = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z };
			vertex.Normal = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z };
			if (mesh->HasTangentsAndBitangents())
			{
				vertex.Tangent = { mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z };
				vertex.Bitangent = { mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z };
			}
			if (mesh->HasTextureCoords(0))
			{
				vertex.UV = { mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y };
			}
			model.Vertices.push_back(vertex);
		}

		model.Indices.reserve(mesh->mNumFaces * 3);
		for (uint32_t j = 0; j < mesh->mNumFaces; j++)
		{
			if (mesh->mFaces[j].mNumIndices == 3)
			{
				model.Indices.push_back(mesh->mFaces[j].mIndices[0]);
				model.Indices.push_back(mesh->mFaces[j].mIndices[1]);
				model.Indices.push_back(mesh->mFaces[j].mIndices[2]);
			}
		}

		model.MaterialID = mesh->mMaterialIndex;

		models.emplace(std::move(model));
	}

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	uint32_t baseVertex = 0;
	uint32_t baseIndex = 0;
	for (auto it = models.begin(); it != models.end();)
	{
		uint32_t lastMatID;
		uint32_t maxIndex = 0;
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		do
		{
			lastMatID = it->MaterialID;

			vertices.insert(vertices.end(), it->Vertices.begin(), it->Vertices.end());
			indices.reserve(it->Indices.size());
			uint32_t currMaxIndex = 0;
			std::transform(it->Indices.begin(), it->Indices.end(), std::back_inserter(indices), [&](uint32_t index)
				{
					currMaxIndex = std::max(currMaxIndex, index);
					return index + maxIndex;
				});
			maxIndex += currMaxIndex;
			indexCount += uint32_t(it->Indices.size());
			vertexCount += uint32_t(it->Vertices.size());

			it++;
		} while (it != models.end() && it->MaterialID == lastMatID);

		Material material;
		material.BaseVertex = baseVertex;
		material.BaseIndex = baseIndex;
		material.IndexCount = indexCount;

		aiString texPath;
		std::string basePath = std::filesystem::path(path).parent_path().string() + "/";
		aiMaterial* mat = scene->mMaterials[lastMatID];

		Texture albedo;
		Texture specular;
		Texture bump;
		bool albedoTex = false;
		bool specularTex = false;
		bool bumpTex = false;
		uint8_t albedoColor[4];
		uint8_t specularColor[4];
		uint8_t bumpColor[4];

		if (mat->GetTextureCount(aiTextureType_DIFFUSE))
		{
			albedoTex = true;
			mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
			albedo = Texture::Load(basePath + texPath.C_Str());
		}
		else
		{
			aiColor3D color;
			mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			albedoColor[0] = color.r * 255;
			albedoColor[1] = color.g * 255;
			albedoColor[2] = color.b * 255;
			albedoColor[3] = 255;

			albedo.Data = albedoColor;
			albedo.Width = albedo.Height = 1;
		}

		if (mat->GetTextureCount(aiTextureType_AMBIENT))
		{
			specularTex = true;
			mat->GetTexture(aiTextureType_AMBIENT, 0, &texPath);
			specular = Texture::Load(basePath + texPath.C_Str());
		}
		else
		{
			aiColor3D color;
			mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
			specularColor[0] = color.r * 255;
			specularColor[1] = color.g * 255;
			specularColor[2] = color.b * 255;
			specularColor[3] = 255;

			specular.Data = specularColor;
			specular.Width = specular.Height = 1;
		}

		if (mat->GetTextureCount(aiTextureType_HEIGHT))
		{
			bumpTex = true;
			mat->GetTexture(aiTextureType_HEIGHT, 0, &texPath);
			bump = Texture::Load(basePath + texPath.C_Str());
		}
		else
		{
			bumpColor[0] = 127;
			bumpColor[1] = 127;
			bumpColor[2] = 127;
			bumpColor[3] = 255;

			bump.Data = bumpColor;
			bump.Width = bump.Height = 1;
		}

		D3D11_TEXTURE2D_DESC desc{
			.Width = albedo.Width,
			.Height = albedo.Height,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			.SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0 },
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_SHADER_RESOURCE
		};
		D3D11_SUBRESOURCE_DATA data{
			.pSysMem = albedo.Data,
			.SysMemPitch = albedo.Width * sizeof(uint32_t)
		};

		ID3D11Texture2D* tex;
		Window::Device->CreateTexture2D(&desc, &data, &tex);
		Window::Device->CreateShaderResourceView(tex, nullptr, &material.Albedo);
		tex->Release();

		desc.Width = specular.Width;
		desc.Height = specular.Height;
		data.pSysMem = specular.Data;
		data.SysMemPitch = specular.Width * sizeof(uint32_t);
		Window::Device->CreateTexture2D(&desc, &data, &tex);
		Window::Device->CreateShaderResourceView(tex, nullptr, &material.Specular);
		tex->Release();

		desc.Width = bump.Width;
		desc.Height = bump.Height;
		data.pSysMem = bump.Data;
		data.SysMemPitch = bump.Width * sizeof(uint32_t);
		Window::Device->CreateTexture2D(&desc, &data, &tex);
		Window::Device->CreateShaderResourceView(tex, nullptr, &material.Bump);
		tex->Release();

		material.BumpMapSize = { float(bump.Width), float(bump.Height) };

		if (albedoTex) stbi_image_free(albedo.Data);
		if (specularTex) stbi_image_free(specular.Data);
		if (bumpTex) stbi_image_free(bump.Data);

		Materials.emplace_back(material);

		baseVertex += vertexCount;
		baseIndex += indexCount;
	}

	D3D11_BUFFER_DESC desc{
		.ByteWidth = uint32_t(vertices.size() * sizeof(Vertex)),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER
	};
	D3D11_SUBRESOURCE_DATA data{
		.pSysMem = vertices.data()
	};
	Window::Device->CreateBuffer(&desc, &data, &VertexBuffer);
	desc.ByteWidth = uint32_t(indices.size() * sizeof(uint32_t));
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	data.pSysMem = indices.data();
	Window::Device->CreateBuffer(&desc, &data, &IndexBuffer);
}

Scene::Scene(Scene&& other)
{
	VertexBuffer = other.VertexBuffer;
	other.VertexBuffer = nullptr;
	IndexBuffer = other.IndexBuffer;
	other.IndexBuffer = nullptr;
	Materials = std::move(other.Materials);
}

Scene& Scene::operator=(Scene&& other)
{
	this->~Scene();

	VertexBuffer = other.VertexBuffer;
	other.VertexBuffer = nullptr;
	IndexBuffer = other.IndexBuffer;
	other.IndexBuffer = nullptr;
	Materials = std::move(other.Materials);

	return *this;
}

Scene::~Scene()
{
	if (VertexBuffer)
	{
		VertexBuffer->Release();
		IndexBuffer->Release();
	}

	for (const auto& material : Materials)
	{
		material.Albedo->Release();
		material.Specular->Release();
		material.Bump->Release();
	}
}
