#pragma once

#include "Window.h"
#include "VulkanContext.h"
#include "Utilities.h"
#include "VulkanTypes.h"
#include "VulkanUtilities.h"
#include "SingleInstance.h"

#include "KeyHandler.h"
#include "MouseHandler.h"


namespace Nagi
{

class Window;
class VulkanContext;

class Application
{
public:
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
protected:
	Application() = delete;
	~Application() = default;
	Application(Window& window, VulkanContext& gfxCon);

	// Default application behaviour that exists on all Applications should be here
	
protected:
	Window& m_window;
	VulkanContext& m_gfxCon;

};
}



