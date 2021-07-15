#pragma once
#include "VulkanContext.h"

namespace Nagi
{

	// We can add a gen mipmap down the line..
	Texture loadVkImage(const VulkanContext& context, const std::string& filePath);

	// Designed with Vertex/Index buffers in mind
	template <typename T>
	Buffer loadVkImmutableBuffer(const VulkanContext& context, const std::vector<T>& inData, vk::BufferUsageFlagBits usage)
	{
		if (!(usage & vk::BufferUsageFlagBits::eVertexBuffer || usage & vk::BufferUsageFlagBits::eIndexBuffer))
			throw std::runtime_error("loadVkImmutableBuffer suitability with non Vertex/Index buffers have not been checked! (Temporarily disabled for non Vertex/Index buffers");

		uint32_t dataSizeInBytes = static_cast<uint32_t>(inData.size() * sizeof(T));
		VmaAllocator allocator = context.getResourceAllocator();

		// Create staging buffer
		vk::BufferCreateInfo stagingCI({}, dataSizeInBytes, vk::BufferUsageFlagBits::eTransferSrc);
		VmaAllocationCreateInfo stagingBufAllocCI{};
		stagingBufAllocCI.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		Buffer stagingBuffer(allocator, stagingCI, stagingBufAllocCI);

		// Copy data from CPU to staging buffer
		stagingBuffer.putData(inData.data(), dataSizeInBytes);



		// =========================================================


		// Create vertex buffer
		vk::BufferCreateInfo vertBufCI({}, dataSizeInBytes, usage | vk::BufferUsageFlagBits::eTransferDst);
		VmaAllocationCreateInfo vertBufAllocCI{};
		vertBufAllocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		Buffer vertexBuffer(allocator, vertBufCI, vertBufAllocCI);

		// Copy data from staging buffer to vertex buffer
		auto& uploadContext = context.getUploadContext();
		uploadContext.submitWork(
			[&](const vk::CommandBuffer& cmd)
			{
				vk::BufferCopy copyRegion(0, 0, dataSizeInBytes);
				cmd.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer.getBuffer(), copyRegion);
			});

		stagingBuffer.destroy();

		return vertexBuffer;
	}



// Helpers for unchanging states in simple applications, not meant to be an all-round helper
namespace ezTmp
{
	//Deps:
	//gfxCon: getSwapchainImageFormat(), getDepthFormat(), getDevice(for resource creation)
	vk::UniqueRenderPass createDefaultRenderPass(const VulkanContext& context);

	// Create framebuffers
	// resource deps: (1) sc view, (2) depth view (3) swapchain image count
	// Note: Can we remove these deps? sc image count dep may propagate to other per-frame resources.. (buffers to update, etc.)
	std::vector<vk::UniqueFramebuffer> createDefaultFramebuffers(const VulkanContext& context, const vk::RenderPass& suitableRenderPass);


}


}
