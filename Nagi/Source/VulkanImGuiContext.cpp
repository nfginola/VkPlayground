#include "pch.h"
#include "VulkanImGuiContext.h"
#include "Window.h"


namespace Nagi
{

    VulkanImGuiContext::VulkanImGuiContext(VulkanContext& context, Window& window, vk::RenderPass compatibleRenderPass) :
        m_vkContext(context)
	{
        // Create descriptor pool for ImGui 
        std::vector<vk::DescriptorPoolSize> poolSizes =
        {
            { vk::DescriptorType::eSampler, 1000 },
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
            { vk::DescriptorType::eSampledImage, 1000 },
            { vk::DescriptorType::eStorageImage, 1000 },
            { vk::DescriptorType::eUniformTexelBuffer, 1000 },
            { vk::DescriptorType::eStorageTexelBuffer, 1000 },
            { vk::DescriptorType::eUniformBuffer, 1000 },
            { vk::DescriptorType::eStorageBuffer, 1000 },
            { vk::DescriptorType::eUniformBufferDynamic, 1000 },
            { vk::DescriptorType::eStorageBufferDynamic, 1000 },
            { vk::DescriptorType::eInputAttachment, 1000 }
        };

        vk::DescriptorPoolCreateInfo poolCI({}, 1000, poolSizes.size(), poolSizes.data());
        m_descriptorPool = context.getDevice().createDescriptorPoolUnique(poolCI);

        // Init ImGui
        ImGui::CreateContext();

        //this initializes imgui for SDL
        ImGui_ImplGlfw_InitForVulkan(window.m_window, true);

        //this initializes imgui for Vulkan
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = context.m_instance;
        initInfo.PhysicalDevice = context.m_physicalDevice;
        initInfo.Device = context.m_device;
        initInfo.Queue = context.m_gfxQueue;
        initInfo.DescriptorPool = m_descriptorPool.get();

        // hardcoded for now (we do have 3 in VulkanContext
        initInfo.MinImageCount = 3;
        initInfo.ImageCount = 3;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&initInfo, compatibleRenderPass);

        // Upload fonts
        context.getUploadContext().submitWork(
            [&](const vk::CommandBuffer& cmd)
            {
                ImGui_ImplVulkan_CreateFontsTexture(cmd);
            });

        // Clear font data from CPU now that its on GPU
        ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

    VulkanImGuiContext::~VulkanImGuiContext()
    {    
        m_vkContext.getDevice().waitIdle();
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void VulkanImGuiContext::beginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void VulkanImGuiContext::render(vk::CommandBuffer& cmd)
    {
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    }

}
