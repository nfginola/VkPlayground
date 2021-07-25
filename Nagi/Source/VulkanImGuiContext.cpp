#include "pch.h"
#include "VulkanImGuiContext.h"
#include "Window.h"
#include "Utilities.h"


namespace Nagi
{
    // From imgui_impl_vulkan.cpp
    // Used to access the Renderbackend, specifically to get the PipelineLayout so we can use our custom Pipeline (gamma corrected)
    // This is technically looking at implementation details, but this "hack" works well enough to solve the gamma problem while still retaining
    // the simple API for rendering
    struct ImGui_ImplVulkan_Data
    {
        ImGui_ImplVulkan_InitInfo   VulkanInitInfo;
        VkRenderPass                RenderPass;
        VkDeviceSize                BufferMemoryAlignment;
        VkPipelineCreateFlags       PipelineCreateFlags;
        VkDescriptorSetLayout       DescriptorSetLayout;
        VkPipelineLayout            PipelineLayout;
        VkDescriptorSet             DescriptorSet;
        VkPipeline                  Pipeline;
        uint32_t                    Subpass;
        VkShaderModule              ShaderModuleVert;
        VkShaderModule              ShaderModuleFrag;

        // Font data
        VkSampler                   FontSampler;
        VkDeviceMemory              FontMemory;
        VkImage                     FontImage;
        VkImageView                 FontView;
        VkDeviceMemory              UploadBufferMemory;
        VkBuffer                    UploadBuffer;

        // Render buffers
        //ImGui_ImplVulkanH_WindowRenderBuffers MainWindowRenderBuffers;
        uint32_t            Index;
        uint32_t            Count;
        void* FrameRenderBuffers;

        ImGui_ImplVulkan_Data()
        {
            memset(this, 0, sizeof(*this));
            BufferMemoryAlignment = 256;
        }
    };


    VulkanImGuiContext::VulkanImGuiContext(VulkanContext& context, Window& window, vk::RenderPass compatibleRenderPass, uint32_t subpass) :
        m_vkContext(context)
    {
        try
        {
            // Create descriptor pool for ImGui. Sizes taken straight from ImGui repo code example for Vulkan
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

            vk::DescriptorPoolCreateInfo poolCI(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, static_cast<uint32_t>(poolSizes.size()), poolSizes.data());
            m_descriptorPool = context.getDevice().createDescriptorPoolUnique(poolCI);

        }
        catch (vk::SystemError& err)
        {
            std::cout << "vk::SystemError: " << err.what() << std::endl;
            assert(false);
        }

        // Init ImGui
        ImGui::CreateContext();
        if (!ImGui_ImplGlfw_InitForVulkan(window.m_window, true))
            assert(false);
        
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = context.m_instance;
        initInfo.PhysicalDevice = context.m_physicalDevice;
        initInfo.Device = context.m_device;
        initInfo.Queue = context.m_gfxQueue;
        initInfo.DescriptorPool = m_descriptorPool.get();

        // This is dependent on application
        initInfo.Subpass = subpass;      

        // These below are hardcoded for now
        // Perhaps we should store the relevant values in VulkanContext and get them here!
        initInfo.MinImageCount = 2;
        initInfo.ImageCount = 3;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        if (!ImGui_ImplVulkan_Init(&initInfo, compatibleRenderPass))
            assert(false);

        // Custom pipeline which loads in a modified frag that undo gamma correction 
        createPipeline(context, compatibleRenderPass, subpass);

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
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd, m_correctedGammaPipeline.get());
    }


    void VulkanImGuiContext::createPipeline(VulkanContext& context, vk::RenderPass compatibleRenderPass, uint32_t subpass)
    {
        /*
        We can backtrack in imgui_impl_vulkan.cpp and check ImGui_ImplVulkan_CreatePipeline to recreate exactly the pipeline that they create, but with
        a different fragment shader that undo gamma correction! Do this in the future.
        If we dont pass a third argument for ImGui_ImplVulkan_RenderDrawData, it creates an internal Pipeline.
        Better that we supply it with modified fragment but identical everything else.
        */

        auto& dev = context.getDevice();

        // Mirroring the pipeline layout used internally is not enough!
        // We need the IDENTICAL pipeline layout object to initialize our custom pipeline!
        auto backendData = (ImGui_ImplVulkan_Data*)ImGui::GetIO().BackendRendererUserData;
        VkPipelineLayout pipelineLayout = backendData->PipelineLayout;

        // Not used 
        {
            // ============================= Create resources (This is not used, above is used to get the internal Pipeline Layout)
            VkSamplerCreateInfo samplerInfo = {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.minLod = -1000;
            samplerInfo.maxLod = 1000;
            samplerInfo.maxAnisotropy = 1.0f;
            m_sampler = dev.createSamplerUnique(samplerInfo);

            VkSampler sampler[1] = { m_sampler.get() };
            VkDescriptorSetLayoutBinding binding[1] = {};
            binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding[0].descriptorCount = 1;
            binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            binding[0].pImmutableSamplers = sampler;
            VkDescriptorSetLayoutCreateInfo dslInfo = {};
            dslInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            dslInfo.bindingCount = 1;
            dslInfo.pBindings = binding;
            m_dsl = dev.createDescriptorSetLayoutUnique(dslInfo);

            vk::DescriptorSetAllocateInfo allocInfo(m_descriptorPool.get(), m_dsl.get());
            m_descriptorSet = std::move(dev.allocateDescriptorSetsUnique(allocInfo).front());

            // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
            VkPushConstantRange pushConstants[1] = {};
            pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            pushConstants[0].offset = sizeof(float) * 0;
            pushConstants[0].size = sizeof(float) * 4;
            VkDescriptorSetLayout setLayout[1] = { m_dsl.get() };
            VkPipelineLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            layoutInfo.setLayoutCount = 1;
            layoutInfo.pSetLayouts = setLayout;
            layoutInfo.pushConstantRangeCount = 1;
            layoutInfo.pPushConstantRanges = pushConstants;
            m_correctedGammaPipelineLayout = dev.createPipelineLayoutUnique(layoutInfo);
        }

        // ============================ Create pipeline
        //ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
        //ImGui_ImplVulkan_CreateShaderModules(device, allocator);

        // Create Vert/Frag shader modules
        auto vertBin = readFile("compiled_shaders/imguiVert.spv");
        auto fragBin = readFile("compiled_shaders/imguiFrag.spv");
        auto vertMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, vertBin.size(), reinterpret_cast<uint32_t*>(vertBin.data())));
        auto fragMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, fragBin.size(), reinterpret_cast<uint32_t*>(fragBin.data())));

        VkPipelineShaderStageCreateInfo stage[2] = {};
        stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stage[0].module = vertMod.get();
        stage[0].pName = "main";
        stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stage[1].module = fragMod.get();
        stage[1].pName = "main";

        VkVertexInputBindingDescription binding_desc[1] = {};
        binding_desc[0].stride = sizeof(ImDrawVert);
        binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attribute_desc[3] = {};
        attribute_desc[0].location = 0;
        attribute_desc[0].binding = binding_desc[0].binding;
        attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_desc[0].offset = IM_OFFSETOF(ImDrawVert, pos);
        attribute_desc[1].location = 1;
        attribute_desc[1].binding = binding_desc[0].binding;
        attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_desc[1].offset = IM_OFFSETOF(ImDrawVert, uv);
        attribute_desc[2].location = 2;
        attribute_desc[2].binding = binding_desc[0].binding;
        attribute_desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        attribute_desc[2].offset = IM_OFFSETOF(ImDrawVert, col);

        VkPipelineVertexInputStateCreateInfo vertex_info = {};
        vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_info.vertexBindingDescriptionCount = 1;
        vertex_info.pVertexBindingDescriptions = binding_desc;
        vertex_info.vertexAttributeDescriptionCount = 3;
        vertex_info.pVertexAttributeDescriptions = attribute_desc;

        VkPipelineInputAssemblyStateCreateInfo ia_info = {};
        ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewport_info = {};
        viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_info.viewportCount = 1;
        viewport_info.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo raster_info = {};
        raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster_info.polygonMode = VK_POLYGON_MODE_FILL;
        raster_info.cullMode = VK_CULL_MODE_NONE;
        raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster_info.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms_info = {};
        ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        //ms_info.rasterizationSamples = (MSAASamples != 0) ? MSAASamples : VK_SAMPLE_COUNT_1_BIT; // Hardcoded
        ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState color_attachment[1] = {};
        color_attachment[0].blendEnable = VK_TRUE;
        color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
        color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
        color_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineDepthStencilStateCreateInfo depth_info = {};
        depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        VkPipelineColorBlendStateCreateInfo blend_info = {};
        blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend_info.attachmentCount = 1;
        blend_info.pAttachments = color_attachment;

        VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamic_state = {};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = (uint32_t)IM_ARRAYSIZE(dynamic_states);
        dynamic_state.pDynamicStates = dynamic_states;


        VkGraphicsPipelineCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        info.flags = {};
        info.stageCount = 2;
        info.pStages = stage;
        info.pVertexInputState = &vertex_info;
        info.pInputAssemblyState = &ia_info;
        info.pViewportState = &viewport_info;
        info.pRasterizationState = &raster_info;
        info.pMultisampleState = &ms_info;
        info.pDepthStencilState = &depth_info;
        info.pColorBlendState = &blend_info;
        info.pDynamicState = &dynamic_state;
        info.layout = pipelineLayout;
        //info.layout = m_correctedGammaPipelineLayout.get();
        //info.layout = pipLayout;
        info.renderPass = compatibleRenderPass;
        info.subpass = subpass;
        m_correctedGammaPipeline = dev.createGraphicsPipelineUnique({}, info).value;


     



    }

}
