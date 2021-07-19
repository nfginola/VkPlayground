#pragma once

struct GLFWwindow;

namespace Nagi
{

	class MouseHandler
	{
	public:
		MouseHandler() = default;
		~MouseHandler() = default;

		void handleCursor(GLFWwindow* win, double xPos, double yPos);
		void handleButton(GLFWwindow* win, int button, int action, int mods);

		float getDeltaX();
		float getDeltaY();
		float getPosX();
		float getPosY();

		void hookFunctionToCursor(std::function<void(float, float)> function);

	private:
		struct DeltaHelper
		{
			bool firstTime = true;
			double prevX = 0;
			double prevY = 0;
		};

	private:
		DeltaHelper m_deltaHelper;

		std::function<void(float, float)> m_cursorHook;
		
		float m_frameDeltaX;
		float m_frameDeltaY;
		float m_framePosX;
		float m_framePosY;
	
	};
}


