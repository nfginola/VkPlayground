#include "pch.h"
#include "Application/QuadApp.h"



namespace Nagi
{

QuadApp::QuadApp(Window& window, GraphicsContext& gfxCon) :
	Application(window, gfxCon)
{
	glm::vec3 pos{ 0.f, 1.f, 1.f };

	std::array<Keystate, 5> keystates;
	auto& aKey = keystates[0];
	auto& dKey = keystates[1];
	auto& wKey = keystates[2];
	auto& sKey = keystates[3];
	auto& eKey = keystates[4];

	window.setKeyCallback([&keystates](int key, int scancode, int action, int mods)
		{
			static std::function handleFunc = [&action, &keystates](int keystateID)
			{
				if (action == GLFW_PRESS)
					keystates[keystateID].onPress();
				else if (action == GLFW_RELEASE)
					keystates[keystateID].onRelease();
			};


			if (key == GLFW_KEY_A)
				handleFunc(0);
			else if (key == GLFW_KEY_D)
				handleFunc(1);
			else if (key == GLFW_KEY_W)
				handleFunc(2);
			else if (key == GLFW_KEY_S)
				handleFunc(3);
			else if (key == GLFW_KEY_E)
				handleFunc(4);
		});



	try
	{
		createUBO();
		setupDescriptorSetLayout();
		createDescriptorPool();
		allocateDescriptorSets();

		createRenderPass();
		createGraphicsPipeline(m_rendPass.get());
		createFramebuffers();

		createVertexIndexBuffer(gfxCon.getResourceAllocator());


		while (m_window.isRunning())
		{
			m_window.processEvents();

			// Update camera (no frame-time fix for now)
			if (aKey.isDown())
				pos[0] += -0.1f;
			else if (dKey.isDown())
				pos[0] += 0.1f;
			else if (wKey.isDown())
				pos[2] += -0.1f;
			else if (sKey.isDown())
				pos[2] += 0.1f;

			if (eKey.justPressed())
				std::cout << "hello from E world..\n";

			// Update data for shader
			PushConstant frameConstants{};
			frameConstants.rand = glm::vec4(0.f, 1.f, 0.f, 1.f);
			auto matView = glm::lookAtRH(pos, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
			auto matProj = glm::perspectiveRH(glm::radians(70.f), (float)m_scExtent.width / m_scExtent.height, 0.01f, 50.f);
			//auto rot = glm::rotate(glm::mat4(1.f), glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
			auto matModel = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.f));
			frameConstants.mat = matProj * matView * matModel;



			// GPU BELOW
			auto frameRes = gfxCon.beginFrame();
			auto& cmd = frameRes.gfxCmdBuffer;


			// ================================================ Update UBO
			void* data;
			vmaMapMemory(m_gfxCon.getResourceAllocator(), m_ubos[frameRes.imageIdx].alloc, &data);
			memcpy(data, &frameConstants, sizeof(UBO));
			vmaUnmapMemory(m_gfxCon.getResourceAllocator(), m_ubos[frameRes.imageIdx].alloc);
			
			// reset push constant (to verify that we are using the UBO!)
			memset(&frameConstants, 0, sizeof(PushConstant));



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

			// Bind UBO after Pipeline
			//vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &get_current_frame().globalDescriptor, 0, nullptr);
			cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, m_descriptorSets[frameRes.frameIdx], {});


			// Flip viewport height https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
			// We set this on Graphics Pipeline instead of changing it dynamically
			//cmd->setViewport(0, vk::Viewport(0.f, (float)m_scExtent.height, m_scExtent.width, -(float)m_scExtent.height, 0.0, 1.0));

			// Bind vertex buffer
			std::array<vk::Buffer, 1> vbs{ m_vb.resource };
			std::array<vk::DeviceSize, 1> offsets{ 0 };
			cmd->bindVertexBuffers(0, vbs, offsets);

			cmd->bindIndexBuffer(m_ib.resource, 0, vk::IndexType::eUint32);

			// ===================================================== Push constants (command constants)
			cmd->pushConstants<PushConstant>(m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, { frameConstants });
			
			cmd->drawIndexed(6, 1, 0, 0, 0);
			//cmd->draw(6, 1, 0, 0);
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

QuadApp::~QuadApp()
{
	vmaDestroyBuffer(m_gfxCon.getResourceAllocator(), m_vb.resource, m_vb.alloc);
	vmaDestroyBuffer(m_gfxCon.getResourceAllocator(), m_ib.resource, m_ib.alloc);

	for (int i = 0; i < GraphicsContext::s_maxFramesInFlight; ++i)
		vmaDestroyBuffer(m_gfxCon.getResourceAllocator(), m_ubos[i].resource, m_ubos[i].alloc);


}

void QuadApp::createRenderPass()
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


	// Corrected (sync validation WAW hazard)
	vk::SubpassDependency extInDep(
		VK_SUBPASS_EXTERNAL,
		0,
	
		vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests |
		vk::PipelineStageFlagBits::eColorAttachmentOutput,												

		vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests |
		vk::PipelineStageFlagBits::eColorAttachmentOutput,												

		{},	
		vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead | 
		vk::AccessFlagBits::eColorAttachmentWrite						
	);

	// Using implicit external subpass

	m_rendPass = dev.createRenderPassUnique(vk::RenderPassCreateInfo({}, attachmentDescs, subpassDesc, extInDep));

}

void QuadApp::createGraphicsPipeline(vk::RenderPass& compatibleRendPass)
{
	auto dev = m_gfxCon.getDevice();
	m_scExtent = m_gfxCon.getSwapchainExtent();

	auto vertBin = Nagi::Utils::readFile("compiled_shaders/vertQuad.spv");
	auto fragBin = Nagi::Utils::readFile("compiled_shaders/fragQuad.spv");
	auto vertMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, vertBin.size(), reinterpret_cast<uint32_t*>(vertBin.data())));
	auto fragMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, fragBin.size(), reinterpret_cast<uint32_t*>(fragBin.data())));
	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageC = {
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertMod.get(), "main"),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragMod.get(), "main")
	};

	std::vector<vk::VertexInputBindingDescription> bindingDescs{ Vertex::getBindingDescription() };
	auto inputAttrDescs{ Vertex::getAttributeDescriptions() };
	vk::PipelineVertexInputStateCreateInfo vertInC({}, bindingDescs, inputAttrDescs);

	vk::PipelineInputAssemblyStateCreateInfo iaC({}, vk::PrimitiveTopology::eTriangleList);
	
	// viewports origin is at top left (0, 0)
	// https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
	// Flip viewport height https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
	vk::Viewport vp = vk::Viewport(0.f, static_cast<float>(m_scExtent.height), m_scExtent.width, -static_cast<float>(m_scExtent.height), 0.0, 1.0);

	//vk::Viewport vp = vk::Viewport(0.f, 0.f, static_cast<float>(m_scExtent.width), static_cast<float>(m_scExtent.height), 0.0, 1.0);

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

	// Change: We are now using pipeline layout for Push Constants and Descriptor Sets


	// 1. Push Constant
	vk::PushConstantRange pushRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstant));
	
	// Change: We now have to store the layout as it is used when pushing constants on the command buffer
	// watch out so you dont accidentally use the other constructor which takes in size! :)

	// Now added descriptor set layout 
	m_pipelineLayout = dev.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, m_descriptorSetLayout.get(), pushRange));

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
			m_pipelineLayout.get(),
			compatibleRendPass,
			0
		)
	).value;
}

void QuadApp::createFramebuffers()
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

void QuadApp::createVertexIndexBuffer(VmaAllocator allocator)
{
	// Local space (RH)
	std::vector<Vertex> vertices{
		{ { -0.5f, 0.5f, 0.f }, { 0.f, 0.f }, { 1.f, 0.f, 0.f } },
		{ { 0.5f, 0.5f, 0.f }, { 1.f, 0.f }, { 1.f, 0.f, 0.f } },
		{ { 0.5f, -0.5f, 0.f }, { 1.f, 1.f }, { 0.f, 1.f, 0.f } },
		{ { -0.5f, -0.5f, 0.f }, { 0.f, 1.f }, { 0.f, 0.f, 1.f } }
	};

	// CCW
	std::vector<uint32_t> indices{
		0, 2, 1,
		0, 3, 2
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

void QuadApp::createUBO()
{
	vk::BufferCreateInfo uboCI({}, sizeof(UBO), vk::BufferUsageFlagBits::eUniformBuffer);
	VmaAllocationCreateInfo uboAllocCI{};
	uboAllocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;		// We have to map and unmap to fill the data

	m_ubos.reserve(GraphicsContext::s_maxFramesInFlight);
	for (int i = 0; i < GraphicsContext::s_maxFramesInFlight; ++i)
	{
		Buffer buf;

		auto res = vmaCreateBuffer(m_gfxCon.getResourceAllocator(), (VkBufferCreateInfo*)&uboCI, &uboAllocCI, (VkBuffer*)&buf.resource, &buf.alloc, nullptr);
		if (res != VK_SUCCESS) throw std::runtime_error("Couldnt create vertex buffer");

		m_ubos.push_back(buf);
	}


}

void QuadApp::setupDescriptorSetLayout()
{
	// Layout bindings for this SET
	/*
		example set:

		set 0:
			uniform UBOMats mats
			uniform sampler2D sampler
			uniform unt texIDs[50]

		DescriptorSetLayoutBinding is a descriptor for exactly one resource in that set 

		
		Only coupling is to Shader
	*/
	std::array<vk::DescriptorSetLayoutBinding, 1> setLayoutBindings;
	setLayoutBindings[0].binding = 0;		// location within a set
	setLayoutBindings[0].descriptorType = vk::DescriptorType::eUniformBuffer;		
	setLayoutBindings[0].descriptorCount = 1;	// can be an array, but we have one UBO now :)
	setLayoutBindings[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
	setLayoutBindings[0].pImmutableSamplers = {};		// no immutable samplers (what are those?)

	vk::DescriptorSetLayoutCreateInfo layoutCI({}, setLayoutBindings);

	m_descriptorSetLayout =	m_gfxCon.getDevice().createDescriptorSetLayoutUnique(layoutCI);

}

void QuadApp::createDescriptorPool()
{
	// Not coupled to anything

	std::array<vk::DescriptorPoolSize, 1> descriptorPoolSizes;

	// Mirror the Set Layout Binding
	descriptorPoolSizes[0].type = vk::DescriptorType::eUniformBuffer;
	descriptorPoolSizes[0].descriptorCount = 2;		// Allow two Uniform Buffer handles

	vk::DescriptorPoolCreateInfo poolCI({}, 
		2,										// max sets, just set a value, this means we have room to allocate 10 DescriptorSet with the specified DescriptorPoolSize content in each set
		descriptorPoolSizes
	);
	
	m_descriptorPool = m_gfxCon.getDevice().createDescriptorPoolUnique(poolCI);

}

void QuadApp::allocateDescriptorSets()
{
	// Coupling is the Set Layout and Pool

	vk::DescriptorSetAllocateInfo allocInfo(m_descriptorPool.get(), m_descriptorSetLayout.get());
	
	for (int i = 0; i < GraphicsContext::s_maxFramesInFlight; ++i)
	{
		m_descriptorSets.push_back(m_gfxCon.getDevice().allocateDescriptorSets(allocInfo)[0]);

		// We have a set, and now we need to point it to the UBOs

		vk::DescriptorBufferInfo binfo(m_ubos[i].resource, 0, sizeof(UBO));
		vk::WriteDescriptorSet setWrite(m_descriptorSets[i], 0 /* we will write to binding 0 in this Set */, 0, vk::DescriptorType::eUniformBuffer, {}, binfo);

		m_gfxCon.getDevice().updateDescriptorSets(setWrite, {});
	}

}
	
	

}
