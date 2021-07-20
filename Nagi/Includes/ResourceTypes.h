#pragma once
#include "VulkanContext.h"


namespace Nagi
{
	class Buffer
	{
	public:
		Buffer() = delete;
		~Buffer();
		Buffer(VmaAllocator allocator, const vk::BufferCreateInfo& bufCI, const VmaAllocationCreateInfo& allocCI);

		void putData(const void* inData, size_t dataSize);
		const vk::Buffer& getBuffer() const;

		template <typename T>
		static std::unique_ptr<Buffer> loadImmutable(const VulkanContext& context, const std::vector<T>& inData, vk::BufferUsageFlagBits usage)
		{
			if (!(usage & vk::BufferUsageFlagBits::eVertexBuffer || usage & vk::BufferUsageFlagBits::eIndexBuffer))
				throw std::runtime_error("loadVkImmutableBuffer suitability with non Vertex/Index buffers have not been checked! (Temporarily disabled for non Vertex/Index buffers");

			uint32_t dataSizeInBytes = static_cast<uint32_t>(inData.size() * sizeof(T));
			VmaAllocator allocator = context.getAllocator();

			// Create staging buffer
			vk::BufferCreateInfo stagingCI({}, dataSizeInBytes, vk::BufferUsageFlagBits::eTransferSrc);
			VmaAllocationCreateInfo stagingBufAllocCI{};
			stagingBufAllocCI.usage = VMA_MEMORY_USAGE_CPU_ONLY;

			auto stagingBuffer = std::make_unique<Buffer>(allocator, stagingCI, stagingBufAllocCI);

			// Copy data from CPU to staging buffer
			stagingBuffer->putData(inData.data(), dataSizeInBytes);



			// =========================================================


			// Create vertex buffer
			vk::BufferCreateInfo vertBufCI({}, dataSizeInBytes, usage | vk::BufferUsageFlagBits::eTransferDst);
			VmaAllocationCreateInfo vertBufAllocCI{};
			vertBufAllocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			auto vertexBuffer = std::make_unique<Buffer>(allocator, vertBufCI, vertBufAllocCI);

			// Copy data from staging buffer to vertex buffer
			auto& uploadContext = context.getUploadContext();
			uploadContext.submitWork(
				[&](const vk::CommandBuffer& cmd)
				{
					vk::BufferCopy copyRegion(0, 0, dataSizeInBytes);
					cmd.copyBuffer(stagingBuffer->getBuffer(), vertexBuffer->getBuffer(), copyRegion);
				});


			return vertexBuffer;
		}

	private:
		// Non owning 
		VmaAllocator m_allocator;

		// Owns these resources
		VmaAllocation alloc;
		vk::Buffer resource;

	};

	bool operator==(const Buffer& a, const Buffer& b);
	bool operator!=(const Buffer& a, const Buffer& b);

	class Texture
	{
	public:
		Texture() = default;
		~Texture();
		Texture(VmaAllocator allocator, vk::Device& dev, const vk::ImageCreateInfo& imgCI, const VmaAllocationCreateInfo& allocCI);

		void createView(const vk::ImageViewCreateInfo& viewCI);

		const vk::Image& getImage() const;
		const vk::ImageView& getImageView() const;

		static std::unique_ptr<Texture> fromFile(VulkanContext& context, const std::string& filePath, bool generateMips = false);

	private:
		// Non owning
		VmaAllocator m_allocator;
		vk::Device& m_dev;

		// Owning
		VmaAllocation m_alloc;
		vk::Image m_resource;
		vk::ImageView m_view;

	};

	bool operator==(const Texture& a, const Texture& b);
	bool operator!=(const Texture& a, const Texture& b);

	class Material
	{
	public:
		Material() = default;
		Material(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, vk::DescriptorSet descriptorSet);
		~Material() = default;

		const vk::Pipeline& getPipeline() const;
		const vk::PipelineLayout& getPipelineLayout() const;
		const vk::DescriptorSet& getDescriptorSet() const;

	private:
		vk::Pipeline m_pipeline;						// actual pipeline (e.g full graphics pipeline states)
		vk::PipelineLayout m_pipelineLayout;			// has descriptor set layout and push range info (needed for setting descriptor sets and pushing data for push constants)

		vk::DescriptorSet m_descriptorSet;					// has the resource bindings
	};

	bool operator==(const Material& a, const Material& b);
	bool operator!=(const Material& a, const Material& b);

	class Mesh
	{
	public:
		Mesh() = delete;
		Mesh(uint32_t firstIndex, uint32_t numIndices, uint32_t vbOffset = 0);
		~Mesh() = default;

		uint32_t getFirstIndex() const;
		uint32_t getNumIndices() const;
		uint32_t getVertexBufferOffset() const;

	private:
		uint32_t m_ibFirstIndex;
		uint32_t m_numIndices;
		uint32_t m_vbOffset;
	};

	class RenderUnit
	{
	public:
		RenderUnit() = delete;
		RenderUnit(const Mesh& mesh, const Material& material);
		~RenderUnit() = default;

		const Mesh& getMesh() const;
		const Material& getMaterial() const;

	private:
		Mesh m_mesh;				// POD

		// Non-owning
		const Material& m_material;
	};

	// A collection of coherent meshes to be rendered
	class RenderModel
	{
	public:
		RenderModel() = delete;
		RenderModel(std::unique_ptr<Buffer> vb, std::unique_ptr<Buffer> ib, std::vector<RenderUnit> renderUnits);
		~RenderModel() = default;

		const vk::Buffer& getVertexBuffer() const;
		const vk::Buffer& getIndexBuffer() const;

		// Temporary way of rendering (For each RenderObject, traverse this RenderUnit list
		const std::vector<RenderUnit>& getRenderUnits() const;

		RenderModel(const RenderModel&) = delete;
		RenderModel& operator=(const RenderModel&) = delete;
		RenderModel(RenderModel&&) = delete;
		RenderModel operator=(RenderModel&&) = delete;

	private:
		std::vector<RenderUnit> m_renderUnits;

		// Owning
		std::unique_ptr<Buffer> m_vb;
		std::unique_ptr<Buffer> m_ib;

	};



}
