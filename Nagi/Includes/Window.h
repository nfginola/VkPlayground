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

	void setResizeCallback(std::function<void(GLFWwindow*, int, int)> function);
	void setKeyCallback(std::function<void(GLFWwindow*, int, int, int, int)> function);
	void setMouseCursorCallback(std::function<void(GLFWwindow*, double, double)> function);
	void setMouseButtonCallback(std::function<void(GLFWwindow*, int, int, int)> function);

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
	std::function<void(GLFWwindow*, int, int, int, int)> m_keyCallback;
	std::function<void(GLFWwindow*, double, double)> m_mouseCursorCallback;
	std::function<void(GLFWwindow*, int, int, int)> m_mouseButtonCallback;
	std::vector<const char*> m_reqExtensions;
};


}
