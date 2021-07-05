#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vk
{
	class SurfaceKHR;
	class Instance;
}

namespace Nagi
{

class Window
{
public:
	Window(int width, int height, const char* title = "Nagi Engine");
	~Window();

	Window() = delete;
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	uint32_t GetClientWidth() const;
	uint32_t GetClientHeight() const;
	bool IsRunning() const;
	void ProcessEvents() const;
	void SetResizeCallback(std::function<void(int, int)> function);
	void SetKeyCallback(std::function<void(GLFWwindow*, int, int, int, int)> function);
	vk::SurfaceKHR GetSurface(const vk::Instance& vInst) const;
	const std::vector<const char*>& GetRequiredExtensions() const;

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
	GLFWwindow* m_window;
	std::pair<int, int> m_clientDimensions;
	std::function<void(int, int)> m_resizeCallback;
	std::function<void(GLFWwindow*, int, int, int, int)> m_keyCallback;
	std::vector<const char*> m_reqExtensions;
};


}
