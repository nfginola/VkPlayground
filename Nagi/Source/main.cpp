#include "pch.h"

#include "Application/SponzaApp.h"



int main()
{
	auto keyHandler = std::make_unique<Nagi::KeyHandler>();
	auto mouseHandler = std::make_unique<Nagi::MouseHandler>();

	auto win = std::make_unique<Nagi::Window>(2464, 1386);
	//auto win = std::make_unique<Nagi::Window>(1920, 1080);
	win->setKeyHandler(keyHandler.get());
	win->setMouseHandler(mouseHandler.get());

	auto gfxCon = std::make_unique<Nagi::VulkanContext>(*win.get()); 





	Nagi::SponzaApp app(*win.get(), *gfxCon.get());

	return 0;
}