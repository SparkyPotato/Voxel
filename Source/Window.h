#pragma once

#include <string>
#include <utility>

#include <d3d11_3.h>
#include <d3dcompiler.h>
#include <shobjidl.h>
#include <Windows.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

namespace Window
{
	void Create(HINSTANCE hInstance);
	void Destroy();

	std::string FileDialog();

	extern ID3D11RenderTargetView* RenderTarget;
	extern ID3D11Device3* Device;
	extern ID3D11DeviceContext* Context;
	extern IDXGISwapChain* Swapchain;
}
