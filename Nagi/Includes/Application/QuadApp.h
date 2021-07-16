#pragma once
#include "Application.h"

namespace Nagi
{

/*

Creates a quad in world space with a proper camera and projection matrix.

Purpose
- Get familiar with Push Constants
- Get familiar with UBO binding through Descriptor sets

- Playground for abstractions


Process:

1. Create UBO per frame
2. Create Descriptor Set Layout --> Describe the resources in the sets we want to use (e.g uniform buffer, uniform buffer arrays, uniform sampler2D, etc.)

3. Create Descriptor Pool --> Supply information on "max amount of sets to allocate" and "how many descriptors of a certain type" is to be allocated in the pool

		createUBO();
		setupDescriptorSetLayout();
		createDescriptorPool();
		allocateDescriptorSets();

4. Load/create an image, bind it and sample from it.

*/

class QuadApp : public Application
{
public:
	QuadApp(Window& window, VulkanContext& gfxCon);
	~QuadApp();

	QuadApp() = delete;
	QuadApp& operator=(const Application&) = delete;
	
private:
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 color;

		constexpr static int s_bindingSlot = 0;

		static vk::VertexInputBindingDescription getBindingDescription()
		{
			// Binding on Vertex Buffer slot 0 hardcoded
			return 	vk::VertexInputBindingDescription{ s_bindingSlot, sizeof(Vertex) };
		}

		static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
		{
			std::array<vk::VertexInputAttributeDescription, 3> dscs;
			// 1st arg is the shader input location
			// 2nd arg is 0 because thats the bindinng number from where we take the data from! We hardcoded it to 0 as above
			dscs[0] = vk::VertexInputAttributeDescription(0, s_bindingSlot, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos));
			dscs[1] = vk::VertexInputAttributeDescription(1, s_bindingSlot, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv));
			dscs[2] = vk::VertexInputAttributeDescription(2, s_bindingSlot, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color));
			return dscs;
		}

	};


	struct PushConstant
	{
		glm::mat4 mat;
		glm::vec4 rand;
	};

	struct UBO
	{
		glm::mat4 mat;
		glm::vec4 bogus;
	};

	// Handle key down / just pressed
	class Keystate
	{
	public:
		// Called on GLFW key down
		void onPress()
		{
			m_isDown = true;
			if (m_justPressed)
				m_justPressed = false;
			else
				m_justPressed = true;
		};

		// Called on GLFW key release
		void onRelease()
		{
			m_isDown = false;
			m_justPressed = false;
		};

		bool isDown()
		{
			return m_isDown;
		};

		bool justPressed()
		{
			// Turns off isPressed after the first time its retrieved 
			bool ret = m_justPressed;
			m_justPressed = false;
			return ret;
		};

	private:
		bool m_justPressed = false;
		bool m_isDown = false;
	};

	struct FrameData
	{
		std::unique_ptr<Buffer> ubo;
		vk::DescriptorSet descriptorSet;
	};

private:
	void createRenderPass();
	void createGraphicsPipeline(vk::RenderPass& compatibleRendPass);
	void createFramebuffers();

	void createVertexIndexBuffer(VmaAllocator allocator);

	void createUBO();
	void setupDescriptorSetLayout();
	void createDescriptorPool();
	void allocateDescriptorSets();

	void loadImage();

	void createRenderModel();

private:
	vk::UniqueRenderPass m_rendPass;
	vk::UniquePipeline m_gfxPipeline;
	std::vector<vk::UniqueFramebuffer> m_framebuffers;
	vk::Extent2D m_scExtent;

	std::unique_ptr<Texture> m_image;
	vk::UniqueSampler m_sampler;
	vk::DescriptorSet m_materialDescSet;

	std::unique_ptr<Buffer> m_vb;
	std::unique_ptr<Buffer> m_ib;

	std::vector<FrameData> m_frameData;

	vk::UniquePipelineLayout m_pipelineLayout;

	vk::UniqueDescriptorSetLayout m_descriptorSetLayout;	// Needed for UBO thing
	vk::UniqueDescriptorSetLayout m_materialSetLayout;
	vk::UniqueDescriptorPool m_descriptorPool;


	std::vector<std::unique_ptr<RenderModel>> m_testModels;
	std::unique_ptr<Material> m_materialStorage;
	std::unique_ptr<Texture> m_texStorage;
};

}

