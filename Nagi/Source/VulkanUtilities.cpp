#include "pch.h"
#include "VulkanUtilities.h"
#include "VulkanTypes.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Nagi
{

std::unique_ptr<Texture> loadVkImage(VulkanContext& context, const std::string& filePath, bool generateMips)
{


	// ========================== Load image data
	int texWidth, texHeight, texChannels;

	stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
		throw std::runtime_error("Can't find the image resource: " + filePath);

	size_t imageSize = texWidth * texHeight * sizeof(uint32_t);

	auto allocator = context.getAllocator();

	// Get mip levels
	uint32_t mipLevels = 1;
	if (generateMips)
		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	// ========================== Create staging buffer and copy data to staging buffer
	vk::BufferCreateInfo stagingCI({}, imageSize, vk::BufferUsageFlagBits::eTransferSrc);
	VmaAllocationCreateInfo stagingBufAlloc{};
	stagingBufAlloc.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	Buffer stagingBuffer(allocator, stagingCI, stagingBufAlloc);

	stagingBuffer.putData(pixels, imageSize);

	stbi_image_free(pixels);


	// =========================== Create texture
	auto texExtent = vk::Extent3D(texWidth, texHeight, 1);
	vk::Format imageFormat = vk::Format::eR8G8B8A8Srgb;

	auto imageUsageBits = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	if (generateMips)
		imageUsageBits |= vk::ImageUsageFlagBits::eTransferSrc;

	vk::ImageCreateInfo imgCI({},
		vk::ImageType::e2D, imageFormat,
		texExtent,
		mipLevels, 1,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		imageUsageBits
	);

	VmaAllocationCreateInfo texAlloc{};
	texAlloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	//Texture texture(allocator, context.getDevice(), imgCI, texAlloc);
	auto texture = std::make_unique<Texture>(allocator, context.getDevice(), imgCI, texAlloc);

	// ======================================= Initial Layout of image is Undefined, we need to transition its layout!
	auto& uploadContext = context.getUploadContext();

	// Transition layout, copy buffer, transition again to shader read only optimal
	uploadContext.submitWork(
		[&](const vk::CommandBuffer& cmd)
		{
			vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1);	// mipLevels --> makes sure that all mips get transitioned to linear layout

			vk::ImageMemoryBarrier barrierForTransfer(
				{},
				vk::AccessFlagBits::eTransferWrite,		// any transfer ops should wait until the image layout transition has occurred!
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eTransferDstOptimal,	// -> puts into linear layout, best for copying data from buffer to texture
				{},
				{},
				texture->getImage(),
				range
			);

			// Transition layout through barrier (after availability ops and before visibility ops)
			cmd.pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eTransfer,
				{},
				{},
				{},
				barrierForTransfer
			);

			// Now we can do our transfer cmd
			vk::BufferImageCopy copyRegion({}, {}, {},
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
				{},
				texExtent
			);

			cmd.copyBufferToImage(stagingBuffer.getBuffer(), texture->getImage(), vk::ImageLayout::eTransferDstOptimal, copyRegion);

			// IF we dont generate mips --> Transfer layout to shader read optimal
			// IF we will generate mips --> Leave layout in Transfer Destination Optimal
			if (!generateMips)
			{
				// Now we can transfer the image layout to optimal for shader usage
				vk::ImageMemoryBarrier barrierForReading(
					vk::AccessFlagBits::eTransferWrite,
					vk::AccessFlagBits::eShaderRead,
					vk::ImageLayout::eTransferDstOptimal,
					vk::ImageLayout::eShaderReadOnlyOptimal,
					{},
					{},
					texture->getImage(),
					range
				);

				// Transition layout to Read Only Optimal through pipeline barrier (after availability ops and before visibility ops)
				cmd.pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eFragmentShader,		// guarantee that transition has happened before any subsequent fragment shader reads
					{},												// above stage doesnt really matter since we are waiting for a Fence in submitWork which waits until the submitted work has COMPLETED execution
					{},
					{},
					barrierForReading
				);
			}
		});


	if (generateMips)
	{
		// Note that each mip is now still in Transfer Dst Optimal state given that generateMips = true
			// Generate mips
		uploadContext.submitWork(
			[&](const vk::CommandBuffer& cmd)
			{
				uint32_t mipWidth = texWidth;
				uint32_t mipHeight = texHeight;

				for (uint32_t i = 1; i < mipLevels; ++i)
				{
					// We choose to transition subres i-1 
					vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, i - 1, 1, 0, 1);

					vk::ImageMemoryBarrier barrierTransitionForBlit(
						vk::AccessFlagBits::eTransferWrite,
						vk::AccessFlagBits::eTransferRead,		// we will be reading from mip[i - 1]
						vk::ImageLayout::eTransferDstOptimal,
						vk::ImageLayout::eTransferSrcOptimal,
						{},
						{},
						texture->getImage(),
						range
					);

					// Make sure any previous transfer writes has been done (both copy from staging but also previous mip blits)
					cmd.pipelineBarrier(
						vk::PipelineStageFlagBits::eTransfer,
						vk::PipelineStageFlagBits::eTransfer,
						{},
						{},
						{},
						barrierTransitionForBlit
					);



					std::array<vk::Offset3D, 2> srcBounds;
					srcBounds[0] = vk::Offset3D(0, 0, 0);
					srcBounds[1] = vk::Offset3D(mipWidth, mipHeight, 1);	// tex dim of i-1 texture (which starts at texWidth and texHeight respectively on 0th mip

					std::array<vk::Offset3D, 2> dstBounds;
					dstBounds[0] = vk::Offset3D(0, 0, 0);
					dstBounds[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);

					vk::ImageSubresourceLayers srcSubres(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
					vk::ImageSubresourceLayers dstSubres(vk::ImageAspectFlagBits::eColor, i, 0, 1);

					// Blit image from i-1 to i
					vk::ImageBlit blitInfo(srcSubres, srcBounds, dstSubres, dstBounds);

					cmd.blitImage(texture->getImage(), vk::ImageLayout::eTransferSrcOptimal, texture->getImage(), vk::ImageLayout::eTransferDstOptimal, blitInfo, vk::Filter::eLinear);




					// Done reading from i-1, lets change its layout to shader read optimal
					barrierTransitionForBlit.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
					barrierTransitionForBlit.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
					barrierTransitionForBlit.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
					barrierTransitionForBlit.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

					// We guarantee that the image layout transition happens after blit transfer write and happens before shader read in fragment shader
					cmd.pipelineBarrier(
						vk::PipelineStageFlagBits::eTransfer,
						vk::PipelineStageFlagBits::eFragmentShader,
						{},
						{},
						{},
						barrierTransitionForBlit
					);


					if (mipWidth > 1) mipWidth /= 2;
					if (mipHeight > 1) mipHeight /= 2;
				}

				// Handle last mip since for loop doesnt take care of it
				vk::ImageSubresourceRange lastSubresRange(vk::ImageAspectFlagBits::eColor, mipLevels - 1, 1, 0, 1);
				vk::ImageMemoryBarrier barrierTransitionForBlit(
					vk::AccessFlagBits::eTransferWrite,
					vk::AccessFlagBits::eShaderRead,
					vk::ImageLayout::eTransferDstOptimal,
					vk::ImageLayout::eShaderReadOnlyOptimal,
					{},
					{},
					texture->getImage(),
					lastSubresRange
				);

				cmd.pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eFragmentShader,
					{},
					{},
					{},
					barrierTransitionForBlit
				);



			});
	}
	

	// NOTE TO SELF: We can extend this later for mipmap generation
	// ============================ Create image view
	vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
	vk::ImageSubresourceRange subresRange(vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1);

	vk::ImageViewCreateInfo viewCreateInfo({},
		texture->getImage(),
		vk::ImageViewType::e2D,
		imageFormat,
		componentMapping,
		subresRange
	);

	texture->createView(viewCreateInfo);

	return texture;
}




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

