#include "pch.h"
#include "Application/MinimalTriangleWithBuffersApp.h"

namespace Nagi
{

MinimalTriangleWithBuffersApp::MinimalTriangleWithBuffersApp(Window& window, GraphicsContext& gfxCon) :
	Application(window, gfxCon)
{
	try
	{
		createRenderPass();
		createGraphicsPipeline(m_rendPass.get());
		createFramebuffers();

		createVertexIndexBuffer(gfxCon.getResourceAllocator());

		while (m_window.isRunning())
		{
			m_window.processEvents();

			auto frameRes = gfxCon.beginFrame();
			auto& cmd = frameRes.gfxCmdBuffer;

			// Setup render pass info
			std::array<vk::ClearValue, 2> clearValues = {
				vk::ClearColorValue(std::array<float, 4>({0.f, 0.f, 0.f, 1.f})),
				vk::ClearDepthStencilValue( /*depth*/ 1.f, /*stencil*/ 0)
			};

			vk::RenderPassBeginInfo rpInfo(m_rendPass.get(), m_framebuffers[frameRes.imageIdx].get(), vk::Rect2D({ 0, 0 }, m_scExtent), clearValues);

			// Record command buffer
			cmd->begin(vk::CommandBufferBeginInfo());		// implicitly calls resetCommandBuffer

			cmd->beginRenderPass(rpInfo, {});
			cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, m_gfxPipeline.get());

			// Bind vertex buffer
			std::array<vk::Buffer, 1> vbs{ m_vb.resource };
			std::array<vk::DeviceSize, 1> offsets{ 0 };
			cmd->bindVertexBuffers(0, vbs, offsets);

			cmd->bindIndexBuffer(m_ib.resource, 0, vk::IndexType::eUint32);

			cmd->draw(3, 1, 0, 0);
			cmd->endRenderPass();

			cmd->end();

			// Setup submit info
			std::array<vk::PipelineStageFlags, 1> waitStages{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
			// Queue waits at just before this stage executes for the sem signal with a full mem barrier

			vk::SubmitInfo submitInfo(
				frameRes.sync->imageAvailableSemaphore,
				waitStages,
				*cmd,
				frameRes.sync->renderFinishedSemaphore
			);

			gfxCon.submitQueue(submitInfo);
			gfxCon.endFrame();

		}

		// Idle to wait for GPU resources to stop being used before resource destruction
		gfxCon.getDevice().waitIdle();

	}
	catch (vk::SystemError& err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		assert(false);
	}
	catch (std::exception& err)
	{
		std::cout << "std::exception: " << err.what() << std::endl;
		assert(false);
	}
	catch (...)
	{
		std::cout << "Unknown error\n";
		assert(false);
	}

}

MinimalTriangleWithBuffersApp::~MinimalTriangleWithBuffersApp()
{
	vmaDestroyBuffer(m_gfxCon.getResourceAllocator(), m_vb.resource, m_vb.alloc);
	vmaDestroyBuffer(m_gfxCon.getResourceAllocator(), m_ib.resource, m_ib.alloc);
}

void MinimalTriangleWithBuffersApp::createRenderPass()
{
	/*
	Deps:
		gfxCon: getSwapchainImageFormat(), getDepthFormat(),
				getDevice (for resource creation)

	*/

	auto dev = m_gfxCon.getDevice();

	std::array<vk::AttachmentDescription, 2> attachmentDescs;

	// Color
	attachmentDescs[0] = vk::AttachmentDescription({},
		m_gfxCon.getSwapchainImageFormat(),		// format
		vk::SampleCountFlagBits::e1,			// sample
		vk::AttachmentLoadOp::eClear,			// attachment load/store op
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,		// stencil load/store op
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,			// enter renderpass in this layout
		vk::ImageLayout::ePresentSrcKHR			// exit renderpass in this layout
	);

	// Depth
	attachmentDescs[1] = vk::AttachmentDescription({},
		m_gfxCon.getDepthFormat(),
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eDontCare,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eDepthStencilAttachmentOptimal
	);

	// Reference to the attachment descriptions
	vk::AttachmentReference colorRef(0, vk::ImageLayout::eAttachmentOptimalKHR);				// 2nd arg -> layout to use during in subpass using this ref
	vk::AttachmentReference depthRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::SubpassDescription subpassDesc({}, vk::PipelineBindPoint::eGraphics, {}, colorRef, {}, &depthRef);

	//// Image layout is not transitiooning at the right time.. we have no color output stage anywhere here!
	//vk::SubpassDependency extInDep(
	//	VK_SUBPASS_EXTERNAL,
	//	0,
	//	// wait for prev until depth testing of prev frame finished (write on srcStages) before starting depth test on this frame 
	//	vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
	//	vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
	//	vk::AccessFlagBits::eDepthStencilAttachmentWrite,	// why only write? (stated on sync examples Khronos Group)
	//	vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead
	//);

	//// Not caring about depth (To try out the validation layer)
	//vk::SubpassDependency extInDep(
	//	VK_SUBPASS_EXTERNAL,
	//	0,
	//	vk::PipelineStageFlagBits::eColorAttachmentOutput,		
	//	vk::PipelineStageFlagBits::eColorAttachmentOutput,									
	//	{},		
	//	vk::AccessFlagBits::eColorAttachmentWrite
	//);

	// Corrected (sync validation WAW hazard)
	vk::SubpassDependency extInDep(
		VK_SUBPASS_EXTERNAL,
		0,
		// We have early AND late because the accessFlags only applies to explicitly specified stages (the mem access flags does NOT apply to the implicit stages that come with exec dep)
		vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests |	// wait for prev frame depth testing (happens in early and late frag test)
		vk::PipelineStageFlagBits::eColorAttachmentOutput,													// form exec dep chain with present sem

		vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests |	// halt this frames depth testing before the prev is done
		vk::PipelineStageFlagBits::eColorAttachmentOutput,													// form exec dep chain with present sem (note that we have mem dep for this (dst stage) on access mask!)

		{},	// Waiting on semaphore guarantees memory dependency already! No need to supply mem access to wait for in src stages (Once colorAttachmentOutput stage is opened up, all memory writes are visible
		// https://vulkan.lunarg.com/doc/view/1.2.141.2/linux/chunked_spec/chap6.html (6.4.2)

		vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead | // assign mem dep on the depth resource for which to halt
		vk::AccessFlagBits::eColorAttachmentWrite							// ensure that the layout transition happens after color output is unblocked but before color output writing through mem dep!
	);
	/*
		We can look at the WSI + External subpass sync with Pipeline Barriers instead:

		Barrier1 ( srcStage="semSignal",	dstStage=ColorOutput, srcAccessMask=ALL,	dstAccessMask=ALL						  )		--> Semaphore (full memory barrier)
		Barrier2 ( srcStage=ColorOutput,	dstStage=ColorOutput, srcAccessMask=0,		dstAccessMask=depthReadWrite | colorWrite )		--> Subpass dependency

		Subpass dependency is placed there with Barrier1 dst intersecting Barrier2 src to create an execution depedency chain.
		This in turns guarantees that the image layout transition implicitly done through the subpass dependency is done AFTER the swapchain image is acquired but before it used (e.g written to).

		Depth stages with read/write is added to also sync the depth image usage (fix depth WAW hazard)
	*/

	// Using implicit external subpass

	m_rendPass = dev.createRenderPassUnique(vk::RenderPassCreateInfo({}, attachmentDescs, subpassDesc, extInDep));

}

void MinimalTriangleWithBuffersApp::createGraphicsPipeline(vk::RenderPass& compatibleRendPass)
{
	auto dev = m_gfxCon.getDevice();
	m_scExtent = m_gfxCon.getSwapchainExtent();

	// Change: Now we load vsMinTri_buf (Buffer version of minimal triangle)
	auto vertBin = Nagi::Utils::readFile("compiled_shaders/vsMinTri_buf.spv");
	auto fragBin = Nagi::Utils::readFile("compiled_shaders/psMinTri_buf.spv");
	auto vertMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, vertBin.size(), reinterpret_cast<uint32_t*>(vertBin.data())));
	auto fragMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, fragBin.size(), reinterpret_cast<uint32_t*>(fragBin.data())));
	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageC = {
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertMod.get(), "main"),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragMod.get(), "main")
	};

	// Change: Now we fill the Input state 
	std::vector<vk::VertexInputBindingDescription> bindingDescs{ Vertex::getBindingDescription() };
	auto inputAttrDescs{ Vertex::getAttributeDescriptions() };
	vk::PipelineVertexInputStateCreateInfo vertInC({}, bindingDescs, inputAttrDescs);

	vk::PipelineInputAssemblyStateCreateInfo iaC({}, vk::PrimitiveTopology::eTriangleList);

	vk::Viewport vp = vk::Viewport(0.f, 0.f, static_cast<float>(m_scExtent.width), static_cast<float>(m_scExtent.height), 0.0, 1.0);
	vk::Rect2D scissor = vk::Rect2D({ 0, 0 }, m_scExtent);
	vk::PipelineViewportStateCreateInfo vpC({}, 1, &vp, 1, &scissor);	// no stencil

	vk::PipelineRasterizationStateCreateInfo rsC({},
		false,
		false,
		vk::PolygonMode::eFill,				// eLine topology for wireframe
		vk::CullModeFlagBits::eBack,
		vk::FrontFace::eCounterClockwise,	// default
		{}, {}, {}, {},						// depth bias
		1.0f								// line width
	);

	vk::PipelineMultisampleStateCreateInfo msC({},
		vk::SampleCountFlagBits::e1
		// everything else is off (sample shading and sample mask
	);

	vk::PipelineDepthStencilStateCreateInfo dsC({},
		true,
		true,
		vk::CompareOp::eLessOrEqual
		// everything else off (depth bound test args and stencil)
	);

	vk::PipelineColorBlendAttachmentState colorBlendAttachment(
		false,
		// blend op bogus (it is disabled so these values below dont matter)
		vk::BlendFactor::eOne,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eOne,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,

		// specifies which channels to 'let through' (enabled for writing)
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
	);

	vk::PipelineColorBlendStateCreateInfo cbC({},
		false,
		{},		// no logic Op (disabled)
		1,
		&colorBlendAttachment,
		{}		// we are not blending so this the blendConstants are irrelevant
	);

	// no descriptor sets or push constants yet
	// but we MUST supply a pipeline layout
	auto pipelineLayout = dev.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo());

	m_gfxPipeline = dev.createGraphicsPipelineUnique({},
		vk::GraphicsPipelineCreateInfo({},
			shaderStageC,
			&vertInC,
			&iaC,
			{},
			&vpC,
			&rsC,
			&msC,
			&dsC,
			&cbC,
			{},
			pipelineLayout.get(),
			compatibleRendPass,
			0
		)
	).value;
}

void MinimalTriangleWithBuffersApp::createFramebuffers()
{
	// Create framebuffers
	// resource deps: (1) sc view, (2) depth view (3) swapchain image count
	// Note: Can we remove these deps? sc image count dep may propagate to other per-frame resources.. (buffers to update, etc.)
	auto dev = m_gfxCon.getDevice();
	auto scExtent = m_gfxCon.getSwapchainExtent();
	uint32_t scImageCount = m_gfxCon.getSwapchainImageCount();
	auto scViews = m_gfxCon.getSwapchainViews();
	auto depthV = m_gfxCon.getDepthView();

	m_framebuffers.reserve(scImageCount);
	for (uint32_t i = 0; i < scImageCount; ++i)
	{
		std::array<vk::ImageView, 2> attachments{ scViews[i], depthV };
		vk::FramebufferCreateInfo fbc({}, m_rendPass.get(), attachments, scExtent.width, scExtent.height, 1);
		m_framebuffers.push_back(dev.createFramebufferUnique(fbc));
	}
}

void MinimalTriangleWithBuffersApp::createVertexIndexBuffer(VmaAllocator allocator)
{
	// CCW
	std::vector<Vertex> vertices{
		{ { 0.f, -0.5f, 0.f }, { 0.f, 1.f }, { 1.f, 0.f, 0.f } },
		{ { -0.5f, 0.5f, 0.f }, { 0.f, 0.f }, { 0.f, 1.f, 0.f } },
		{ { 0.5f, 0.5f, 0.f }, { 1.f, 0.f }, { 0.f, 0.f, 1.f } },
	};

	std::vector<uint32_t> indices{
		0, 1, 2
	};

	vk::BufferCreateInfo vertBufCI({}, vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eVertexBuffer);
	VmaAllocationCreateInfo vertBufAllocCI{};
	vertBufAllocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;		// We have to map and unmap to fill the data

	// Similar for index buffer
	vk::BufferCreateInfo indexBufCI({}, indices.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eIndexBuffer);
	VmaAllocationCreateInfo indexBufAllocCI{};
	indexBufAllocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	auto res = vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&vertBufCI, &vertBufAllocCI, (VkBuffer*)&m_vb.resource, &m_vb.alloc, nullptr);
	if (res != VK_SUCCESS) throw std::runtime_error("Couldnt create vertex buffer");
	res = vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&indexBufCI, &indexBufAllocCI, (VkBuffer*)&m_ib.resource, &m_ib.alloc, nullptr);
	if (res != VK_SUCCESS) throw std::runtime_error("Couldnt create index buffer");

	// Copy data
	void* data = nullptr;
	vmaMapMemory(allocator, m_vb.alloc, &data);
	memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(allocator, m_vb.alloc);

	vmaMapMemory(allocator, m_ib.alloc, &data);
	memcpy(data, indices.data(), indices.size() * sizeof(uint32_t));
	vmaUnmapMemory(allocator, m_ib.alloc);

	// Traditionally, without VMA, this is done with a staging buffer which is host visible and the vertex buffer which is device local.
	// We would have to Map/Unmap (copy) the data onto the staging buffer where we then call a copy command on the GPU to copy it from the staging buffer to the vertex buffer.
	// Here we rely on VMA figuring that out. Responsbility of whether VMA does the staging copy or not is not our responsibility :)
}

}
