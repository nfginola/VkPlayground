#pragma once
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
#include "SingleInstance.h"


namespace Nagi
{

class Window;
class UploadContext;

struct QueueFamilies
{
	std::optional<uint32_t> gphIdx;
	std::optional<uint32_t> presentIdx;

	bool isComplete()
	{
		return gphIdx.has_value() &&
			presentIdx.has_value();
	}
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


class VulkanContext : private SingleInstance<VulkanContext>
{
private:
	static constexpr uint32_t s_maxFramesInFlight = 2;

public:
	VulkanContext(const Window& win, bool debugLayer = true);
	~VulkanContext();

	FrameResource beginFrame();
	void endFrame();								// Last external subpass must transition the swapchain image to proper presentation layout! 
	void submitQueue(const vk::SubmitInfo& info); 	// One queue submit per frame is assumed right now until further exploration




	// ========================== Below are dependencies needed outside
	vk::Device& getDevice();
	VmaAllocator getAllocator() const;
	UploadContext& getUploadContext() const;
	const vk::PhysicalDeviceProperties& getPhysicalDeviceProperties() const;

	// Maybe we can refactor to SwapchainInfo and DepthInfo
	uint32_t getSwapchainImageCount() const;
	const std::vector<vk::ImageView>& getSwapchainViews() const;
	const vk::Extent2D& getSwapchainExtent() const;
	vk::Format getSwapchainImageFormat() const;

	const vk::ImageView& getDepthView() const;
	vk::Format getDepthFormat() const;
	
	static constexpr uint32_t getMaxFramesInFlight() { return s_maxFramesInFlight; };

	VulkanContext() = delete;
	VulkanContext(const VulkanContext&) = delete;
	VulkanContext& operator=(const VulkanContext&) = delete;
	VulkanContext(VulkanContext&&) = delete;
	VulkanContext operator=(VulkanContext&&) = delete;

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

	friend class VulkanImGuiContext;
private:
	vk::Instance m_instance;
	vk::PhysicalDevice m_physicalDevice;
	QueueFamilies m_queueFamilies;
	vk::PhysicalDeviceProperties m_physicalDeviceProperties;
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



}
