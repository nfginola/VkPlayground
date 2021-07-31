#include "pch.h"
#include "ResourceTypes.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Nagi
{

	Buffer::~Buffer()
	{
		if (m_mappedData != nullptr)
			vmaUnmapMemory(m_allocator, alloc);
		vmaDestroyBuffer(m_allocator, resource, alloc);
	}

	Buffer::Buffer(VmaAllocator allocator, const vk::BufferCreateInfo& bufCI, const VmaAllocationCreateInfo& allocCI) : 
		m_allocator(allocator)
	{
		if (vmaCreateBuffer(allocator, (const VkBufferCreateInfo*)&bufCI, &allocCI, (VkBuffer*)&resource, &alloc, nullptr) != VK_SUCCESS)
			throw std::runtime_error("Could not create buffer");
	}

	void Buffer::putData(const void* inData, size_t dataSize, size_t offset)
	{
		//char* mappedData = nullptr;
		
		// map once and keep it mapped
		if (m_mappedData == nullptr)
			vmaMapMemory(m_allocator, alloc, (void**)&m_mappedData);
		memcpy(m_mappedData + offset, inData, dataSize);

		//vmaUnmapMemory(m_allocator, alloc);
	}

	const vk::Buffer& Buffer::getBuffer() const
	{
		return resource;
	}







	Texture::~Texture()
	{
		m_dev.destroyImageView(m_view); 
		vmaDestroyImage(m_allocator, m_resource, m_alloc);
	}

	Texture::Texture(VmaAllocator allocator, vk::Device& dev, const vk::ImageCreateInfo& imgCI, const VmaAllocationCreateInfo& allocCI) :
		m_allocator(allocator),
		m_dev(dev)
	{
		if (vmaCreateImage(allocator, (const VkImageCreateInfo*)&imgCI, &allocCI, (VkImage*)&m_resource, &m_alloc, nullptr) != VK_SUCCESS)
			throw std::runtime_error("Could not create image");
	}

	void Texture::createView(const vk::ImageViewCreateInfo& viewCI)
	{
		m_view = m_dev.createImageView(viewCI);
	}

	const vk::Image& Texture::getImage() const
	{
		return m_resource;
	}

	const vk::ImageView& Texture::getImageView() const
	{
		return m_view;
	}

	std::unique_ptr<Texture> Texture::fromFile(VulkanContext& context, const std::string& filePath, bool generateMips, bool srgb)
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
		if (!srgb)
			imageFormat = vk::Format::eR8G8B8A8Unorm;

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

	std::unique_ptr<Texture> Texture::cubeFromFile(VulkanContext& context, const std::filesystem::path& path, bool srgb)
	{
		// Get all image data
		// ========================== Load image data
		std::array<stbi_uc*, 6> cubeData;
		std::array<std::string, 6> cubePaths{ "posx.jpg", "negx.jpg", "posy.jpg", "negy.jpg", "posz.jpg", "negz.jpg" };
		int texWidth, texHeight, texChannels;
		for (size_t i = 0; i < cubeData.size(); ++i)
		{
			auto fpath = path.relative_path().string() + cubePaths[i];
			stbi_uc* pixels = stbi_load(fpath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			if (!pixels)
				throw std::runtime_error("Can't find the image resource: " + fpath);

			cubeData[i] = pixels;
		}

		size_t imageSize = texWidth * texHeight * sizeof(uint32_t);

		auto allocator = context.getAllocator();

		// ========================== Create staging buffer and copy data to staging buffer
		vk::BufferCreateInfo stagingCI({}, imageSize * cubeData.size(), vk::BufferUsageFlagBits::eTransferSrc);
		VmaAllocationCreateInfo stagingBufAlloc{};
		stagingBufAlloc.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		Buffer stagingBuffer(allocator, stagingCI, stagingBufAlloc);

		for (size_t i = 0; i < cubeData.size(); ++i)
		{
			stagingBuffer.putData(cubeData[i], imageSize, i * imageSize);
			stbi_image_free(cubeData[i]);
		}

		// ========================== We now want to copy the data from our staging buffer into a texture array

		// Create texture array
		auto texExtent = vk::Extent3D(texWidth, texHeight, 1);
		vk::Format imageFormat = vk::Format::eR8G8B8A8Srgb;
		if (!srgb)	imageFormat = vk::Format::eR8G8B8A8Unorm;

		auto imageUsageBits = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;

		vk::ImageCreateInfo imgCI(
			vk::ImageCreateFlagBits::eCubeCompatible,
			vk::ImageType::e2D, imageFormat,
			texExtent,
			1, 6,	// 6
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			imageUsageBits
		);

		VmaAllocationCreateInfo texAlloc{};
		texAlloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		//Texture texture(allocator, context.getDevice(), imgCI, texAlloc);
		auto texture = std::make_unique<Texture>(allocator, context.getDevice(), imgCI, texAlloc);

	
		// Copy buffer data to texture in steps:
		// 1. Transition texture array into TransferDstOptimal
		// 2. Copy from buffer to texture
		// 3. Transition texture array into ShaderReadOnlyOptimal

		auto& uploadContext = context.getUploadContext();

		// Transition layout, copy buffer, transition again to shader read only optimal
		uploadContext.submitWork(
			[&](const vk::CommandBuffer& cmd)
			{
				vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6);		// 6 layers

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
					vk::PipelineStageFlagBits::eTransfer,	// any subsequent transfers should wait for this layout transition
					{},
					{},
					{},
					barrierForTransfer
				);

				// Now we can do our transfer cmd
				vk::BufferImageCopy copyRegion({}, {}, {},
					vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 6),
					{},
					texExtent
				);

				cmd.copyBufferToImage(stagingBuffer.getBuffer(), texture->getImage(), vk::ImageLayout::eTransferDstOptimal, copyRegion);

				// Transition to shader read only optimal
				vk::ImageMemoryBarrier barrierForReading(
					vk::AccessFlagBits::eTransferWrite,
					vk::AccessFlagBits::eShaderRead,
					vk::ImageLayout::eTransferDstOptimal,
					vk::ImageLayout::eShaderReadOnlyOptimal,
					{},
					{},
					texture->getImage(),
					range	// for all 6 
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


			});

		// Create image views
		vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
		vk::ImageSubresourceRange subresRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6);

		vk::ImageViewCreateInfo viewCreateInfo({},
			texture->getImage(),
			vk::ImageViewType::eCube,
			imageFormat,
			componentMapping,
			subresRange
		);

		texture->createView(viewCreateInfo);


		return texture;
	}

	Material::Material(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, vk::DescriptorSet descriptorSet) :
		m_pipeline(pipeline), m_pipelineLayout(pipelineLayout), m_descriptorSet(descriptorSet)
	{
	}





	const vk::Pipeline& Material::getPipeline() const
	{
		return m_pipeline;
	}

	const vk::PipelineLayout& Material::getPipelineLayout() const
	{
		return m_pipelineLayout;
	}

	const vk::DescriptorSet& Material::getDescriptorSet() const
	{
		return m_descriptorSet;
	}

	Mesh::Mesh(uint32_t firstIndex, uint32_t numIndices, uint32_t vbOffset) :
		m_ibFirstIndex(firstIndex), m_numIndices(numIndices), m_vbOffset(vbOffset)
	{
	}





	uint32_t Mesh::getFirstIndex() const
	{
		return m_ibFirstIndex;
	}

	uint32_t Mesh::getNumIndices() const
	{
		return m_numIndices;
	}

	uint32_t Mesh::getVertexBufferOffset() const
	{
		return m_vbOffset;
	}





	RenderUnit::RenderUnit(const Mesh& mesh, const Material& material) :
		m_mesh(mesh), m_material(material) 
	{
	}

	const Mesh& RenderUnit::getMesh() const
	{
		return m_mesh;
	}

	const Material& RenderUnit::getMaterial() const
	{
		return m_material;
	}





	RenderModel::RenderModel(std::unique_ptr<Buffer> vb, std::unique_ptr<Buffer> ib, std::vector<RenderUnit> renderUnits) :
		m_vb(std::move(vb)), m_ib(std::move(ib)),	// Move ownership
		m_renderUnits(renderUnits)
	{
	}


	const vk::Buffer& RenderModel::getVertexBuffer() const
	{
		return m_vb.get()->getBuffer();
	}

	const vk::Buffer& RenderModel::getIndexBuffer() const
	{
		return m_ib->getBuffer();
	}

	const std::vector<RenderUnit>& RenderModel::getRenderUnits() const
	{
		return m_renderUnits;
	}

	bool operator==(const Buffer& a, const Buffer& b)
	{
		return a.getBuffer() == b.getBuffer();
	}

	bool operator!=(const Buffer& a, const Buffer& b)
	{
		return !(a==b);
	}

	bool operator==(const Texture& a, const Texture& b)
	{
		return a.getImage() == b.getImage() && a.getImageView() == b.getImageView();
	}

	bool operator!=(const Texture& a, const Texture& b)
	{
		return !(a==b);
	}

	bool operator==(const Material& a, const Material& b)
	{
		return a.getPipeline() == b.getPipeline() &&
			a.getPipelineLayout() == b.getPipelineLayout() &&
			a.getDescriptorSet() == b.getDescriptorSet();
	}

	bool operator!=(const Material& a, const Material& b)
	{
		return !(a==b);
	}

	bool operator<(const Material& a, const Material& b)
	{
		// order priority: pipeline 1st and descriptor set 2nd 
		if (a.getPipeline() < b.getPipeline())
			return true;
		else if (a.getDescriptorSet() < b.getDescriptorSet())
			return true;
		return false;
	}

	bool operator<(const RenderUnit& a, const RenderUnit& b)
	{
		if (a.getMaterial() < b.getMaterial())
			return true;
		return false;
	}

}