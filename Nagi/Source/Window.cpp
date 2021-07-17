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
	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
	glfwSetKeyCallback(m_window, keyCallback);

	glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
	glfwSetCursorPosCallback(m_window, mouseCursorCallback);



	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);





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

uint32_t Window::getClientWidth() const
{
	return static_cast<uint32_t>(m_clientDimensions.first);
}

uint32_t Window::getClientHeight() const
{
	return static_cast<uint32_t>(m_clientDimensions.second);
}

bool Window::isRunning() const
{
	return !glfwWindowShouldClose(m_window);
}

void Window::processEvents() const
{
	glfwPollEvents();
}

void Window::setResizeCallback(std::function<void(GLFWwindow*, int, int)> function)
{
	m_resizeCallback = function;
}

void Window::setKeyCallback(std::function<void(GLFWwindow*, int, int, int, int)> function)
{
	m_keyCallback = function;
}

void Window::setMouseCursorCallback(std::function<void(GLFWwindow*, int, int)> function)
{
	m_mouseCursorCallback = function;
}

void Window::setMouseButtonCallback(std::function<void(GLFWwindow*, int, int, int)> function)
{
	m_mouseButtonCallback = function;
}

vk::SurfaceKHR Window::getSurface(const vk::Instance& vInst) const
{
	VkSurfaceKHR tmpSurface = VK_NULL_HANDLE;
	auto res = glfwCreateWindowSurface(vInst, m_window, nullptr, &tmpSurface);
	if (res != VK_SUCCESS)
		throw std::runtime_error("glfw: Could not create window surface");

	return vk::SurfaceKHR(tmpSurface);
}

const std::vector<const char*>& Window::getRequiredExtensions() const 
{
	return m_reqExtensions;
}


void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	if (app->m_resizeCallback)
		app->m_resizeCallback(window, width, height);
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	// Handle application specific responses
	auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	if (app->m_keyCallback)
		app->m_keyCallback(window, key, scancode, action, mods);
}

void Window::mouseCursorCallback(GLFWwindow* window, double xPos, double yPos)
{
	auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	if (app->m_mouseCursorCallback)
	{
		app->m_mouseCursorCallback(window, xPos, yPos);	
	}

}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{


	auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	if (app->m_mouseButtonCallback)
		app->m_mouseButtonCallback(window, button, action, mods);
}

}


