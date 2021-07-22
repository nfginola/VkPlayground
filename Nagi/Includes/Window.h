#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "SingleInstance.h"

namespace vk
{
	class SurfaceKHR;
	class Instance;
}

namespace Nagi
{

	class KeyHandler;
	class MouseHandler;

class Window : SingleInstance<Window>
{
public:
	Window(int width, int height, const char* title = "Nagi Engine");
	~Window();

	Window() = delete;
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;


	uint32_t getClientWidth() const;
	uint32_t getClientHeight() const;
	bool isRunning() const;
	void processEvents() const;

	void setKeyHandler(KeyHandler* handler);
	void setMouseHandler(MouseHandler* handler);
	KeyHandler* getKeyHandler() const;
	MouseHandler* getMouseHandler() const;

	void setResizeCallback(std::function<void(GLFWwindow*, int, int)> function);

	// Vulkan helpers
	vk::SurfaceKHR getSurface(const vk::Instance& vInst) const;
	const std::vector<const char*>& getRequiredExtensions() const;

	// GLFW callbacks which we re-route to our Window instance
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseCursorCallback(GLFWwindow* window, double xpos, double ypos);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);


private:
	GLFWwindow* m_window;
	std::pair<int, int> m_clientDimensions;
	std::function<void(GLFWwindow*, int, int)> m_resizeCallback;
	
	KeyHandler* m_keyHandler;
	MouseHandler* m_mouseHandler;

	std::vector<const char*> m_reqExtensions;

	friend class VulkanImGuiContext;
};


}
