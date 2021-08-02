#include "pch.h"
#include "VulkanUtilities.h"

namespace Nagi
{

namespace ezTmp
{

vk::UniqueRenderPass createDefaultRenderPass(VulkanContext& context)
{
	auto dev = context.getDevice();

	std::array<vk::AttachmentDescription, 2> attachmentDescs;

	// Color
	attachmentDescs[0] = vk::AttachmentDescription({},
		context.getSwapchainImageFormat(),		// format
		vk::SampleCountFlagBits::e1,			// sample
		vk::AttachmentLoadOp::eClear,			// attachment load/store op
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,		// stencil load/store op
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,			// enter renderpass in this layout
		vk::ImageLayout::ePresentSrcKHR			// exit renderpass in this layout
	);

	// Depth
	attachmentDescs[1] = vk::AttachmentDescription({},
		context.getDepthFormat(),
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eDontCare,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eDepthStencilAttachmentOptimal
	);

	// Reference to the attachment descriptions
	vk::AttachmentReference colorRef(0, vk::ImageLayout::eAttachmentOptimalKHR);				// 2nd arg -> layout to use during in subpass using this ref
	vk::AttachmentReference depthRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::SubpassDescription subpassDesc({}, vk::PipelineBindPoint::eGraphics, {}, colorRef, {}, &depthRef);


	// Corrected (sync validation WAW hazard)
	vk::SubpassDependency extInDep(
		VK_SUBPASS_EXTERNAL,
		0,

		vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests |
		vk::PipelineStageFlagBits::eColorAttachmentOutput,

		vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests |
		vk::PipelineStageFlagBits::eColorAttachmentOutput,

		{},
		vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead |
		vk::AccessFlagBits::eColorAttachmentWrite
	);

	// Using implicit external subpass

	return dev.createRenderPassUnique(vk::RenderPassCreateInfo({}, attachmentDescs, subpassDesc, extInDep));

	/*
	RenderPassBuilder

		.setAttachmentDesc(0,
			...
			...
			...)
		.setAttachmentDesc(1,
			...
			...
			...)
		.setSubpassInfo(subpassIDX, { 0, Layout }, { 1, Layout }, ...) --> Variadic templates to make unlimited args

		.setSubpassDep(VK_EXT, 0, SubpassDependency)
		.setSubpassDep(0, 1, SubpassDependency)
		.setSubpassDep(1, VK_EXT, SubpassDependency)

		.build(device);
	
	
	*/
}

std::vector<vk::UniqueFramebuffer> createDefaultFramebuffers(VulkanContext& context, const vk::RenderPass& suitableRenderPass)
{

	auto dev = context.getDevice();
	auto scExtent = context.getSwapchainExtent();
	uint32_t scImageCount = context.getSwapchainImageCount();
	auto scViews = context.getSwapchainViews();
	auto depthV = context.getDepthView();

	std::vector<vk::UniqueFramebuffer> framebuffers;
	framebuffers.reserve(scImageCount);
	for (uint32_t i = 0; i < scImageCount; ++i)
	{
		std::array<vk::ImageView, 2> attachments{ scViews[i], depthV };
		vk::FramebufferCreateInfo fbc({}, suitableRenderPass, attachments, scExtent.width, scExtent.height, 1);
		framebuffers.push_back(dev.createFramebufferUnique(fbc));
	}

	return framebuffers;
}



}


}

