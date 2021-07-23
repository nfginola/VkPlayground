#include "pch.h"
#include "Input/KeyHandler.h"
#include <GLFW/glfw3.h>


namespace Nagi
{
	KeyHandler::KeyHandler()
	{
	}

	void KeyHandler::handleKeyEvent(GLFWwindow* win, int key, int scancode, int action, int mods)
	{
		// Perhaps we can add an intermediary between GLFW and KeyHandler. 
		// This way, only the intermediary knows about GLFW and KeyHandler while KeyHandler doesn't have to know about GLFW
		// but it will add complexity since we would have to maintain the intermediary in the case of added functionality
		// Save it for sometime later maybe
		if (key == GLFW_KEY_Q)			handleKeyAction(KeyName::Q, action);
		if (key == GLFW_KEY_W)			handleKeyAction(KeyName::W, action);
		if (key == GLFW_KEY_E)			handleKeyAction(KeyName::E, action);
		if (key == GLFW_KEY_R)			handleKeyAction(KeyName::R, action);
		if (key == GLFW_KEY_T)			handleKeyAction(KeyName::T, action);
		if (key == GLFW_KEY_Y)			handleKeyAction(KeyName::Y, action);
		if (key == GLFW_KEY_U)			handleKeyAction(KeyName::U, action);
		if (key == GLFW_KEY_I)			handleKeyAction(KeyName::I, action);
		if (key == GLFW_KEY_O)			handleKeyAction(KeyName::O, action);
		if (key == GLFW_KEY_P)			handleKeyAction(KeyName::P, action);

		if (key == GLFW_KEY_A)			handleKeyAction(KeyName::A, action);
		if (key == GLFW_KEY_S)			handleKeyAction(KeyName::S, action);
		if (key == GLFW_KEY_D)			handleKeyAction(KeyName::D, action);
		if (key == GLFW_KEY_F)			handleKeyAction(KeyName::F, action);
		if (key == GLFW_KEY_G)			handleKeyAction(KeyName::G, action);
		if (key == GLFW_KEY_H)			handleKeyAction(KeyName::H, action);
		if (key == GLFW_KEY_J)			handleKeyAction(KeyName::J, action);
		if (key == GLFW_KEY_K)			handleKeyAction(KeyName::K, action);
		if (key == GLFW_KEY_L)			handleKeyAction(KeyName::L, action);

		if (key == GLFW_KEY_Z)			handleKeyAction(KeyName::Z, action);
		if (key == GLFW_KEY_X)			handleKeyAction(KeyName::X, action);
		if (key == GLFW_KEY_C)			handleKeyAction(KeyName::C, action);
		if (key == GLFW_KEY_V)			handleKeyAction(KeyName::V, action);
		if (key == GLFW_KEY_B)			handleKeyAction(KeyName::B, action);
		if (key == GLFW_KEY_N)			handleKeyAction(KeyName::N, action);
		if (key == GLFW_KEY_M)			handleKeyAction(KeyName::M, action);

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

