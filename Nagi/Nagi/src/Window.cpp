#include <pch.h>
#include "Window.h"
#include <vulkan/vulkan.hpp>

namespace Nagi
{

Window::Window(int clientWidth, int clientHeight, const char* title) :
	m_window(nullptr),
	m_clientDimensions(clientWidth, clientHeight)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);		// False for now until we handle resizing

	m_window = glfwCreateWindow(clientWidth, clientHeight, title, nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
	glfwSetKeyCallback(m_window, KeyCallback);

	// Get required extensions
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	m_reqExtensions.reserve(glfwExtensionCount);
	for (uint32_t i = 0; i < glfwExtensionCount; ++i)
		m_reqExtensions.push_back(glfwExtensions[i]);
}

Window::~Window()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

uint32_t Window::GetClientWidth() const
{
	return static_cast<uint32_t>(m_clientDimensions.first);
}

uint32_t Window::GetClientHeight() const
{
	return static_cast<uint32_t>(m_clientDimensions.second);
}

bool Window::IsRunning() const
{
	return !glfwWindowShouldClose(m_window);
}

void Window::ProcessEvents() const
{
	glfwPollEvents();
}

void Window::SetResizeCallback(std::function<void(int, int)> function)
{
	m_resizeCallback = function;
}

void Window::SetKeyCallback(std::function<void(GLFWwindow*, int, int, int, int)> function)
{
	m_keyCallback = function;
}

vk::SurfaceKHR Window::GetSurface(const vk::Instance& vInst) const
{
	VkSurfaceKHR tmpSurface;
	assert(glfwCreateWindowSurface(vInst, m_window, nullptr, &tmpSurface) == VK_SUCCESS);

	vk::SurfaceKHR surface(tmpSurface);
	return surface;
}

const std::vector<const char*>& Window::GetRequiredExtensions() const 
{
	return m_reqExtensions;
}


void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	if (app->m_resizeCallback)
		app->m_resizeCallback(width, height);
}

void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	// Handle application specific responses
	auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	if (app->m_keyCallback)
		app->m_keyCallback(app->m_window, key, scancode, action, mods);
}

}


