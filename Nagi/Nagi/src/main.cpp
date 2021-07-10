#include "pch.h"

#include "GraphicsContext.h"
#include "Window.h"

#include "Application/MinimalTriangleApp.h"

int main()
{
	Nagi::Window win(1920, 1080);
	Nagi::GraphicsContext gfxCon(win);

	Nagi::MinimalTriangleApp app(win, gfxCon);




	return 0;
}