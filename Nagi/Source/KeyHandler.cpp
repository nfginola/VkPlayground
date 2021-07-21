#include "pch.h"
#include "KeyHandler.h"
#include <GLFW/glfw3.h>


namespace Nagi
{
	KeyHandler::KeyHandler()
	{
	}

	void KeyHandler::handleKeyEvent(GLFWwindow* win, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_A)			handleKeyAction(KeyName::A, action);
		if (key == GLFW_KEY_D)			handleKeyAction(KeyName::D, action);
		if (key == GLFW_KEY_W)			handleKeyAction(KeyName::W, action);
		if (key == GLFW_KEY_S)			handleKeyAction(KeyName::S, action);
		if (key == GLFW_KEY_E)			handleKeyAction(KeyName::E, action);
		if (key == GLFW_KEY_Q)			handleKeyAction(KeyName::Q, action);
		if (key == GLFW_KEY_F)			handleKeyAction(KeyName::F, action);
		if (key == GLFW_KEY_G)			handleKeyAction(KeyName::G, action);
		if (key == GLFW_KEY_SPACE)		handleKeyAction(KeyName::Space, action);
		if (key == GLFW_KEY_LEFT_SHIFT) handleKeyAction(KeyName::LShift, action);
	}

	bool KeyHandler::isKeyDown(KeyName key)
	{
		return m_keyStates[key].isDown();
	}

	bool KeyHandler::isKeyPressed(KeyName key) 
	{
		return m_keyStates[key].justPressed();
	}

	void KeyHandler::handleKeyAction(KeyName key, int action)
	{
		if (action == GLFW_PRESS)
			m_keyStates[key].onPress();
		else if (action == GLFW_RELEASE)
			m_keyStates[key].onRelease();
	}

}

