#include <pch.h>
#include "GraphicsContext.h"
#include "Window.h"

int main()
{
	Nagi::Window win(1920, 1080);
	
	Nagi::GraphicsContext gfxCon(win);

	
	auto sems = gfxCon.BeginFrame();

	while (win.IsRunning())
	{
		win.ProcessEvents();



	}



	return 0;
}