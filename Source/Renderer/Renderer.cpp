#include "Renderer.h"

#include <chrono>
#define _USE_MATH_DEFINES
#include <math.h>

#include "GBuffer.h"
#include "Voxel.h"
#include "Finalizer.h"

namespace Renderer
{
	std::string m_CurrentScenePath;
	Scene m_CurrentScene;

	struct Camera
	{
		DirectX::XMVECTOR Position;
		DirectX::XMVECTOR Rotation;
	};
	Camera m_Camera;

	struct LightData
	{
		DirectX::XMFLOAT3 Direction = { 0.f, 1.f, 0.f };
		float Intensity = 1.5f;
		DirectX::XMFLOAT3 Color = { 1.f, 1.f, 1.f };
	};
	LightData m_Light;

	void Initialize()
	{
		GBuffer::Initialize();
		Voxel::Initialize();
		Voxel::Resize(128, 16.f);
		Voxel::SetLightData(m_Light.Direction, m_Light.Intensity, m_Light.Color);
		Finalizer::Initialize();

		m_Camera.Position = DirectX::XMVectorSet(0.f, 0.f, 0.f, 1.f);
		m_Camera.Rotation = DirectX::XMVectorSet(0.f, 0.f, 0.f, 1.f);
	}

	void Shutdown()
	{
		Finalizer::Shutdown();
		Voxel::Shutdown();
		GBuffer::Shutdown();
	}

	void UpdateInput(float deltaTime)
	{
		using namespace DirectX;
		static float moveSpeed = 500.f;

		float min = 1.f;
		ImGui::DragScalar("Move Speed", ImGuiDataType_Float, &moveSpeed, 5.f, &min);

		XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYawFromVector(-m_Camera.Rotation);

		if (GetAsyncKeyState('W'))
			m_Camera.Position += XMVector3Transform(XMVectorSet(0.f, 0.f, 1.f, 0.f), rotation) * deltaTime * moveSpeed;
		if (GetAsyncKeyState('S'))
			m_Camera.Position -= XMVector3Transform(XMVectorSet(0.f, 0.f, 1.f, 0.f), rotation) * deltaTime * moveSpeed;
		if (GetAsyncKeyState('A'))
			m_Camera.Position -= XMVector3Transform(XMVectorSet(1.f, 0.f, 0.f, 0.f), rotation) * deltaTime * moveSpeed;
		if (GetAsyncKeyState('D'))
			m_Camera.Position += XMVector3Transform(XMVectorSet(1.f, 0.f, 0.f, 0.f), rotation) * deltaTime * moveSpeed;
		if (GetAsyncKeyState('Q'))
			m_Camera.Position -= XMVector3Transform(XMVectorSet(0.f, 1.f, 0.f, 0.f), rotation) * deltaTime * moveSpeed;
		if (GetAsyncKeyState('E'))
			m_Camera.Position += XMVector3Transform(XMVectorSet(0.f, 1.f, 0.f, 0.f), rotation) * deltaTime * moveSpeed;

		XMMATRIX position = DirectX::XMMatrixTranslationFromVector(-m_Camera.Position);

		XMMATRIX view = position * rotation;
		GBuffer::SetViewMatrix(view);
	}

	void DrawLight()
	{
		bool change = false;
		if (ImGui::ColorEdit3("Light Color", &m_Light.Color.x))
		{
			change = true;
		}

		static float angle[3];
		if (ImGui::DragFloat3("Light Direction", angle, 1.f))
		{
			using namespace DirectX;
			XMMATRIX rotation = XMMatrixRotationRollPitchYaw(angle[0] * M_PI / 180.f, angle[1] * M_PI / 180.f, angle[2] * M_PI / 180.f);
			XMVECTOR vec = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);
			XMStoreFloat3(&m_Light.Direction, XMVector3Transform(vec, rotation));

			change = true;
		}

		if (ImGui::DragFloat("Light Intensity", &m_Light.Intensity, 0.2f))
		{
			change = true;
		}

		if (change)
		{
			Voxel::SetLightData(m_Light.Direction, m_Light.Intensity, m_Light.Color);
		}
	}

	void Update()
	{
		static std::chrono::high_resolution_clock::time_point lastTickTime = std::chrono::high_resolution_clock::now();
		static bool selected[] = { true, false, false, false, false, false };

		float deltaTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - lastTickTime).count();

		if (ImGui::Begin("Tools"))
		{
			ImGui::Text("Delta Time: %.1fms", deltaTime * 1000.f);
			ImGui::Text("FPS: %.3f", 1 / deltaTime);
			if (ImGui::Button("Load"))
			{
				m_CurrentScenePath = Window::FileDialog();
				if (m_CurrentScenePath.size())
				{
					try { m_CurrentScene = Scene(m_CurrentScenePath); }
					catch (const std::exception& e)
					{
						MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
					}
				}
			}
			ImGui::Text("%s", m_CurrentScenePath.c_str());

			DrawLight();

			if (ImGui::ListBoxHeader("Debug", 4))
			{
				if (ImGui::RadioButton("Final", selected[0]))
				{
					selected[0] = true;
					selected[1] = false;
					selected[2] = false;
					selected[3] = false;
					selected[4] = false;
					selected[5] = false;
				}
				else if (ImGui::RadioButton("Albedo", selected[1]))
				{
					selected[0] = false;
					selected[1] = true;
					selected[2] = false;
					selected[3] = false;
					selected[4] = false;
					selected[5] = false;

					GBuffer::SetDebugMode(0);
				}
				else if (ImGui::RadioButton("Specular", selected[2]))
				{
					selected[0] = false;
					selected[1] = false;
					selected[2] = true;
					selected[3] = false;
					selected[4] = false;
					selected[5] = false;

					GBuffer::SetDebugMode(1);
				}
				else if (ImGui::RadioButton("Normal", selected[3]))
				{
					selected[0] = false;
					selected[1] = false;
					selected[2] = false;
					selected[3] = true;
					selected[4] = false;
					selected[5] = false;

					GBuffer::SetDebugMode(2);
				}
				else if (ImGui::RadioButton("Depth", selected[4]))
				{
					selected[0] = false;
					selected[1] = false;
					selected[2] = false;
					selected[3] = false;
					selected[4] = true;
					selected[5] = false;

					GBuffer::SetDebugMode(3);
				}
				else if (ImGui::RadioButton("Voxel", selected[5]))
				{
					selected[0] = false;
					selected[1] = false;
					selected[2] = false;
					selected[3] = false;
					selected[4] = false;
					selected[5] = true;
				}
	
				ImGui::ListBoxFooter();
			}
		}

		UpdateInput(deltaTime);

		DirectX::XMFLOAT3 pos;
		DirectX::XMStoreFloat3(&pos, m_Camera.Position);
		Voxel::Voxelize(&m_CurrentScene, pos);
		GBuffer::Write(&m_CurrentScene);

		if (selected[0])
		{
			Finalizer::Draw();
		}
		else if (selected[5])
		{
			Voxel::DrawDebug();
		}
		else
		{
			GBuffer::DrawDebug();
		}

		ImGui::End();

		lastTickTime = std::chrono::high_resolution_clock::now();
	}

	void OnResize(uint32_t width, uint32_t height)
	{
		GBuffer::Resize(width, height);
	}
}
