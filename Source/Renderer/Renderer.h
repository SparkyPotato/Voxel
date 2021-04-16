#pragma once

#include <vector>

#include "Scene.h"

namespace Renderer
{
	void Initialize();
	void Shutdown();

	void Update();
	void OnResize(uint32_t width, uint32_t height);
}
