#pragma once
#include "Application.h"

namespace Nagi
{

/*

Creates a quad in world space with a proper camera and projection matrix.

Purpose
- Get familiar with Push Constants
- Get familiar with UBO binding through Descriptor sets

*/

class QuadApp : public Application
{
public:
	QuadApp(Window& window, GraphicsContext& gfxCon);
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

	struct Buffer
	{
		VmaAllocation alloc;
		vk::Buffer resource;
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

private:
	void createRenderPass();
	void createGraphicsPipeline(vk::RenderPass& compatibleRendPass);
	void createFramebuffers();

	void createVertexIndexBuffer(VmaAllocator allocator);

	void createUBO();
	void setupDescriptorSetLayout();
	void createDescriptorPool();
	void allocateDescriptorSets();

private:
	vk::UniqueRenderPass m_rendPass;
	vk::UniquePipeline m_gfxPipeline;
	std::vector<vk::UniqueFramebuffer> m_framebuffers;
	vk::Extent2D m_scExtent;

	Buffer m_vb;
	Buffer m_ib;

	// We need one for each frame
	std::vector<Buffer> m_ubos;
	std::vector<vk::DescriptorSet> m_descriptorSets;

	vk::UniquePipelineLayout m_pipelineLayout;		// Needed for Push Constant

	vk::UniqueDescriptorSetLayout m_descriptorSetLayout;	// Needed for UBO thing
	vk::UniqueDescriptorPool m_descriptorPool;
};

}

