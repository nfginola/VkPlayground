#pragma once

#include "Window.h"
#include "GraphicsContext.h"
#include "Utilities.h"
#include "SingleInstance.h"

// Important GLM defines
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // forces [0, 1] instead of [-1, -1] on persp matrix
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES  // make sure to align math types (for UBO --> there are alignment reqs)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Nagi
{

class Window;
class GraphicsContext;

class Application
{
public:
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
protected:
	Application() = delete;
	~Application() = default;
	Application(Window& window, GraphicsContext& gfxCon);

	// Default application behaviour that exists on all Applications should be here
	
protected:
	Window& m_window;
	GraphicsContext& m_gfxCon;

};
}



