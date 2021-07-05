#include "pch.h"
#include "GraphicsContext.h"
#include "Window.h"

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
	CreateInstance(win.GetRequiredExtensions(), debugLayer);

	vk::SurfaceKHR surface = win.GetSurface(m_instance);
	CreateDebugMessenger(m_instance);

	GetPhysicalDevice(m_instance);
	CreateLogicalDevice(m_physicalDevice, surface);

	CreateSwapchain(m_physicalDevice, m_logicalDevice, surface, { win.GetClientWidth(), win.GetClientHeight() });
}

GraphicsContext::~GraphicsContext()
{
	m_logicalDevice.destroy();
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
	auto instanceLoader = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);

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
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,

			DebugCallback
		},
		nullptr,	// User data
		instanceLoader);
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

	// Fallback to FIFO if Mailbox not available
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

	std::for_each(uniqueQfs.cbegin(), uniqueQfs.cend(),
		[&queueCreateInfos, queuePriority](uint32_t qfmIdx)
		{
			queueCreateInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), qfmIdx, 1, &queuePriority));
		});

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

void GraphicsContext::CreateSwapchain(const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice, vk::SurfaceKHR surface, std::pair<uint32_t, uint32_t> clientDimensions)
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


	// Revisit this later!!! We can check for support
	vk::SurfaceTransformFlagBitsKHR preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;

	// Revisit this later!!! We can check for support
	// The alpha channel, if it exists, of the images is ignored
	vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;


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

}


}

