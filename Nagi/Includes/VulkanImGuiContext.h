#pragma once
#include "VulkanContext.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

namespace Nagi
{
	class VulkanImGuiContext
	{
	public:
		VulkanImGuiContext(VulkanContext& context, Window& window, vk::RenderPass compatibleRenderPass, uint32_t subpass = 0);
		VulkanImGuiContext() = delete;
		~VulkanImGuiContext();

		void beginFrame();
		void render(vk::CommandBuffer& cmd);

	private:
		void createPipeline(VulkanContext& context, vk::RenderPass compatibleRenderPass, uint32_t subpass);

	private:
		VulkanContext& m_vkContext;
		vk::UniquePipeline m_correctedGammaPipeline;
		vk::UniquePipelineLayout m_correctedGammaPipelineLayout;
		vk::UniqueDescriptorPool m_descriptorPool;

		vk::UniqueSampler m_sampler;
		vk::UniqueDescriptorSetLayout m_dsl;
		vk::UniqueDescriptorSet m_descriptorSet;
	};
}
