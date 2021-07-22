#pragma once
#include "VulkanContext.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

namespace Nagi
{
	class VulkanImGuiContext : private SingleInstance<VulkanImGuiContext>
	{
	public:
		VulkanImGuiContext(VulkanContext& context, Window& window, vk::RenderPass compatibleRenderPass);
		VulkanImGuiContext() = delete;
		~VulkanImGuiContext();


	private:
		VulkanContext& m_vkContext;
		vk::UniqueDescriptorPool m_descriptorPool;
	};
}
