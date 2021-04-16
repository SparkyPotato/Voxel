#include <filesystem>

#include "Renderer/Renderer.h"

void RunLoop()
{
	MSG msg;
	for (;;)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) return;
		}

		float clear[] = { 0.f, 0.f, 0.f, 1.f };
		Window::Context->ClearRenderTargetView(Window::RenderTarget, clear);
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		Renderer::Update();

		ImGui::Render();
		Window::Context->OMSetRenderTargets(1, &Window::RenderTarget, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		Window::Swapchain->Present(0, 0);
	}
}

std::wstring GetPath(HINSTANCE hInstance)
{
	std::wstring result(MAX_PATH, 0);
	for (;;)
	{
		auto length = GetModuleFileNameW(nullptr, result.data(), DWORD(result.length()));
		if (length == 0)
		{
			return std::wstring();
		}

		if (length < result.length() - 1)
		{
			result.resize(length);
			result.shrink_to_fit();
			return result;
		}

		result.resize(result.length() * 2);
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nShowCmd)
{
	std::wstring path = GetPath(hInstance);
	std::filesystem::path execPath = path;
	std::filesystem::current_path(execPath.parent_path());

	Window::Create(hInstance);
	Renderer::Initialize();

	RunLoop();

	Renderer::Shutdown();
	Window::Destroy();

	return 0;
}
