#pragma once
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
#include "Singleton.h"

namespace Nagi
{

struct QueueFamilies;
class Window;

struct PerFrameSyncResource
{
	vk::Semaphore imageAvailableSemaphore;		// To halt the pipeline if Swapchain image is not yet available (Swapchain not done with it)
	vk::Semaphore renderFinishedSemaphore;		// To wait for the render to be finished before letting the Swapchain acquire the image to present
	vk::Fence inFlightFence;					// To make sure that we are not using resources that are in flight! (e.g Command Buffer still in use when we want to use it again)
};

struct FrameResource
{
	vk::CommandBuffer* gfxCmdBuffer;
	uint32_t imageIdx;
	PerFrameSyncResource* sync;
};


class GraphicsContext : Utils::Singleton<GraphicsContext>
{
public:
	GraphicsContext(const Window& win, bool debugLayer = true);
	~GraphicsContext();

	GraphicsContext() = delete;
	GraphicsContext(const GraphicsContext&) = delete;
	GraphicsContext& operator=(const GraphicsContext&) = delete;
	GraphicsContext(GraphicsContext&&) = delete;
	GraphicsContext operator=(GraphicsContext&&) = delete;

	// Should be called as late as possible (Fence wait can be CPU blocking!)
	// MAYBE REPLACE??
	// Separate getFrameResource from vkWaitFence? Maybe not? Since what we have now is essentially that any GPU res usage between beginFrame and endFrame
	// is guaranteed to be O.K (since we waited for the fence)
	FrameResource beginFrame();

	// One queue submit per frame is assumed right now until further exploration
	void submitQueue(const vk::SubmitInfo& info);

	// Last external subpass must transition the swapchain image to proper presentation layout!
	void endFrame();

	// Return device for now (until we can find better abstraction)
	vk::Device getDevice() const;

	VmaAllocator getResourceAllocator() const;

	// Temporary dependencies needed by the outside, maybe we can refactor to
	// SwapchainInfo and DepthInfo
	uint32_t getSwapchainImageCount() const;
	const std::vector<vk::ImageView>& getSwapchainViews() const;
	const vk::Extent2D& getSwapchainExtent() const;
	const vk::Format& getSwapchainImageFormat() const;

	const vk::ImageView& getDepthView() const;
	vk::Format getDepthFormat() const;

private:
	static constexpr uint32_t s_maxFramesInFlight = 2;

	// Arguments for these functions are verbose on purpose (instead of using member variables inside)
	// It is to make the dependency clear (e.g what does a swapchain need?)
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
	void createCommandBuffers(const vk::Device& logicalDevice, const vk::CommandPool& cmdPool, uint32_t maxFramesInFlight);
		
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

	vk::DispatchLoaderDynamic m_didl;
	vk::DebugUtilsMessengerEXT m_debugMessenger;

	vk::CommandPool m_gfxCmdPool;
	std::vector<vk::CommandBuffer> m_gfxCmdBuffers;

	uint32_t m_currFrame;
	uint32_t m_currImageIdx;
	std::vector<PerFrameSyncResource> m_frameSyncResources;

	// VMA
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
