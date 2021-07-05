#include <pch.h>
#include "GraphicsContext.h"
#include "Window.h"

int main()
{
	Nagi::Window win(1920, 1080);
	
	Nagi::GraphicsContext gphCon(win);


	while (win.IsRunning())
	{
		win.ProcessEvents();



	}



	return 0;
}