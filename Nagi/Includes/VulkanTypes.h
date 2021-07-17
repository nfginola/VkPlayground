#pragma once
#include "VulkanContext.h"

namespace Nagi
{


class Buffer
{
public:
	Buffer() = delete;
	~Buffer() { vmaDestroyBuffer(m_allocator, resource, alloc); };
	Buffer(VmaAllocator allocator, const vk::BufferCreateInfo& bufCI, const VmaAllocationCreateInfo& allocCI) :
		m_allocator(allocator)
	{
		if (vmaCreateBuffer(allocator, (const VkBufferCreateInfo*)&bufCI, &allocCI, (VkBuffer*)&resource, &alloc, nullptr) != VK_SUCCESS)
			throw std::runtime_error("Could not create buffer");
	}

	void putData(const void* inData, size_t dataSize)
	{
		void* mappedData = nullptr;
		vmaMapMemory(m_allocator, alloc, &mappedData);
		memcpy(mappedData, inData, dataSize);
		vmaUnmapMemory(m_allocator, alloc);
	}

	const vk::Buffer& getBuffer() const { return resource; }

private:
	// Non owning 
	VmaAllocator m_allocator;

	// Owns these resources
	VmaAllocation alloc;
	vk::Buffer resource;

};

class Texture
{
public:
	Texture() = default;
	~Texture() { m_dev.destroyImageView(m_view); vmaDestroyImage(m_allocator, m_resource, m_alloc); }
	Texture(VmaAllocator allocator, vk::Device& dev, const vk::ImageCreateInfo& imgCI, const VmaAllocationCreateInfo& allocCI) :
		m_allocator(allocator),
		m_dev(dev)
	{
		if (vmaCreateImage(allocator, (const VkImageCreateInfo*)&imgCI, &allocCI, (VkImage*)&m_resource, &m_alloc, nullptr) != VK_SUCCESS)
			throw std::runtime_error("Could not create image");
	}

	//void setView(vk::ImageView view) { m_view = view; }
	void createView(const vk::ImageViewCreateInfo& viewCI)
	{
		m_view = m_dev.createImageView(viewCI);
	}

	const vk::Image& getImage() const { return m_resource; }
	const vk::ImageView& getImageView() const { return m_view; }

private:
	// Non owning
	VmaAllocator m_allocator;
	vk::Device& m_dev;

	// Owning
	VmaAllocation m_alloc;
	vk::Image m_resource;
	vk::ImageView m_view;

};

class Material
{
public:
	Material(const vk::Pipeline& pipeline, const vk::PipelineLayout& pipelineLayout, vk::DescriptorSet descriptorSet) :
		m_pipeline(pipeline), m_pipelineLayout(pipelineLayout), m_descriptorSet(descriptorSet) { }

	const vk::Pipeline& getPipeline() const { return m_pipeline; }
	const vk::PipelineLayout& getPipelineLayout() const { return m_pipelineLayout; }
	const vk::DescriptorSet& getDescriptorSet() const { return m_descriptorSet; }


private:
	// Non owning
	const vk::Pipeline& m_pipeline;						// actual pipeline (e.g full graphics pipeline states)
	const vk::PipelineLayout& m_pipelineLayout;			// has descriptor set layout and push range info (needed for setting descriptor sets and pushing data for push constants)

	// Non owning too (Pool is responsible internals for this)
	vk::DescriptorSet m_descriptorSet;					// has the resource bindings
};

class Mesh
{
public:
	Mesh() = delete;
	Mesh(uint32_t firstIndex, uint32_t numIndices, uint32_t vbOffset = 0) :
		m_ibFirstIndex(firstIndex), m_numIndices(numIndices), m_vbOffset(vbOffset) {}
	~Mesh() = default;

	uint32_t getFirstIndex() const { return m_ibFirstIndex; }
	uint32_t getNumIndices() const { return m_numIndices; }
	uint32_t getVertexBufferOffset() const { return m_vbOffset; }

private:
	const uint32_t m_ibFirstIndex;
	const uint32_t m_numIndices;
	const uint32_t m_vbOffset;
};

class RenderUnit
{
public:
	RenderUnit() = delete;
	RenderUnit(const Mesh& mesh, const Material& material) :
		m_mesh(mesh), m_material(material) {}
	~RenderUnit() = default;

	const Mesh& getMesh() const { return m_mesh; }
	const Material& getMaterial() const { return m_material; }

private:
	const Mesh m_mesh;				// POD

	// Non-owning
	const Material& m_material;
};

// A collection of coherent meshes to be rendered
class RenderModel
{
public:
	RenderModel() = delete;
	RenderModel(std::unique_ptr<Buffer> vb, std::unique_ptr<Buffer> ib, std::vector<RenderUnit> renderUnits) :
		m_vb(std::move(vb)), m_ib(std::move(ib)),	// Move ownership
		m_renderUnits(renderUnits) {}
	~RenderModel() = default;

	const vk::Buffer& getVertexBuffer() const { return m_vb->getBuffer(); }
	const vk::Buffer& getIndexBuffer() const { return m_ib->getBuffer(); }

	// Temporary way of rendering (For each RenderObject, traverse this RenderUnit list
	const std::vector<RenderUnit>& getRenderUnits() const { return m_renderUnits; }


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
