#pragma once
#include "Application.h"

#include "GraphicsContext.h"
#include "Window.h"
#include "Utilities.h"

namespace Nagi
{

/*

Creates a triangle with minimal resources!
We are drawing it with the immediate buffer in the shader which
we index with the help of simple vertexID and instanceID

*/

class MinimalTriangleApp : public Application
{
public:
	MinimalTriangleApp(Window& window, GraphicsContext& gfxCon);
	~MinimalTriangleApp() = default;

	MinimalTriangleApp() = delete;
	MinimalTriangleApp& operator=(const Application&) = delete;

private:
	void createRenderPass();
	void createGraphicsPipeline(vk::RenderPass& compatibleRendPass);
	void createFramebuffers();

private:
	vk::UniqueRenderPass m_rendPass;
	vk::UniquePipeline m_gfxPipeline;
	std::vector<vk::UniqueFramebuffer> m_framebuffers;
	vk::Extent2D m_scExtent;
};


}
