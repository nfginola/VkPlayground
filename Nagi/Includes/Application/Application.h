#pragma once

#include "Window.h"
#include "VulkanContext.h"
#include "Utilities.h"
#include "VulkanTypes.h"
#include "VulkanUtilities.h"
#include "SingleInstance.h"




namespace Nagi
{

class Window;
class VulkanContext;

// Handle key down / just pressed
class Keystate
{
public:
	// Called on GLFW key down
	void onPress()
	{
		m_isDown = true;
		if (m_justPressed)
			m_justPressed = false;
		else
			m_justPressed = true;
	};

	// Called on GLFW key release
	void onRelease()
	{
		m_isDown = false;
		m_justPressed = false;
	};

	bool isDown()
	{
		return m_isDown;
	};

	bool justPressed()
	{
		// Turns off isPressed after the first time its retrieved 
		bool ret = m_justPressed;
		m_justPressed = false;
		return ret;
	};

private:
	bool m_justPressed = false;
	bool m_isDown = false;
};

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



