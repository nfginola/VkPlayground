#pragma once

#include "Window.h"
#include "VulkanContext.h"
#include "Utilities.h"
#include "VulkanUtilities.h"
#include "ResourceTypes.h"
#include "SingleInstance.h"

#include "Input/KeyHandler.h"
#include "Input/MouseHandler.h"



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

protected:
	Window& m_window;
	VulkanContext& m_gfxCon;

};
}



