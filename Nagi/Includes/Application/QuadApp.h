#pragma once
#include "Application.h"

namespace Nagi
{

/*

Creates a quad and uses some Uniform Buffers and Push Constants!


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

private:
	void createRenderPass();
	void createGraphicsPipeline(vk::RenderPass& compatibleRendPass);
	void createFramebuffers();

	void createVertexIndexBuffer(VmaAllocator allocator);

private:
	vk::UniqueRenderPass m_rendPass;
	vk::UniquePipeline m_gfxPipeline;
	std::vector<vk::UniqueFramebuffer> m_framebuffers;
	vk::Extent2D m_scExtent;

	Buffer m_vb;
	Buffer m_ib;
};

}

