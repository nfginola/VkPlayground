#pragma once
#include <vulkan/vulkan.hpp>

namespace Nagi
{
	// Assumption: 4 descriptor sets max (per frame, pass, material, object)
	class ShaderGroup
	{
	public:
		ShaderGroup();
		~ShaderGroup();

		// add stages to reflect and build
		ShaderGroup& addStage(vk::ShaderStageFlagBits stage, const std::filesystem::path& path);

		// reflect and build
		ShaderGroup& build(vk::Device dev);

		vk::PipelineLayout getPipelineLayout();
		vk::DescriptorSetLayout getSetLayout(uint32_t setNum);

	private:
		// Used to extract data for Reflection
		struct DescriptorSetLayoutData
		{
			uint32_t setNum = -1;
			vk::DescriptorSetLayoutCreateInfo ci;
			vk::DescriptorSetLayout layout;
			std::vector<vk::DescriptorSetLayoutBinding> bindings;
		};

		struct StageInfo
		{
			vk::ShaderStageFlagBits stage;
			std::vector<uint8_t> code;
			vk::ShaderModule module;
		};
		
		void reflect();


	private:
		std::vector<StageInfo> m_stages;
		std::array<DescriptorSetLayoutData, 4> m_setLayoutsData;
		vk::PipelineVertexInputStateCreateInfo m_vertInState;
		vk::PipelineLayout m_pipelineLayout;

	};

}

