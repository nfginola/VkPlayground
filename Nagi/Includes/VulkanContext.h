#pragma once
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
#include "SingleInstance.h"

namespace Nagi
{

class Window;
struct QueueFamilies;

class Buffer
{
public:
	Buffer() = default;

	Buffer(VmaAllocator allocator, const vk::BufferCreateInfo& bufCI, const VmaAllocationCreateInfo& allocCI) :
		m_allocator(allocator)
	{
		if (vmaCreateBuffer(allocator, (const VkBufferCreateInfo*)&bufCI, &allocCI, (VkBuffer*)&resource, &alloc, nullptr) != VK_SUCCESS) 
			throw std::runtime_error("Could not create buffer");
	}

	const vk::Buffer& getBuffer() const { return resource; }

	// Maybe we want templated? (Instead of free form void* and size)
	void putData(const void* inData, size_t dataSize)
	{
		// No validity checks done here (to check whether our resource is CPU mappable or not. The validation layer will catch right? o.o
		// Neither checking if inData is nullptr or not

		void* mappedData = nullptr;
		vmaMapMemory(m_allocator, alloc, &mappedData);
		memcpy(mappedData, inData, dataSize);
		vmaUnmapMemory(m_allocator, alloc);
	}

	void destroy() { vmaDestroyBuffer(m_allocator, resource, alloc); }

private:
	VmaAllocator m_allocator;
	VmaAllocation alloc;
	vk::Buffer resource;

};

struct Texture
{
	Texture() = default;

	Texture(VmaAllocator allocator, vk::Device dev, const vk::ImageCreateInfo& imgCI, const VmaAllocationCreateInfo& allocCI) :
		m_allocator(allocator),
		m_dev(dev)
	{
		if (vmaCreateImage(allocator, (const VkImageCreateInfo*)&imgCI, &allocCI, (VkImage*)&m_resource, &m_alloc, nullptr) != VK_SUCCESS)
			throw std::runtime_error("Could not create image");
	}

	const vk::Image& getImage() const { return m_resource; }
	const vk::ImageView& getImageView() const { return m_view; }

	void setView(vk::ImageView view) { m_view = view; }
	void destroy() { m_dev.destroyImageView(m_view); vmaDestroyImage(m_allocator, m_resource, m_alloc); }

private:
	VmaAllocator m_allocator;
	VmaAllocation m_alloc;
	vk::Image m_resource;
	vk::ImageView m_view;
	vk::Device m_dev;

};

struct Material
{
public:
	Material(const vk::Pipeline& pipeline, const vk::PipelineLayout& pipelineLayout, vk::DescriptorSet descriptorSet) :
		m_pipeline(pipeline), m_pipelineLayout(pipelineLayout), m_descriptorSet(descriptorSet) { }
	
	const vk::Pipeline& getPipeline() const { return m_pipeline; }
	const vk::PipelineLayout& getPipelineLayout() const { return m_pipelineLayout; }
	const vk::DescriptorSet& getDescriptorSet() const { return m_descriptorSet; }


private:
	const vk::Pipeline& m_pipeline;						// actual pipeline (e.g full graphics pipeline states)

	const vk::PipelineLayout& m_pipelineLayout;			// has descriptor set layout and push range info (needed for setting descriptor sets and pushing data for push constants)
	vk::DescriptorSet m_descriptorSet;					// has the resource bindings
};

class Mesh
{
public:
	Mesh() = delete;
	Mesh(const Buffer& vb, const Buffer& ib, uint32_t firstIndex, uint32_t numVerts, uint32_t vbOffset = 0) :
		m_vb(vb), m_ib(ib), m_ibFirstIndex(firstIndex), m_numVerts(numVerts), m_vbOffset(vbOffset) {}
	~Mesh() = default;

	uint32_t getFirstIndex() const { return m_ibFirstIndex; }
	uint32_t getNumVertices() const { return m_numVerts; }
	uint32_t getVertexBufferOffset() const { return m_vbOffset; }

private:
	// Each mesh will have a reference which it will not use (for now)
	// This is to simply make the dependency clear!
	const Buffer& m_vb;
	const Buffer& m_ib;
	const uint32_t m_ibFirstIndex;
	const uint32_t m_numVerts;
	const uint32_t m_vbOffset;
};

struct RenderUnit
{
public:
	RenderUnit() = delete;
	RenderUnit(const Mesh& mesh, const Material& material) :
		m_mesh(mesh), m_material(material) {}
	~RenderUnit() = default;

	const Mesh& getMesh() const { return m_mesh; }
	const Material& getMaterial() const { return m_material; }

private:
	const Mesh& m_mesh;
	const Material& m_material;
};

// A collection of coherent meshes to be rendered
class RenderModel
{
public:
	RenderModel() = delete;
	RenderModel(Buffer vb, Buffer ib, std::vector<RenderUnit> renderUnits) :
		m_vb(vb), m_ib(ib), m_renderUnits(renderUnits) {}
	~RenderModel()
	{
		m_vb.destroy();
		m_ib.destroy();
	}

	const vk::Buffer& getVertexBuffer() const { return m_vb.getBuffer(); }
	const vk::Buffer& getIndexBuffer() const { return m_ib.getBuffer(); }

	// Temporary way of rendering (For each RenderObject, traverse this RenderUnit list
	const std::vector<RenderUnit>& getRenderUnits() const { return m_renderUnits; }


	RenderModel(const RenderModel&) = delete;
	RenderModel& operator=(const RenderModel&) = delete;
	RenderModel(RenderModel&&) = delete;
	RenderModel operator=(RenderModel&&) = delete;

private:
	std::vector<RenderUnit> m_renderUnits;

	// Owns Buffers
	Buffer m_vb;
	Buffer m_ib;

};



struct PerFrameSyncResource
{
	vk::Semaphore imageAvailableSemaphore;		// To halt the pipeline if Swapchain image is not yet available (Swapchain not done with it)
	vk::Semaphore renderFinishedSemaphore;		// To wait for the render to be finished before letting the Swapchain acquire the image to present
	vk::Fence inFlightFence;					// To make sure that we are not using resources that are in flight! (e.g Command Buffer still in use when we want to use it again)
};

struct FrameResource
{
	vk::CommandPool& cmdPool;
	vk::CommandBuffer& gfxCmdBuffer;

	uint32_t imageIdx;
	PerFrameSyncResource& sync;
	uint32_t frameIdx;
};

class UploadContext : private SingleInstance<UploadContext>
{
public:
	UploadContext() = delete;
	UploadContext(vk::Device& dev, vk::Queue& queue, uint32_t queueFamily);
	~UploadContext() = default;

	UploadContext(const UploadContext&) = delete;
	UploadContext& operator=(const UploadContext&) = delete;
	UploadContext(UploadContext&&) = delete;
	UploadContext operator=(UploadContext&&) = delete;

	void submitWork(const std::function<void(const vk::CommandBuffer& cmd)>& work);

private:
	vk::Device& m_dev;
	vk::Queue& m_queue;
	uint32_t m_queueFamily;
	vk::UniqueFence m_fence;
	vk::UniqueCommandPool m_pool;

};


class VulkanContext : private SingleInstance<VulkanContext>
{
public:
	VulkanContext(const Window& win, bool debugLayer = true);
	~VulkanContext();

	VulkanContext() = delete;
	VulkanContext(const VulkanContext&) = delete;
	VulkanContext& operator=(const VulkanContext&) = delete;
	VulkanContext(VulkanContext&&) = delete;
	VulkanContext operator=(VulkanContext&&) = delete;

	FrameResource beginFrame();
	void endFrame();								// Last external subpass must transition the swapchain image to proper presentation layout! 
	void submitQueue(const vk::SubmitInfo& info); 	// One queue submit per frame is assumed right now until further exploration




	// ========================== Below are dependencies needed outside
	const vk::Device& getDevice() const;
	VmaAllocator getResourceAllocator() const;
	UploadContext& getUploadContext() const;

	// Maybe we can refactor to SwapchainInfo and DepthInfo
	uint32_t getSwapchainImageCount() const;
	const std::vector<vk::ImageView>& getSwapchainViews() const;
	const vk::Extent2D& getSwapchainExtent() const;
	const vk::Format& getSwapchainImageFormat() const;

	const vk::ImageView& getDepthView() const;
	vk::Format getDepthFormat() const;

	static constexpr uint32_t s_maxFramesInFlight = 2;

private:
	void createInstance(std::vector<const char*> requiredExtensions, bool debugLayer);
	void createDebugMessenger(const vk::Instance& instance);

	void createVulkanMemoryAllocator(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice);

	void getPhysicalDevice(const vk::Instance& instance);
	void createLogicalDevice(const vk::PhysicalDevice& physDevice, const QueueFamilies& qfs, vk::SurfaceKHR surface, bool debugLayer);
	vk::SurfaceFormatKHR createSwapchain(const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice, vk::SurfaceKHR surface, std::pair<uint32_t, uint32_t> clientDimensions);
	void createSwapchainImageViews(const vk::SwapchainKHR& swapchain, const vk::Device& logicalDevice, const vk::SurfaceFormatKHR& surfaceFormat);
	void createDepthResources(const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice, std::pair<uint32_t, uint32_t> clientDimensions);
	void createCommandPools(const vk::Device& logicalDevice, const QueueFamilies& qfs);
	void createSyncObjects(const vk::Device& logicalDevice, uint32_t maxFramesInFlight);
	void createCommandBuffers(const vk::Device& logicalDevice, const vk::CommandPool& cmdPool);

	// Helpers
	QueueFamilies findQueueFamilies(const vk::PhysicalDevice& physDevice, vk::SurfaceKHR surface) const;
	vk::SurfaceFormatKHR selectSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& surfaceFormats) const;
	vk::PresentModeKHR selectPresentMode(const std::vector<vk::PresentModeKHR>& presentModes) const;
	vk::Extent2D selectSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, std::pair<uint32_t, uint32_t> clientDimensions);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

private:
	vk::Instance m_instance;
	vk::PhysicalDevice m_physicalDevice;
	vk::Device m_device;
	vk::Queue m_gfxQueue;
	vk::Queue m_presentQueue;

	vk::DispatchLoaderDynamic m_dld;
	vk::DebugUtilsMessengerEXT m_debugMessenger;

	// Per frame
	std::vector<vk::CommandPool> m_gfxCmdPools;
	std::vector<vk::CommandBuffer> m_gfxCmdBuffers;

	uint32_t m_currFrame;
	uint32_t m_currImageIdx;
	std::vector<PerFrameSyncResource> m_frameSyncResources;

	std::unique_ptr<UploadContext> m_uploadContext;
	VmaAllocator m_allocator;

	vk::SurfaceKHR m_surface;
	vk::SwapchainKHR m_swapchain;
	vk::Extent2D m_swapchainExtent;
	vk::Format m_swapchainFormat;
	uint32_t m_swapchainImageCount;
	std::vector<vk::Image> m_swapchainImages;
	std::vector<vk::ImageView> m_swapchainImageViews;

	vk::Image m_depthImage;
	vk::DeviceMemory m_depthMemory;
	vk::ImageView m_depthView;
	vk::Format m_depthFormat;

	// Vma depth
	//vk::Image m_vmaDepthImage;
	//VmaAllocation m_vmaDepthAlloc;

};


}
