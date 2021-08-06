#pragma once
#include <vulkan/vulkan.hpp>

namespace spv_reflect { class ShaderModule; }

namespace Nagi
{
	// Assumption: 4 descriptor sets (per frame, pass, material, object)
	class ShaderGroup
	{
	public:
		ShaderGroup();
		~ShaderGroup();

		ShaderGroup& addStage(vk::ShaderStageFlagBits stage, const std::filesystem::path& path);
		ShaderGroup& build(vk::Device dev);

		vk::PipelineLayout getPipelineLayout();
		vk::DescriptorSetLayout getPerMaterialSetLayout();
		const std::vector<vk::DescriptorSetLayout>& getSetLayouts();
		vk::PipelineVertexInputStateCreateInfo getVertexInputStateCI();

	private:
		// Used to extract data for Reflection
		static constexpr uint32_t INVALID_SET_NUM = 7777;
		static constexpr uint32_t PER_FRAME_SET_NUM = 0;
		static constexpr uint32_t PER_PASS_SET_NUM = 1;
		static constexpr uint32_t PER_MATERIAL_SET_NUM = 2;
		static constexpr uint32_t PER_OBJECT_SET_NUM = 3;

		struct DescriptorSetLayoutData
		{
			uint32_t setNum = INVALID_SET_NUM;
			vk::DescriptorSetLayoutCreateInfo createInfo;
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
		void reflectDescriptorSets(const spv_reflect::ShaderModule& spvMod);
		void reflectPushConstantBlocks(const spv_reflect::ShaderModule& spvMod);
		void reflectVertexInputState(const spv_reflect::ShaderModule& spvMod);


	private:
		std::vector<StageInfo> m_stages;
		std::array<DescriptorSetLayoutData, 4> m_setLayoutsData;
		std::vector<vk::DescriptorSetLayout> m_setLayouts;
		std::vector<vk::PushConstantRange> m_pushConstantRanges;
		vk::PipelineVertexInputStateCreateInfo m_vertInputState;
		vk::PipelineLayout m_pipelineLayout;

		std::vector<std::function<void()>> m_deletionQueue;

	};

}

