#pragma once
#include <unordered_map>

struct GLFWwindow;

namespace Nagi
{

	enum class KeyName
	{
		Q, W, E, R, T, Y, U, I, O, P,
		A, S, D, F, G, H, J, K, L,
		Z, X, C, V, B, N, M,

		LShift, Space
	};

	class Keyboard
	{
	public:
		Keyboard();
		~Keyboard() = default;
		
		// Handles GLFW explicitly
		void handleKeyEvent(GLFWwindow* win, int key, int scancode, int action, int mods);		// --> Appends commands to commandsThisFrame

		bool isKeyDown(KeyName key);
		bool isKeyPressed(KeyName key);
	
	private:
		class KeyState
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
		void handleKeyAction(KeyName key, int action);

	private:
		std::unordered_map<KeyName, KeyState> m_keyStates;


	};

}


