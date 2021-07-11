#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Singleton.h"

namespace vk
{
	class SurfaceKHR;
	class Instance;
}

namespace Nagi
{

class Window : Utils::Singleton<Window>
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
	void setResizeCallback(std::function<void(int, int)> function);
	void setKeyCallback(std::function<void(int, int, int, int)> function);
	vk::SurfaceKHR getSurface(const vk::Instance& vInst) const;
	const std::vector<const char*>& getRequiredExtensions() const;

	// GLFW callbacks which we re-route to our Window instance
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
	GLFWwindow* m_window;
	std::pair<int, int> m_clientDimensions;
	std::function<void(int, int)> m_resizeCallback;
	std::function<void(int, int, int, int)> m_keyCallback;
	//std::function<void(int, int)> m_cursorCallba
	std::vector<const char*> m_reqExtensions;
};


}
