#include "pch.h"
#include "MouseHandler.h"
#include <GLFW/glfw3.h>

namespace Nagi
{
	void MouseHandler::handleCursor(GLFWwindow* win, double xPos, double yPos)
	{
		if (glfwGetInputMode(win, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
		{
			if (m_deltaHelper.firstTime)
			{
				m_deltaHelper.prevX = xPos;
				m_deltaHelper.prevY = yPos;
				m_deltaHelper.firstTime = false;
			}

			double dx = xPos - m_deltaHelper.prevX;
			double dy = -(yPos - m_deltaHelper.prevY);	// Down is positive in Screenspace, we flip it

			m_frameDeltaX = static_cast<float>(dx);
			m_frameDeltaY = static_cast<float>(dy);

			//std::cout << "deltaX: " << m_frameDeltaX << "\n";
			//std::cout << "deltaY: " << m_frameDeltaY << "\n";

			if (m_cursorHook)
				m_cursorHook(m_frameDeltaX, m_frameDeltaY);

			m_deltaHelper.prevX = xPos;
			m_deltaHelper.prevY = yPos;
		}
		else
		{
			m_deltaHelper.prevX = xPos;
			m_deltaHelper.prevY = yPos;
		}

		m_framePosX = static_cast<float>(xPos);
		m_framePosY = static_cast<float>(yPos);

	}

	void MouseHandler::handleButton(GLFWwindow* win, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	}

	float MouseHandler::getDeltaX()
	{
		// Reset delta, or it will stick. 
		// When no movement --> No callback --> No prevX/Y == currX/Y
		auto ret = m_frameDeltaX;
		m_frameDeltaX = 0.f;
		return ret;
	}
	float MouseHandler::getDeltaY()
	{
		auto ret = m_frameDeltaY;
		m_frameDeltaY = 0.f;
		return ret;
	}

	float MouseHandler::getPosX()
	{
		return m_framePosX;
	}

	float MouseHandler::getPosY()
	{
		return m_framePosY;
	}

	void MouseHandler::hookFunctionToCursor(std::function<void(float, float)> function)
	{
		m_cursorHook = function;
	}


}
