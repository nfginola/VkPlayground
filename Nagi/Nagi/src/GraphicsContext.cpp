#include "pch.h"
#include "GraphicsContext.h"
#include "Window.h"

// VMA
//#define VMA_IMPLEMENTATION
//#include "vk_mem_alloc.h"

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

GraphicsContext::GraphicsContext(const Window& win, bool debugLayer) :
	m_currFrame(0)
{
	// We limit the use of member variables in these creation helpers for learning purposes
	// This way, we can make it easy to see what each step of the creation requires at a glance!


	try
	{
		createInstance(win.getRequiredExtensions(), debugLayer);

		m_surface = win.getSurface(m_instance);
		if (debugLayer)
			createDebugMessenger(m_instance);

		getPhysicalDevice(m_instance);

		QueueFamilies qfs =
		findQueueFamilies(m_physicalDevice, m_surface);
		createLogicalDevice(m_physicalDevice, qfs, m_surface, debugLayer);

		createCommandPools(m_device, qfs);

		// Initialize VMA
		// We will use later, stick with normal Buffer/Image creation for now for learning purposes
		// Note that there are VMA destruction code that are commented in the destructor
		// CreateVulkanMemoryAllocator(m_instance, m_physicalDevice, m_logicalDevice);

		std::pair<uint32_t, uint32_t> clientDim{ win.getClientWidth(), win.getClientHeight() };

		vk::SurfaceFormatKHR surfaceFormatUsed =
		createSwapchain(m_physicalDevice, m_device, m_surface, clientDim);
		createSwapchainImageViews(m_swapchain, m_device, surfaceFormatUsed);
		createDepthResources(m_physicalDevice, m_device, clientDim);

		// Frame synchronization
		createSyncObjects(m_device, s_maxFramesInFlight);
		createCommandBuffers(m_device, m_gfxCmdPool, s_maxFramesInFlight);
	}
	catch (vk::SystemError& err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		assert(false);
	}
	catch (std::exception& err)
	{
		std::cout << "std::exception: " << err.what() << std::endl;
		assert(false);
	}
	catch (...)
	{
		std::cout << "Unknown error\n";
		assert(false);
	}


}

GraphicsContext::~GraphicsContext()
{
	// ==================================== VMA related destructions

	// Cleanup depth resource (VMA)
	//vmaDestroyImage(m_allocator, m_vmaDepthImage, m_vmaDepthAlloc);

	//	vmaDestroyAllocator(m_allocator);

	// ==================================== Logical device related destructions
	m_device.destroyCommandPool(m_gfxCmdPool);	// All cmd buffers associated with this pool is cleaned up automatically

	for (auto view : m_swapchainImageViews)
		m_device.destroyImageView(view);
	
	// This destroys the swapchain images too
	m_device.destroySwapchainKHR(m_swapchain);

	// Cleanup depth resource
	m_device.destroyImageView(m_depthView);
	m_device.destroyImage(m_depthImage);
	m_device.freeMemory(m_depthMemory);

	for (auto res : m_frameSyncResources)
	{
		m_device.destroySemaphore(res.imageAvailableSemaphore);
		m_device.destroySemaphore(res.renderFinishedSemaphore);
		m_device.destroyFence(res.inFlightFence);
	}


	m_device.destroy();

	// ==================================== Instance related destructions
	m_instance.destroySurfaceKHR(m_surface);
	m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger, {}, m_didl);		// We used the dynamic dispatch to create our debug utils

	m_instance.destroy();
}


FrameResource GraphicsContext::beginFrame()
{
	try
	{
		// Wait for frame resources..
		auto waitRes = m_device.waitForFences({ m_frameSyncResources[m_currFrame].inFlightFence }, VK_TRUE, std::numeric_limits<uint64_t>::max());	// CPU blocked by GPU

		// Handle if wait times out
		// if (waitRes ...)

		// Reset fence (unsignal) so that we can use it on subsequent Queue submit(s?) (GPU can signal)
		m_device.resetFences({ m_frameSyncResources[m_currFrame].inFlightFence });
		// At this point, this frames GPU resource are available for use so we are safe to re-record to the command buffer for example

		auto imageAcquireResults = m_device.acquireNextImageKHR(m_swapchain, std::numeric_limits<uint64_t>::max(), { m_frameSyncResources[m_currFrame].imageAvailableSemaphore });
		m_currImageIdx = imageAcquireResults.value;

		// Resize or handle failed acquisition
		//if (imageAcquireResults.result...

		return
		{
			&m_gfxCmdBuffers[m_currFrame],
			m_currImageIdx,
			&m_frameSyncResources[m_currFrame]
		};
		
		//return 
		//{ 
		//	// Arguments needed in VkSubmitInfo
		//	m_frameSyncResources[m_currFrame].imageAvailableSemaphore.get(),
		//	m_frameSyncResources[m_currFrame].renderFinishedSemaphore.get(),
		//	//m_frameSyncResources[m_currFrame].inFlightFence.get(),
		//	m_gfxCmdBuffers[m_currFrame]		// Even though command buffer is tied to frame resource, lets keep it separate for now
		//};
	}
	catch (vk::SystemError& err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		assert(false);
	}

	return {};
}

// CharlesG: MaxFramesInFlight Command Buffers. One for recording this 'frame' and X for frame (N-1, N-2, ..) (which may be in execution) 
void GraphicsContext::submitQueue(const vk::SubmitInfo& info)
{
	try
	{
		m_gfxQueue.submit({ info }, m_frameSyncResources[m_currFrame].inFlightFence);
	}
	catch (vk::SystemError& err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		assert(false);
	}
}

void GraphicsContext::endFrame()
{
	try
	{
		vk::PresentInfoKHR presentInfo(m_frameSyncResources[m_currFrame].renderFinishedSemaphore, m_swapchain, m_currImageIdx);	
		auto presentResults = m_presentQueue.presentKHR(presentInfo);

		// Resize or handle failed presentation
		//if (imageAcquireResults.result...
	}
	catch (vk::SystemError& err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		assert(false);
	}

	m_currFrame = (m_currFrame + 1) % s_maxFramesInFlight;
}

vk::Device GraphicsContext::getDevice()
{
	return m_device;
}

uint32_t GraphicsContext::getSwapchainImageCount() const
{
	return m_swapchainImageCount;
}

const std::vector<vk::ImageView>& GraphicsContext::getSwapchainViews() const
{
	return m_swapchainImageViews;
}

const vk::ImageView& GraphicsContext::getDepthView() const
{
	return m_depthView;
}

const vk::Extent2D& GraphicsContext::getSwapchainExtent() const
{
	return m_swapchainExtent;
}

const vk::Format& GraphicsContext::getSwapchainImageFormat() const
{
	return m_swapchainFormat;
}



VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsContext::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void GraphicsContext::createInstance(std::vector<const char*> requiredExtensions, bool debugLayer)
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

	m_instance = vk::createInstance(instCreateInfo);


}

void GraphicsContext::createDebugMessenger(const vk::Instance& instance)
{
	m_didl = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);
	// Note to self:  (Compare with old C-style project)
	// Instead of loading the Extension function at compile time, we can do it in runtime (hence using dispatch loader dynamic)

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

			debugCallback
		},
		nullptr,	// User data
		m_didl);
}

void GraphicsContext::createVulkanMemoryAllocator(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice)
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = logicalDevice;
	allocatorInfo.instance = instance;
	
	//assert(vmaCreateAllocator(&allocatorInfo, &m_allocator) == VK_SUCCESS);
}

void GraphicsContext::getPhysicalDevice(const vk::Instance& instance) 
{
	// Simply get the one in front. We will assume that this is our primary graphics card
	// We can extend this by having some score value checking for each physical device in the future
	m_physicalDevice = instance.enumeratePhysicalDevices().front();
}

QueueFamilies GraphicsContext::findQueueFamilies(const vk::PhysicalDevice& physicalDevice, vk::SurfaceKHR surface) const
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

vk::SurfaceFormatKHR GraphicsContext::selectSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& surfaceFormats) const
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

vk::PresentModeKHR GraphicsContext::selectPresentMode(const std::vector<vk::PresentModeKHR>& presentModes) const
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

vk::Extent2D GraphicsContext::selectSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, std::pair<uint32_t, uint32_t> clientDimensions)
{
	if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
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

void GraphicsContext::createLogicalDevice(const vk::PhysicalDevice& physicalDevice, const QueueFamilies& qfs, vk::SurfaceKHR surface, bool debugLayer)
{
	// The Vulkan spec states: The queueFamilyIndex member of each element of pQueueCreateInfos must be unique within pQueueCreateInfos (hence we use set)
	// (https://vulkan.lunarg.com/doc/view/1.2.176.1/windows/1.2-extensions/vkspec.html#VUID-VkDeviceCreateInfo-queueFamilyIndex-00372)
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQfs = { qfs.gphIdx.value(), qfs.presentIdx.value() };

	queueCreateInfos.reserve(queueCreateInfos.size());
	float queuePriority = 1.0f;

	for (auto qfmIdx : uniqueQfs)
		queueCreateInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), qfmIdx, 1, &queuePriority));

	// Enable validation layer on Device level (deprecated now) for backwards comp.
	std::vector<const char*> enabledLayers;
	if (debugLayer)
		enabledLayers.push_back("VK_LAYER_KHRONOS_validation");

	// Enable device specific extension
	std::vector<const char*> enabledExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	m_device = physicalDevice.createDevice(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), queueCreateInfos, enabledLayers, enabledExtensions));

	
	// Retrieve the queues
	m_gfxQueue = m_device.getQueue(qfs.gphIdx.value(), 0);
	m_presentQueue = m_device.getQueue(qfs.presentIdx.value(), 0);

}

vk::SurfaceFormatKHR GraphicsContext::createSwapchain(const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice, vk::SurfaceKHR surface, std::pair<uint32_t, uint32_t> clientDimensions)
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
	vk::SurfaceFormatKHR surfaceFormat = selectSurfaceFormat(supportedFormats);
	m_swapchainFormat = surfaceFormat.format;
	vk::PresentModeKHR presentMode = selectPresentMode(supportedPresentModes);
	vk::Extent2D extent = selectSwapchainExtent(surfaceCapabilities, clientDimensions);
	m_swapchainExtent = extent;

	// Recommended to have one more image than minimum
	// We make sure that doesn't exceed maximum
	m_swapchainImageCount = std::clamp(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);


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
		m_swapchainImageCount,
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


	QueueFamilies qfs = findQueueFamilies(physicalDevice, surface);
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

	m_swapchain = logicalDevice.createSwapchainKHR(scCreateInfo);

	// Return surface format, we will need this when we create swapchain image views
	return surfaceFormat;
}

void GraphicsContext::createSwapchainImageViews(const vk::SwapchainKHR& swapchain, const vk::Device& logicalDevice, const vk::SurfaceFormatKHR& surfaceFormat)
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

		m_swapchainImageViews.push_back(logicalDevice.createImageView(viewCreateInfo));
	}	
}

void GraphicsContext::createDepthResources(const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice, std::pair<uint32_t, uint32_t> clientDimensions)
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

	m_depthImage = logicalDevice.createImage(imageCreateInfo);

	// ======================= Allocate memory for specified image
	vk::MemoryRequirements memReq = logicalDevice.getImageMemoryRequirements(m_depthImage);
	vk::PhysicalDeviceMemoryProperties physMemProps = physicalDevice.getMemoryProperties();

	uint32_t typeIndex = std::numeric_limits<uint32_t>::max();
	vk::MemoryPropertyFlagBits chosenProperty = vk::MemoryPropertyFlagBits::eDeviceLocal;

	// Find specific memory type index for memory allocation!
	// Here we find the suitable memory type index based on our needs (e.g chosen property and from which heap)
	for (uint32_t i = 0; i < physMemProps.memoryTypeCount; i++)
	{
		// memoryTypeBits is a bitmask and contains one bit set for every supported memory type for the resource. 
		// Bit i is set if and only if the memory type i in the VkPhysicalDeviceMemoryProperties structure for the physical device is supported for the resource.
		bool currMemTypeSupported = memReq.memoryTypeBits & (i >> 1);
		bool propertySupported = (physMemProps.memoryTypes[i].propertyFlags & chosenProperty) == chosenProperty;

		// We could add an extra check to select which heap we want but we will skip for now
		if (currMemTypeSupported && propertySupported)
		{
			typeIndex = i;
			break;
		}
	}
	assert(typeIndex != std::numeric_limits<uint32_t>::max());

	m_depthMemory = logicalDevice.allocateMemory(vk::MemoryAllocateInfo(memReq.size, typeIndex));
	

	// ======================= Bind memory to image!
	logicalDevice.bindImageMemory(m_depthImage, m_depthMemory, 0);



	// ======================= VMA ALTERNATIVE TO IMAGE CREATION (Handles image creation and memory allocation)

	//VmaAllocationCreateInfo allocationInfo{};
	//// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/choosing_memory_type.html
	//// We can leave all allocation flags off if we want (means no requirements specified for memory type)
	//allocationInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

	//VkImage tmpDepth;
	//assert(vmaCreateImage(
	//	m_allocator,
	//	(VkImageCreateInfo*)&imageCreateInfo,	// Theres an operator for vk::ImageCreateInfo that returns the underlying thing
	//	&allocationInfo,
	//	&tmpDepth,
	//	&m_vmaDepthAlloc,
	//	nullptr) == VK_SUCCESS);
	//m_vmaDepthImage = tmpDepth;


	// ======================= Create view for image
	
	// Question: Does component mapping change anything when it comes to Depth texture? (We are using D32..)
	vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
	vk::ImageSubresourceRange subresRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1);

	vk::ImageViewCreateInfo viewCreateInfo({},
		m_depthImage,
		vk::ImageViewType::e2D,
		depthFormat,
		componentMapping,
		subresRange
	);

	m_depthView = logicalDevice.createImageView(viewCreateInfo);
		
	//// VMA Alternative
	//viewCreateInfo.setImage(m_vmaDepthImage);
	//m_depthView = logicalDevice.createImageView(viewCreateInfo);


	
}

void GraphicsContext::createCommandPools(const vk::Device& logicalDevice, const QueueFamilies& qfs)
{
	// Resettable for re-recording..
	m_gfxCmdPool = logicalDevice.createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, qfs.gphIdx.value()));

	// Other pools can be created here..
}

void GraphicsContext::createSyncObjects(const vk::Device& logicalDevice, uint32_t maxFramesInFlight)
{
	vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
	vk::SemaphoreCreateInfo semCreateInfo;

	m_frameSyncResources.reserve(s_maxFramesInFlight);
	for (uint32_t i = 0; i < s_maxFramesInFlight; ++i)
	{
		m_frameSyncResources.push_back(
		{
			logicalDevice.createSemaphore(semCreateInfo),		// imageAvailable
			logicalDevice.createSemaphore(semCreateInfo),		// renderFinished
			logicalDevice.createFence(fenceCreateInfo)			// inFlight
		});
	}
}

void GraphicsContext::createCommandBuffers(const vk::Device& logicalDevice, const vk::CommandPool& cmdPool, uint32_t maxFramesInFlight)
{
	vk::CommandBufferAllocateInfo allocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, maxFramesInFlight);

	// We have 'maxFramesInFlight' amount of cmd buffers so that we can re-record a cmd buffer that isn't being executed!
	// (We will sync with the in-flight fence
	m_gfxCmdBuffers = logicalDevice.allocateCommandBuffers(allocateInfo);
	
	// We can create more command buffers here..
}


}

