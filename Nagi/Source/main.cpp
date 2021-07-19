#include "pch.h"

#include "Application/AppDefines.h"
#include "Application/MinimalTriangleApp.h"
#include "Application/MinimalTriangleWithBuffersApp.h"
#include "Application/QuadApp.h"
#include "Application/SponzaApp.h"

int main()
{
	auto win = std::make_unique<Nagi::Window>(2464, 1386);
	auto gfxCon = std::make_unique<Nagi::VulkanContext>(*win.get()); 

#if APP_MINIMAL_TRI
	Nagi::MinimalTriangleApp app(*win.get(), *gfxCon.get());
#elif APP_MINIMAL_TRI_BUFFER
	Nagi::MinimalTriangleWithBuffersApp app(*win.get(), *gfxCon.get());
#elif APP_QUAD
	Nagi::QuadApp app(*win.get(), *gfxCon.get());
#elif APP_SPONZA
	Nagi::SponzaApp app(*win.get(), *gfxCon.get());
#endif



	return 0;
}