#include "pch.h"

#include "Application/AppDefines.h"
#include "Application/MinimalTriangleApp.h"
#include "Application/MinimalTriangleWithBuffersApp.h"
#include "Application/QuadApp.h"

int main()
{
	Nagi::Window win(1920, 1080);
	Nagi::VulkanContext gfxCon(win);

#if APP_MINIMAL_TRI
	Nagi::MinimalTriangleApp app(win, gfxCon);
#elif APP_MINIMAL_TRI_BUFFER
	Nagi::MinimalTriangleWithBuffersApp app(win, gfxCon);
#elif APP_QUAD
	Nagi::QuadApp app(win, gfxCon);
#endif

	return 0;
}