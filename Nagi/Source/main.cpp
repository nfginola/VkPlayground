#include "pch.h"

#include "Application/SponzaApp.h"


int main()
{
	auto win = std::make_unique<Nagi::Window>(2464, 1386);
	auto vkCon = std::make_unique<Nagi::VulkanContext>(*win.get()); 

	Nagi::SponzaApp app(*win.get(), *vkCon.get());
	
	return 0;
}