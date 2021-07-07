#pragma once
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

// Forward declare VmaAllocator
//typedef struct VmaAllocator_T* VmaAllocator;

namespace Nagi
{

struct PerFrameSyncResource;
struct QueueFamilies;
class Window;

class GraphicsContext
{
public:
	GraphicsContext(const Window& win, bool debugLayer = true);
	~GraphicsContext();

	GraphicsContext() = delete;
	GraphicsContext(const GraphicsContext&) = delete;
	GraphicsContext& operator=(const GraphicsContext&) = delete;

	std::pair<vk::Semaphore, vk::Semaphore> BeginFrame();
	void EndFrame();

private:
	static constexpr uint32_t s_maxFramesInFlight = 2;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	// Arguments for these functions are verbose on purpose
	// It is to make the dependency clear (e.g what does a swapchain need?)
	void CreateInstance(std::vector<const char*> requiredExtensions, bool debugLayer);
	void CreateDebugMessenger(const vk::Instance& instance);

	void CreateVulkanMemoryAllocator(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice);

	void GetPhysicalDevice(const vk::Instance& instance);
	void CreateLogicalDevice(const vk::PhysicalDevice& physDevice, const QueueFamilies& qfs, vk::SurfaceKHR surface, bool debugLayer);
	vk::SurfaceFormatKHR CreateSwapchain(const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice, vk::SurfaceKHR surface, std::pair<uint32_t, uint32_t> clientDimensions);
	void CreateSwapchainImageViews(const vk::SwapchainKHR& swapchain, const vk::Device& logicalDevice, const vk::SurfaceFormatKHR& surfaceFormat);
	void CreateDepthResources(const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice, std::pair<uint32_t, uint32_t> clientDimensions);
	void CreateCommandPools(const vk::Device& logicalDevice, const QueueFamilies& qfs);
	void CreateSyncObjects(const vk::Device& logicalDevice, uint32_t maxFramesInFlight);
	void CreateCommandBuffers(const vk::Device& logicalDevice, const vk::CommandPool& cmdPool, uint32_t maxFramesInFlight);
		
	// Helpers
	QueueFamilies FindQueueFamilies(const vk::PhysicalDevice& physDevice, vk::SurfaceKHR surface) const;
	vk::SurfaceFormatKHR SelectSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& surfaceFormats) const;
	vk::PresentModeKHR SelectPresentMode(const std::vector<vk::PresentModeKHR>& presentModes) const;
	vk::Extent2D SelectSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, std::pair<uint32_t, uint32_t> clientDimensions);

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
	//VmaAllocator m_allocator;

	vk::SurfaceKHR m_surface;
	vk::SwapchainKHR m_swapchain;
	uint32_t m_swapchainImageCount;
	std::vector<vk::Image> m_swapchainImages;
	std::vector<vk::ImageView> m_swapchainImageViews;

	vk::Image m_depthImage;
	vk::DeviceMemory m_depthMemory;
	vk::ImageView m_depthView;

	// Vma depth
	//vk::Image m_vmaDepthImage;
	//VmaAllocation m_vmaDepthAlloc;

};




}
