#include <pch.h>
#include "GraphicsContext.h"
#include "Window.h"

int main()
{
	Nagi::Window win(1920, 1080);
	Nagi::GraphicsContext gfxCon(win);
	



	while (win.IsRunning())
	{
		win.ProcessEvents();
		
		auto[imageAvailableSem, renderFinishedSem, inFlightFence] = 
		gfxCon.BeginFrame();

		gfxCon.SubmitQueue({});

		gfxCon.EndFrame();
	}



	return 0;
}