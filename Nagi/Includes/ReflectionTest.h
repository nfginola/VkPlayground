#pragma once
#include "Utilities.h"
#include "pch.h"
#include <spirv_reflect.h>
#include <vulkan/vulkan.hpp>

namespace Nagi
{

	// Table for converting format to size in bytes (https://github.com/KhronosGroup/SPIRV-Reflect/blob/master/examples/main_io_variables.cpp)
	static uint32_t formatSize(VkFormat format)
	{
		uint32_t result = 0;
		switch (format) {
		case VK_FORMAT_UNDEFINED: result = 0; break;
		case VK_FORMAT_R4G4_UNORM_PACK8: result = 1; break;
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16: result = 2; break;
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16: result = 2; break;
		case VK_FORMAT_R5G6B5_UNORM_PACK16: result = 2; break;
		case VK_FORMAT_B5G6R5_UNORM_PACK16: result = 2; break;
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16: result = 2; break;
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16: result = 2; break;
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16: result = 2; break;
		case VK_FORMAT_R8_UNORM: result = 1; break;
		case VK_FORMAT_R8_SNORM: result = 1; break;
		case VK_FORMAT_R8_USCALED: result = 1; break;
		case VK_FORMAT_R8_SSCALED: result = 1; break;
		case VK_FORMAT_R8_UINT: result = 1; break;
		case VK_FORMAT_R8_SINT: result = 1; break;
		case VK_FORMAT_R8_SRGB: result = 1; break;
		case VK_FORMAT_R8G8_UNORM: result = 2; break;
		case VK_FORMAT_R8G8_SNORM: result = 2; break;
		case VK_FORMAT_R8G8_USCALED: result = 2; break;
		case VK_FORMAT_R8G8_SSCALED: result = 2; break;
		case VK_FORMAT_R8G8_UINT: result = 2; break;
		case VK_FORMAT_R8G8_SINT: result = 2; break;
		case VK_FORMAT_R8G8_SRGB: result = 2; break;
		case VK_FORMAT_R8G8B8_UNORM: result = 3; break;
		case VK_FORMAT_R8G8B8_SNORM: result = 3; break;
		case VK_FORMAT_R8G8B8_USCALED: result = 3; break;
		case VK_FORMAT_R8G8B8_SSCALED: result = 3; break;
		case VK_FORMAT_R8G8B8_UINT: result = 3; break;
		case VK_FORMAT_R8G8B8_SINT: result = 3; break;
		case VK_FORMAT_R8G8B8_SRGB: result = 3; break;
		case VK_FORMAT_B8G8R8_UNORM: result = 3; break;
		case VK_FORMAT_B8G8R8_SNORM: result = 3; break;
		case VK_FORMAT_B8G8R8_USCALED: result = 3; break;
		case VK_FORMAT_B8G8R8_SSCALED: result = 3; break;
		case VK_FORMAT_B8G8R8_UINT: result = 3; break;
		case VK_FORMAT_B8G8R8_SINT: result = 3; break;
		case VK_FORMAT_B8G8R8_SRGB: result = 3; break;
		case VK_FORMAT_R8G8B8A8_UNORM: result = 4; break;
		case VK_FORMAT_R8G8B8A8_SNORM: result = 4; break;
		case VK_FORMAT_R8G8B8A8_USCALED: result = 4; break;
		case VK_FORMAT_R8G8B8A8_SSCALED: result = 4; break;
		case VK_FORMAT_R8G8B8A8_UINT: result = 4; break;
		case VK_FORMAT_R8G8B8A8_SINT: result = 4; break;
		case VK_FORMAT_R8G8B8A8_SRGB: result = 4; break;
		case VK_FORMAT_B8G8R8A8_UNORM: result = 4; break;
		case VK_FORMAT_B8G8R8A8_SNORM: result = 4; break;
		case VK_FORMAT_B8G8R8A8_USCALED: result = 4; break;
		case VK_FORMAT_B8G8R8A8_SSCALED: result = 4; break;
		case VK_FORMAT_B8G8R8A8_UINT: result = 4; break;
		case VK_FORMAT_B8G8R8A8_SINT: result = 4; break;
		case VK_FORMAT_B8G8R8A8_SRGB: result = 4; break;
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32: result = 4; break;
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32: result = 4; break;
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32: result = 4; break;
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: result = 4; break;
		case VK_FORMAT_A8B8G8R8_UINT_PACK32: result = 4; break;
		case VK_FORMAT_A8B8G8R8_SINT_PACK32: result = 4; break;
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32: result = 4; break;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32: result = 4; break;
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32: result = 4; break;
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32: result = 4; break;
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: result = 4; break;
		case VK_FORMAT_A2R10G10B10_UINT_PACK32: result = 4; break;
		case VK_FORMAT_A2R10G10B10_SINT_PACK32: result = 4; break;
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32: result = 4; break;
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32: result = 4; break;
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32: result = 4; break;
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: result = 4; break;
		case VK_FORMAT_A2B10G10R10_UINT_PACK32: result = 4; break;
		case VK_FORMAT_A2B10G10R10_SINT_PACK32: result = 4; break;
		case VK_FORMAT_R16_UNORM: result = 2; break;
		case VK_FORMAT_R16_SNORM: result = 2; break;
		case VK_FORMAT_R16_USCALED: result = 2; break;
		case VK_FORMAT_R16_SSCALED: result = 2; break;
		case VK_FORMAT_R16_UINT: result = 2; break;
		case VK_FORMAT_R16_SINT: result = 2; break;
		case VK_FORMAT_R16_SFLOAT: result = 2; break;
		case VK_FORMAT_R16G16_UNORM: result = 4; break;
		case VK_FORMAT_R16G16_SNORM: result = 4; break;
		case VK_FORMAT_R16G16_USCALED: result = 4; break;
		case VK_FORMAT_R16G16_SSCALED: result = 4; break;
		case VK_FORMAT_R16G16_UINT: result = 4; break;
		case VK_FORMAT_R16G16_SINT: result = 4; break;
		case VK_FORMAT_R16G16_SFLOAT: result = 4; break;
		case VK_FORMAT_R16G16B16_UNORM: result = 6; break;
		case VK_FORMAT_R16G16B16_SNORM: result = 6; break;
		case VK_FORMAT_R16G16B16_USCALED: result = 6; break;
		case VK_FORMAT_R16G16B16_SSCALED: result = 6; break;
		case VK_FORMAT_R16G16B16_UINT: result = 6; break;
		case VK_FORMAT_R16G16B16_SINT: result = 6; break;
		case VK_FORMAT_R16G16B16_SFLOAT: result = 6; break;
		case VK_FORMAT_R16G16B16A16_UNORM: result = 8; break;
		case VK_FORMAT_R16G16B16A16_SNORM: result = 8; break;
		case VK_FORMAT_R16G16B16A16_USCALED: result = 8; break;
		case VK_FORMAT_R16G16B16A16_SSCALED: result = 8; break;
		case VK_FORMAT_R16G16B16A16_UINT: result = 8; break;
		case VK_FORMAT_R16G16B16A16_SINT: result = 8; break;
		case VK_FORMAT_R16G16B16A16_SFLOAT: result = 8; break;
		case VK_FORMAT_R32_UINT: result = 4; break;
		case VK_FORMAT_R32_SINT: result = 4; break;
		case VK_FORMAT_R32_SFLOAT: result = 4; break;
		case VK_FORMAT_R32G32_UINT: result = 8; break;
		case VK_FORMAT_R32G32_SINT: result = 8; break;
		case VK_FORMAT_R32G32_SFLOAT: result = 8; break;
		case VK_FORMAT_R32G32B32_UINT: result = 12; break;
		case VK_FORMAT_R32G32B32_SINT: result = 12; break;
		case VK_FORMAT_R32G32B32_SFLOAT: result = 12; break;
		case VK_FORMAT_R32G32B32A32_UINT: result = 16; break;
		case VK_FORMAT_R32G32B32A32_SINT: result = 16; break;
		case VK_FORMAT_R32G32B32A32_SFLOAT: result = 16; break;
		case VK_FORMAT_R64_UINT: result = 8; break;
		case VK_FORMAT_R64_SINT: result = 8; break;
		case VK_FORMAT_R64_SFLOAT: result = 8; break;
		case VK_FORMAT_R64G64_UINT: result = 16; break;
		case VK_FORMAT_R64G64_SINT: result = 16; break;
		case VK_FORMAT_R64G64_SFLOAT: result = 16; break;
		case VK_FORMAT_R64G64B64_UINT: result = 24; break;
		case VK_FORMAT_R64G64B64_SINT: result = 24; break;
		case VK_FORMAT_R64G64B64_SFLOAT: result = 24; break;
		case VK_FORMAT_R64G64B64A64_UINT: result = 32; break;
		case VK_FORMAT_R64G64B64A64_SINT: result = 32; break;
		case VK_FORMAT_R64G64B64A64_SFLOAT: result = 32; break;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32: result = 4; break;
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: result = 4; break;

		default:
			break;
		}
		return result;
	}

	// Used to extract data for Reflection
	struct DescriptorSetLayoutData
	{
		uint32_t setID;
		vk::DescriptorSetLayoutCreateInfo ci;
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
	};

	void reflectShader(std::vector<uint8_t> bin)
	{
		// this below can be done through Reflection
		/*
			meaning we can get

			- PipelineVertexInputStateCreateInfo (almost fully, partially because BindingDescription is hardcoded)

			- Set Layouts Fully
			- Push Constant Block Fully
				--> PipelineLayout fully


			What we want:

			ShaderSet defaultLit;
			defaultLit.setShader(VS, vert.spv)
			defaultLit.setShader(FS, frag.spv)
			defaultLit.build()

			...
			pipelineBuilder builder;
			...
			builder.setShaders(defaultLit)	--> sets PipelineLayout and Input Layout

			...



		*/

	
		/*

		Important assumptions!
		- No gaps in binding numbers!

		*/

		spv_reflect::ShaderModule spvMod(bin);

		uint32_t setCount;
		spvMod.EnumerateDescriptorSets(&setCount, nullptr);

		std::vector<DescriptorSetLayoutData> setLayouts;
		setLayouts.resize(setCount);

		SpvReflectResult res;
		for (uint32_t setNum = 0; setNum < setCount; ++setNum)
		{
			auto setRefl = spvMod.GetDescriptorSet(setNum, &res);
			if (res != SPV_REFLECT_RESULT_SUCCESS)
				continue;

			// Prep for data extraction
			auto& finalSetLayout = setLayouts[setNum];
			finalSetLayout.bindings.resize(setRefl->binding_count);

			for (uint32_t bindingNum = 0; bindingNum < setRefl->binding_count; ++bindingNum)
			{
				auto bindingRefl = spvMod.GetDescriptorBinding(bindingNum, setNum, &res);
				assert(res == SPV_REFLECT_RESULT_SUCCESS);

				// Assemble binding data so we can create a DescriptorSetLayout
				auto& bindingExtract = finalSetLayout.bindings[bindingNum];
				bindingExtract.binding = bindingRefl->binding;
				bindingExtract.descriptorType = static_cast<vk::DescriptorType>(bindingRefl->descriptor_type);
				bindingExtract.stageFlags = static_cast<vk::ShaderStageFlagBits>(spvMod.GetShaderStage());

				// Get the descriptor count for this binding (array count)
				bindingExtract.descriptorCount = 1;
				for (uint32_t i_dim = 0; i_dim < bindingRefl->array.dims_count; ++i_dim)
					bindingExtract.descriptorCount *= bindingRefl->array.dims[i_dim];
			}

			finalSetLayout.setID = setNum;
			finalSetLayout.ci = vk::DescriptorSetLayoutCreateInfo({}, finalSetLayout.bindings);
		}


		// Reflect push constants
		uint32_t pcbCount;
		spvMod.EnumeratePushConstantBlocks(&pcbCount, nullptr);

		std::vector<vk::PushConstantRange> pushConstantRanges;
		pushConstantRanges.resize(pcbCount);

		for (uint32_t pcbIdx = 0; pcbIdx < pcbCount; ++pcbIdx)
		{
			auto pcbRefl = spvMod.GetPushConstantBlock(pcbIdx, &res);
			assert(res == SPV_REFLECT_RESULT_SUCCESS);

			pushConstantRanges[pcbIdx] = vk::PushConstantRange(static_cast<vk::ShaderStageFlagBits>(spvMod.GetShaderStage()), pcbRefl->offset, pcbRefl->size);
		}

		// Trying out, reflect Input (input layout)
		// YES! We have enough information to create the VertexInputAttributeDescriptions! (e.g inPos, inUV, etc.)
		//  ---- BUT, we still need to specify VertexInputBindingDescription (VB binding slot, stride and input rate (vertex or instance))
		//  ---- This is fine though! Its just a little extra metadata
		// https://github.com/KhronosGroup/SPIRV-Reflect/blob/master/examples/main_io_variables.cpp
		// We can use the simplifying assumptions and override this if needed in the future when we get a more complex application
		/*

		// Reflect Input Layout
		// Simplifying assumptions:
		   // - All vertex input attributes are sourced from a single vertex buffer,
		   //   bound to VB slot 0.
		   // - Each vertex's attribute are laid out in ascending order by location.
		   // - The format of each attribute matches its usage in the shader;
		   //   float4 -> VK_FORMAT_R32G32B32A32_FLOAT, etc. No attribute compression is applied.
		   // - All attributes are provided per-vertex, not per-instance.

		*/

		if (spvMod.GetShaderStage() == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
		{
			uint32_t ivCount;
			spvMod.EnumerateInputVariables(&ivCount, nullptr);

			std::vector<vk::VertexInputAttributeDescription> inputAttrDescs;
			inputAttrDescs.resize(ivCount);

			// Hardcoded assumptions
			/*
				- All input attr is on VB slot 0
				- Each vertex attr is laid out in ascending order by loc (0, 1, 2...)
				- All attr provided per vertex
				- 16 byte alignment for each input attribute (default behaviour of offsetof?)
			*/
			uint32_t vbBindingSlot = 0;

			// Get in location order (0..n)
			uint32_t stride = 0;
			for (uint32_t ivLoc = 0; ivLoc < ivCount; ++ivLoc)
			{
				auto ivRefl = spvMod.GetInputVariableByLocation(ivLoc, &res);
				assert(res == SPV_REFLECT_RESULT_SUCCESS);

				auto& inputAttrDesc = inputAttrDescs[ivLoc];
				inputAttrDesc.location = ivRefl->location;
				inputAttrDesc.format = static_cast<vk::Format>(ivRefl->format);
				inputAttrDesc.binding = vbBindingSlot;

				inputAttrDesc.offset = stride;
				stride += getAlignedSize(formatSize(static_cast<VkFormat>(inputAttrDesc.format)), 16);	// 16 byte aligned
			}

			// Hardcoded assumption above!
			// We should probably make sure that this is overridable in the future when we want to use e.g per instance data
			vk::VertexInputBindingDescription inputBindingDesc(vbBindingSlot, stride, vk::VertexInputRate::eVertex);

			// Now we have enough to create a vk::PipelineInputStateCreateInfo on top of vk::PipelineLayout (Push Constant + Descriptor Sets)!
			// We could even set up a validator between stages (VS to next, GS to next, etc. to validate Input and Output variables here and assert)

		}




		std::cout << "yeah\n";


	}

}


