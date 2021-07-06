#include "pch.h"
#include "GraphicsContext.h"
#include "Window.h"

// VMA
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"


namespace Nagi
{

struct QueueFamilies
{
	std::optional<uint32_t> gphIdx;
	std::optional<uint32_t> presentIdx;

	bool IsComplete()
	{
		return gphIdx.has_value() &&
			presentIdx.has_value();
	}
};

GraphicsContext::GraphicsContext(const Window& win, bool debugLayer)
{
	// We limit the use of member variables in these creation helpers for learning purposes
	// This way, we can make it easy to see what each step of the creation requires at a glance!

	CreateInstance(win.GetRequiredExtensions(), debugLayer);

	m_surface = win.GetSurface(m_instance);
	CreateDebugMessenger(m_instance);

	GetPhysicalDevice(m_instance);
	CreateLogicalDevice(m_physicalDevice, m_surface);

	// Initialize VMA
	// We will use later, stick with normal Buffer/Image creation for now
	CreateVulkanMemoryAllocator(m_instance, m_physicalDevice, m_logicalDevice);

	vk::SurfaceFormatKHR surfaceFormatUsed = 
		CreateSwapchain(m_physicalDevice, m_logicalDevice, m_surface, { win.GetClientWidth(), win.GetClientHeight() });
	CreateSwapchainImageViews(m_swapchain, m_logicalDevice, surfaceFormatUsed);

	CreateDepthResources(m_physicalDevice, m_logicalDevice, { win.GetClientWidth(), win.GetClientHeight() });

}

GraphicsContext::~GraphicsContext()
{
	// ==================================== VMA related destructions

	// Destroy vma resources here
	// e.g vmaDestroyBuffer(m_allocator, buffer, allocation);

	vmaDestroyAllocator(m_allocator);

	// ==================================== Logical device related destructions
	for (auto view : m_swapchainImageViews)
		m_logicalDevice.destroyImageView(view);
	
	// This destroys the swapchain images too
	m_logicalDevice.destroySwapchainKHR(m_swapchain);

	m_logicalDevice.destroy();

	// ==================================== Instance related destructions
	m_instance.destroySurfaceKHR(m_surface);
	m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger, {}, m_didl);		// We used the dynamic dispatch to create our debug utils

	m_instance.destroy();
}

VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsContext::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void GraphicsContext::CreateInstance(std::vector<const char*> requiredExtensions, bool debugLayer)
{
	vk::ApplicationInfo appInfo("Nagi App", 1, "Nagi Engine", 1, VK_API_VERSION_1_1);

	std::vector<const char*> validationLayers;
	if (debugLayer)
	{
		// Check if validation layer is supported
		std::vector<vk::LayerProperties> instLayerProps = vk::enumerateInstanceLayerProperties();

		auto validationLayerPropIt = std::find_if(instLayerProps.cbegin(), instLayerProps.cend(),
			[](const vk::LayerProperties& layerProp)
			{
				return strcmp(layerProp.layerName, "VK_LAYER_KHRONOS_validation") == 0;
			});
		assert(validationLayerPropIt != instLayerProps.cend());

		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);	
		validationLayers.push_back("VK_LAYER_KHRONOS_validation");
	}

	vk::InstanceCreateInfo instCreateInfo({}, &appInfo,
		static_cast<uint32_t>(validationLayers.size()), validationLayers.data(),
		static_cast<uint32_t>(requiredExtensions.size()), requiredExtensions.data());

	try
	{
		m_instance = vk::createInstance(instCreateInfo);
	}
	catch (vk::SystemError& err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		assert(false);
	}


}

void GraphicsContext::CreateDebugMessenger(const vk::Instance& instance)
{
	m_didl = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);
	// Note to self:  (Compare with old C-style project)
	// Instead of loading the Extension function on compile time, we can do it in runtime.

	// Hook the messenger to the instance
	m_debugMessenger = instance.createDebugUtilsMessengerEXT(
		vk::DebugUtilsMessengerCreateInfoEXT
		{ 
			{},
			// Severities
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | 
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
			// Types
			//vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,

			DebugCallback
		},
		nullptr,	// User data
		m_didl);
}

void GraphicsContext::CreateVulkanMemoryAllocator(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice)
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = logicalDevice;
	allocatorInfo.instance = instance;

	
	assert(vmaCreateAllocator(&allocatorInfo, &m_allocator) == VK_SUCCESS);
}

void GraphicsContext::GetPhysicalDevice(const vk::Instance& instance) 
{
	// Simply get the one in front. We will assume that this is our primary graphics card
	// We can extend this by having some score value checking for each physical device
	m_physicalDevice = instance.enumeratePhysicalDevices().front();
}

QueueFamilies GraphicsContext::FindQueueFamilies(const vk::PhysicalDevice& physicalDevice, vk::SurfaceKHR surface) const
{
	QueueFamilies qfms{};
	std::vector<vk::QueueFamilyProperties> qfps = physicalDevice.getQueueFamilyProperties();
	
	// Get graphics family
	auto gphFamIt = std::find_if(qfps.begin(), qfps.end(),
		[](vk::QueueFamilyProperties const& qfp)
		{
			return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
		}
	);
	qfms.gphIdx = static_cast<uint32_t>(std::distance(qfps.begin(), gphFamIt));
	
	// Get present family
	if (!physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(std::distance(qfps.begin(), gphFamIt)), surface))
		qfms.presentIdx = static_cast<uint32_t>(std::distance(qfps.begin(), gphFamIt));
	else
	{
		// Find another family that supports presenting the surface if the graphics one doesn't support it
		uint32_t i = 0;
		for (; i < qfps.size(); ++i)
			if (physicalDevice.getSurfaceSupportKHR(i, surface))
			{
				qfms.presentIdx = static_cast<uint32_t>(i);
				break;
			}
	}

	assert(qfms.IsComplete());
	return qfms;
}

vk::SurfaceFormatKHR GraphicsContext::SelectSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& surfaceFormats) const
{
	auto selectedFormatIt = std::find_if(surfaceFormats.cbegin(), surfaceFormats.cend(),
		[](const vk::SurfaceFormatKHR& surfaceFormat)
		{
			return surfaceFormat.format == vk::Format::eB8G8R8A8Srgb &&
				surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
	assert(selectedFormatIt != surfaceFormats.cend());

	return *selectedFormatIt;
}

vk::PresentModeKHR GraphicsContext::SelectPresentMode(const std::vector<vk::PresentModeKHR>& presentModes) const
{
	auto selectedPresentModeIt = std::find_if(presentModes.cbegin(), presentModes.cend(),
		[](const vk::PresentModeKHR& presentMode)
		{
			return presentMode == vk::PresentModeKHR::eMailbox;
		});

	// Fallback to FIFO if Mailbox not available --> Guaranteed to be implemented
	if (selectedPresentModeIt == presentModes.cend())
		return vk::PresentModeKHR::eFifo;

	return *selectedPresentModeIt;
}

vk::Extent2D GraphicsContext::SelectSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, std::pair<uint32_t, uint32_t> clientDimensions)
{
	if (capabilities.currentExtent == UINT32_MAX)
	{
		// VkSurfaceCapabilitiesKHR spec: 
		// Special value (0xFFFFFFFF, 0xFFFFFFFF) indicating that the surface size will be determined by the extent of a swapchain
		// In this case, we will use the given window client dimensions to specify the extent
		return vk::Extent2D
		(
			std::clamp(clientDimensions.first, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp(clientDimensions.second, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		);
	}
	else
		return capabilities.currentExtent;
}

void GraphicsContext::CreateLogicalDevice(const vk::PhysicalDevice& physicalDevice, vk::SurfaceKHR surface)
{
	// Find queue family indicies
	QueueFamilies qfms = FindQueueFamilies(physicalDevice, surface);

	// The Vulkan spec states: The queueFamilyIndex member of each element of pQueueCreateInfos must be unique within pQueueCreateInfos 
	// (https://vulkan.lunarg.com/doc/view/1.2.176.1/windows/1.2-extensions/vkspec.html#VUID-VkDeviceCreateInfo-queueFamilyIndex-00372)
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQfs = { qfms.gphIdx.value(), qfms.presentIdx.value() };

	queueCreateInfos.reserve(queueCreateInfos.size());
	float queuePriority = 1.0f;

	for (auto qfmIdx : uniqueQfs)
		queueCreateInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), qfmIdx, 1, &queuePriority));

	// Enable validation layer on Device level (deprecated now) for backwards comp.
	std::vector<const char*> enabledLayers = { "VK_LAYER_KHRONOS_validation" };

	// Enable device specific extension
	std::vector<const char*> enabledExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	try
	{
		m_logicalDevice = physicalDevice.createDevice(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), queueCreateInfos, enabledLayers, enabledExtensions));
	}
	catch (vk::SystemError& err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
	}
}

vk::SurfaceFormatKHR GraphicsContext::CreateSwapchain(const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice, vk::SurfaceKHR surface, std::pair<uint32_t, uint32_t> clientDimensions)
{
	// ================= Gather surface details
	// Get VkFormats supported by the surface
	std::vector<vk::SurfaceFormatKHR> supportedFormats = physicalDevice.getSurfaceFormatsKHR(surface);
	assert(!supportedFormats.empty());

	// Get PresentModes supported by the surface
	std::vector<vk::PresentModeKHR> supportedPresentModes = physicalDevice.getSurfacePresentModesKHR(surface);
	assert(!supportedPresentModes.empty());

	// Get the capabilities of the surface
	vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

	// ================= Select swapchain settings
	vk::SurfaceFormatKHR surfaceFormat = SelectSurfaceFormat(supportedFormats);
	vk::PresentModeKHR presentMode = SelectPresentMode(supportedPresentModes);
	vk::Extent2D extent = SelectSwapchainExtent(surfaceCapabilities, clientDimensions);

	// Recommended to have one more image than minimum
	// We make sure that doesn't exceed maximum
	uint32_t imageCount = std::clamp(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);


	// --------------------------------------------------------
	// Revisit this later!!! We can check for support
	vk::SurfaceTransformFlagBitsKHR preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;

	// Revisit this later!!! We can check for support
	// The alpha channel, if it exists, of the images is ignored
	vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	// --------------------------------------------------------

	vk::SwapchainCreateInfoKHR scCreateInfo
	(
		{},
		surface,
		surfaceCapabilities.minImageCount,
		surfaceFormat.format,
		vk::ColorSpaceKHR::eSrgbNonlinear,
		extent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		{},					// Sharing Mode set below
		{},					// Queue Family Index Count set below
		{},					// Queue Family Indices set below
		preTransform,
		compositeAlpha,
		presentMode,
		true,				// Dont care about pixels that are obscured (e.g Window infront)
		nullptr				// Old swapchain nullptr (resize?)
	);


	QueueFamilies qfs = FindQueueFamilies(physicalDevice, surface);
	std::vector<uint32_t> qfIndices = { qfs.gphIdx.value(), qfs.presentIdx.value() };


	if (qfs.gphIdx != qfs.presentIdx)
	{
		// If different families, make it easy for ourselves:
		// We DO NOT handle ownership transfer explicitly!
		// Images can be used across multiple queue families without explicit ownership transfers
		scCreateInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
		scCreateInfo.setQueueFamilyIndexCount(static_cast<uint32_t>(qfIndices.size()));
		scCreateInfo.setQueueFamilyIndices(qfIndices);
	}
	else
		// If family is same, then we make sure that an image is owned by one queue family AT A TIME (EXCLUSIVE) --> Best performance
		// Ownership must be explicitly transferred before use by another family ( We only have one here :) )
		scCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);		

	try
	{
		m_swapchain = m_logicalDevice.createSwapchainKHR(scCreateInfo);
	}
	catch (vk::SystemError& err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
	}

	// Return surface format, we will need this when we create swapchain image views
	return surfaceFormat;
}

void GraphicsContext::CreateSwapchainImageViews(const vk::SwapchainKHR& swapchain, const vk::Device& logicalDevice, const vk::SurfaceFormatKHR& surfaceFormat)
{
	// Get the swapchain images
	m_swapchainImages = logicalDevice.getSwapchainImagesKHR(swapchain);
	m_swapchainImageViews.reserve(m_swapchainImages.size());

	// Specify component values placed in each component of the output vector (RGBA)
	vk::ComponentMapping componentMapping
	(
		vk::ComponentSwizzle::eR,
		vk::ComponentSwizzle::eG,
		vk::ComponentSwizzle::eB,
		vk::ComponentSwizzle::eA
	);

	vk::ImageSubresourceRange subresRange
	(
		vk::ImageAspectFlagBits::eColor,
		0,	// Base mip level
		1,	// Level count
		0,	// Base array layer 
		1	// layerCount
	);

	for (const auto& image : m_swapchainImages)
	{
		vk::ImageViewCreateInfo viewCreateInfo
		(
			{},
			image,
			vk::ImageViewType::e2D,
			surfaceFormat.format,
			componentMapping,	
			subresRange
		);
	
		try
		{
			m_swapchainImageViews.push_back(logicalDevice.createImageView(viewCreateInfo));
		}
		catch (vk::SystemError& err)
		{
			std::cout << "vk::SystemError: " << err.what() << std::endl;
			assert(false);
		}
	}	
}

void GraphicsContext::CreateDepthResources(const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice, std::pair<uint32_t, uint32_t> clientDimensions)
{
	// ========================= Create Image
	// Declare that we want a 32 bit signed floating point component
	const vk::Format depthFormat = vk::Format::eD32Sfloat;

	// Get properties of this format (We have to check tiling features supported by this device for this format)
	vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(depthFormat);
	
	// ImageTiling specifies tiling arrangement for the data in this image (how is it laid out)
	// We prioritize optimal
	vk::ImageTiling imageTiling;
	if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		imageTiling = vk::ImageTiling::eOptimal;	// Texels laid out in an implementation specific order for optimal access
	else if (formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		imageTiling = vk::ImageTiling::eLinear;		// Texels laid out in row major order in data	
	else
		assert(false);		// D32 is not a supported format a for the depth stencil attachment

	vk::ImageCreateInfo imageCreateInfo
	(
		{},
		vk::ImageType::e2D,
		depthFormat,
		vk::Extent3D(clientDimensions.first, clientDimensions.second, 1),
		1,
		1,
		vk::SampleCountFlagBits::e1,
		imageTiling,
		vk::ImageUsageFlagBits::eDepthStencilAttachment
		// Sharing mode exclusive --> Owned by one queue family at a time, hence no need for the rest of arguments (specifying queue families)
	);

	try
	{
		m_depthImage = logicalDevice.createImage(imageCreateInfo);
	}
	catch (vk::SystemError& err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		assert(false);
	}

	// ======================= Allocate memory for specified image
	// TO DO






}


}

