#include "Window.h"

#include <hidusage.h>

#include "Renderer/Renderer.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DXGI.lib")

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Window
{
	ID3D11Device3* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;
	IDXGISwapChain* Swapchain = nullptr;
	ID3D11RenderTargetView* RenderTarget = nullptr;

	IDXGIFactory1* GFactory = nullptr;
	HWND GWindow = nullptr;
	IFileOpenDialog* GDialog = nullptr;

	LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_CLOSE: PostMessage(hWnd, WM_QUIT, 0, 0); break;
		case WM_SIZE:
		{
			RenderTarget->Release();
			Swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

			ID3D11Texture2D* texture;
			Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&texture);
			Device->CreateRenderTargetView(texture, nullptr, &RenderTarget);
			texture->Release();

			Renderer::OnResize(LOWORD(lParam), HIWORD(lParam));
			break;
		}
		default: break;
		}

		ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	void Create(HINSTANCE hInstance)
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (FAILED(hr))
		{
			MessageBox(nullptr, L"Failed to initialize COM", L"Error", MB_OK | MB_ICONERROR);
			std::exit(1);
		}

		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&GDialog));
		if (FAILED(hr))
		{
			MessageBox(nullptr, L"Failed to initialize file dialog", L"Error", MB_OK | MB_ICONERROR);
			std::exit(1);
		}
		GDialog->SetTitle(L"Load Scene");
		GDialog->SetOkButtonLabel(L"Load");

		D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_1;
		D3D_FEATURE_LEVEL levelGot;
		auto createFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;

#ifndef NDEBUG
		createFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif
		ID3D11Device* device;
		hr = D3D11CreateDevice
		(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createFlags,
			&level,
			1,
			D3D11_SDK_VERSION,
			&device,
			&levelGot,
			&Context
		);
		if (FAILED(hr) || levelGot != D3D_FEATURE_LEVEL_11_1)
		{
			MessageBox(nullptr, L"Failed to initialize DirectX11", L"Error", MB_OK | MB_ICONERROR);
			std::exit(1);
		}
		hr = device->QueryInterface(__uuidof(ID3D11Device3), (void**)&Device);
		if (FAILED(hr))
		{
			MessageBox(nullptr, L"Failed to initialize DirectX11", L"Error", MB_OK | MB_ICONERROR);
			std::exit(1);
		}
		device->Release();

		hr = CreateDXGIFactory
		(
			__uuidof(IDXGIFactory),
			(void**)&GFactory
		);
		if (FAILED(hr))
		{
			MessageBox(nullptr, L"Failed to initialize DirectX11", L"Error", MB_OK | MB_ICONERROR);
			std::exit(1);
		}

		WNDCLASSEX wClass{
			.cbSize = sizeof(WNDCLASSEX),
			.style = CS_CLASSDC,
			.lpfnWndProc = &WindowProc,
			.hInstance = hInstance,
			.lpszClassName = L"Class"
		};

		auto a = RegisterClassEx(&wClass);

		GWindow = CreateWindow(L"Class", L"Voxel", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1600, 900,
			nullptr, nullptr, hInstance, nullptr);

		DXGI_SWAP_CHAIN_DESC desc{
			.BufferDesc = DXGI_MODE_DESC{
				.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
				.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
				.Scaling = DXGI_MODE_SCALING_UNSPECIFIED
			},
			.SampleDesc = DXGI_SAMPLE_DESC{
				.Count = 1,
				.Quality = 0
			},
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = 2,
			.OutputWindow = GWindow,
			.Windowed = TRUE,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
		};

		hr = GFactory->CreateSwapChain(Device, &desc, &Swapchain);
		if (FAILED(hr))
		{
			MessageBox(GWindow, L"Failed to initialize DirectX11", L"Error", MB_OK | MB_ICONERROR);
			std::exit(1);
		}

		ID3D11Texture2D* texture;
		Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&texture);
		Device->CreateRenderTargetView(texture, nullptr, &RenderTarget);
		texture->Release();

		D3D11_RENDER_TARGET_BLEND_DESC renderDesc;
		ZeroMemory(&renderDesc, sizeof(D3D11_RENDER_TARGET_BLEND_DESC));
		renderDesc.BlendEnable = FALSE;
		renderDesc.SrcBlend = D3D11_BLEND_ONE;
		renderDesc.DestBlend = D3D11_BLEND_SRC_COLOR;
		renderDesc.BlendOp = D3D11_BLEND_OP_ADD;
		renderDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
		renderDesc.DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		renderDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		renderDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		D3D11_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0] = renderDesc;

		ID3D11BlendState* blendState = nullptr;
		Device->CreateBlendState(&blendDesc, &blendState);
		Context->OMSetBlendState(blendState, nullptr, 0xffffffff);
		blendState->Release();

		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("../../Content/Roboto.ttf", 16.f);
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ShowWindow(GWindow, SW_SHOWDEFAULT);

		ImGui_ImplWin32_Init(GWindow);
		ImGui_ImplDX11_Init(Device, Context);
	}

	void Destroy()
	{
		ImGui_ImplWin32_Shutdown();
		ImGui_ImplDX11_Shutdown();
		ImGui::DestroyContext();

		Context->ClearState();
		Context->Flush();

		RenderTarget->Release();
		Swapchain->Release();
		GFactory->Release();
		Context->Release();
		Device->Release();

		DestroyWindow(GWindow);

		CoUninitialize();
	}

	std::string FileDialog()
	{
		HRESULT hr = GDialog->Show(nullptr);

		if (SUCCEEDED(hr))
		{
			IShellItem* item;
			hr = GDialog->GetResult(&item);

			if (SUCCEEDED(hr))
			{
				LPWSTR filePath;
				hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filePath);

				if (SUCCEEDED(hr))
				{
					std::wstring temp(filePath);
					int size_needed = WideCharToMultiByte(CP_UTF8, 0, temp.c_str(), -1, NULL, 0, NULL, NULL);
					std::string path(size_needed, 0);
					WideCharToMultiByte(CP_UTF8, 0, temp.c_str(), (int)temp.size(), &path[0], size_needed, NULL, NULL);

					return path;
				}
				else
				{
					MessageBox(GWindow, L"Failed to get path", L"Error", MB_OK | MB_ICONERROR);
				}

				item->Release();
			}
			else
			{
				MessageBox(GWindow, L"Failed to get selected file", L"Error", MB_OK | MB_ICONERROR);
			}
		}

		return "";
	}
}
