#pragma once
#include "VulkanContext.h"


namespace Nagi
{


// Helpers for unchanging states in simple applications, not meant to be an all-round helper
// This Render Pass and Framebuffer is for Forward, single subpass with depth and no transparency.
namespace ezTmp
{
	//Deps:
	//gfxCon: getSwapchainImageFormat(), getDepthFormat(), getDevice(for resource creation)
	vk::UniqueRenderPass createDefaultRenderPass(VulkanContext& context);

	// Create framebuffers
	// resource deps: (1) sc view, (2) depth view (3) swapchain image count
	// Note: Can we remove these deps? sc image count dep may propagate to other per-frame resources.. (buffers to update, etc.)
	std::vector<vk::UniqueFramebuffer> createDefaultFramebuffers(VulkanContext& context, const vk::RenderPass& suitableRenderPass);


}


}
