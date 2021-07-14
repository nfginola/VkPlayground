#include "pch.h"
#include "Application/QuadApp.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>



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
		// Set up UBO
		createUBO();

		// Set up Texture
		loadImage();

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
			matProj[1][1] *= -1;
			//auto rot = glm::rotate(glm::mat4(1.f), glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
			auto matModel = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.f));
			frameConstants.mat = matProj * matView * matModel;



			// GPU BELOW
			auto frameRes = gfxCon.beginFrame();
			auto& cmd = frameRes.gfxCmdBuffer;


			// ================================================ Update UBO
			void* data;
			vmaMapMemory(m_gfxCon.getResourceAllocator(), m_frameData[frameRes.imageIdx].ubo.alloc, &data);
			memcpy(data, &frameConstants, sizeof(UBO));
			vmaUnmapMemory(m_gfxCon.getResourceAllocator(), m_frameData[frameRes.imageIdx].ubo.alloc);
			
			// reset push constant (to verify that we are using the UBO!)
			memset(&frameConstants, 0, sizeof(PushConstant));



			// Setup render pass info
			std::array<vk::ClearValue, 2> clearValues = {
				vk::ClearColorValue(std::array<float, 4>({0.f, 0.f, 0.f, 1.f})),
				vk::ClearDepthStencilValue( /*depth*/ 1.f, /*stencil*/ 0)
			};

			vk::RenderPassBeginInfo rpInfo(m_rendPass.get(), m_framebuffers[frameRes.imageIdx].get(), vk::Rect2D({ 0, 0 }, m_scExtent), clearValues);

	
			// Record command buffer
			cmd.begin(vk::CommandBufferBeginInfo());		// implicitly calls resetCommandBuffer

			cmd.beginRenderPass(rpInfo, {});
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_gfxPipeline.get());

			// Bind UBO after Pipeline
			//vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &get_current_frame().globalDescriptor, 0, nullptr);
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, m_frameData[frameRes.frameIdx].descriptorSet, {});

			// Bind texture
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 1, m_materialDescSet, {});

			// Flip viewport height https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
			// We set this on Graphics Pipeline instead of changing it dynamically
			//cmd.setViewport(0, vk::Viewport(0.f, (float)m_scExtent.height, m_scExtent.width, -(float)m_scExtent.height, 0.0, 1.0));

			// Bind vertex buffer
			std::array<vk::Buffer, 1> vbs{ m_vb.resource };
			std::array<vk::DeviceSize, 1> offsets{ 0 };
			cmd.bindVertexBuffers(0, vbs, offsets);

			cmd.bindIndexBuffer(m_ib.resource, 0, vk::IndexType::eUint32);

			// ===================================================== Push constants (command constants)
			cmd.pushConstants<PushConstant>(m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, { frameConstants });
			
			cmd.drawIndexed(6, 1, 0, 0, 0);
			//cmd.draw(6, 1, 0, 0);
			cmd.endRenderPass();

			cmd.end();

			// Setup submit info
			std::array<vk::PipelineStageFlags, 1> waitStages{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
			// Queue waits at just before this stage executes for the sem signal with a full mem barrier

			vk::SubmitInfo submitInfo(
				frameRes.sync.imageAvailableSemaphore,
				waitStages,
				cmd,
				frameRes.sync.renderFinishedSemaphore
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
	auto allocator = m_gfxCon.getResourceAllocator();

	vmaDestroyBuffer(allocator, m_vb.resource, m_vb.alloc);
	vmaDestroyBuffer(allocator, m_ib.resource, m_ib.alloc);

	for (int i = 0; i < GraphicsContext::s_maxFramesInFlight; ++i)
		vmaDestroyBuffer(allocator, m_frameData[i].ubo.resource, m_frameData[i].ubo.alloc);

	vmaDestroyImage(allocator, m_image.resource, m_image.alloc);

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
	//vk::Viewport vp = vk::Viewport(0.f, static_cast<float>(m_scExtent.height), static_cast<float>(m_scExtent.width), -static_cast<float>(m_scExtent.height), 0.0, 1.0);
	
	// We just decide to flip the projection Y to fix this instead of having to fix the viewport (which may affect renderpass etc.)
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

	// Change: We are now using pipeline layout for Push Constants and Descriptor Sets


	// 1. Push Constant
	vk::PushConstantRange pushRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstant));
	
	// Change: We now have to store the layout as it is used when pushing constants on the command buffer
	// watch out so you dont accidentally use the other constructor which takes in size! :)

	// Now added descriptor set layout 
	std::vector<vk::DescriptorSetLayout> compatibleLayouts{ m_descriptorSetLayout.get(), m_materialSetLayout.get() };			// Order in this array defines set numbers!! set 0, 1, 2, 3.. etc.
	//std::vector<vk::DescriptorSetLayout> compatibleLayouts{ m_materialSetLayout.get(), m_descriptorSetLayout.get() };
	m_pipelineLayout = dev.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, compatibleLayouts, pushRange));

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

	m_frameData.reserve(GraphicsContext::s_maxFramesInFlight);


	for (int i = 0; i < GraphicsContext::s_maxFramesInFlight; ++i)
	{
		m_frameData.push_back(FrameData{});
		Buffer buf;

		auto res = vmaCreateBuffer(m_gfxCon.getResourceAllocator(), (VkBufferCreateInfo*)&uboCI, &uboAllocCI, (VkBuffer*)&buf.resource, &buf.alloc, nullptr);
		if (res != VK_SUCCESS) throw std::runtime_error("Couldnt create vertex buffer");

		m_frameData[i].ubo = buf;;
	}


	//m_ubos.reserve(GraphicsContext::s_maxFramesInFlight);
	//for (int i = 0; i < GraphicsContext::s_maxFramesInFlight; ++i)
	//{
	//	Buffer buf;

	//	auto res = vmaCreateBuffer(m_gfxCon.getResourceAllocator(), (VkBufferCreateInfo*)&uboCI, &uboAllocCI, (VkBuffer*)&buf.resource, &buf.alloc, nullptr);
	//	if (res != VK_SUCCESS) throw std::runtime_error("Couldnt create vertex buffer");

	//	m_ubos.push_back(buf);
	//}


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

	// Per scene set (camera)
	std::array<vk::DescriptorSetLayoutBinding, 1> setLayoutBindings;
	setLayoutBindings[0].binding = 0;		// location within a set
	setLayoutBindings[0].descriptorType = vk::DescriptorType::eUniformBuffer;		
	setLayoutBindings[0].descriptorCount = 1;	// can be an array, but we have single UBO here :)
	setLayoutBindings[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
	setLayoutBindings[0].pImmutableSamplers = {};		// no immutable samplers (what are those?)

	vk::DescriptorSetLayoutCreateInfo layoutCI({}, setLayoutBindings);

	m_descriptorSetLayout =	m_gfxCon.getDevice().createDescriptorSetLayoutUnique(layoutCI);




	// Material set
	std::array<vk::DescriptorSetLayoutBinding, 1> texLayBinding;
	texLayBinding[0].binding = 0;
	texLayBinding[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	texLayBinding[0].descriptorCount = 1;
	texLayBinding[0].stageFlags = vk::ShaderStageFlagBits::eFragment;
	texLayBinding[0].pImmutableSamplers = {};

	vk::DescriptorSetLayoutCreateInfo matLayoutCI({}, texLayBinding);

	m_materialSetLayout = m_gfxCon.getDevice().createDescriptorSetLayoutUnique(matLayoutCI);



	




}

void QuadApp::createDescriptorPool()
{
	// Kinda coupled to Set Layout Binding (we have to make sure that we create a pool big enough to accomodate our needs)

	std::vector<vk::DescriptorPoolSize> descriptorPoolSizes{
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 2),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)		// for texture
	};


	vk::DescriptorPoolCreateInfo poolCI({}, 
		10,										// max sets, just set a value, this means we have room to allocate 10 DescriptorSet with the specified DescriptorPoolSize content in each set
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
		//m_descriptorSets.push_back(m_gfxCon.getDevice().allocateDescriptorSets(allocInfo)[0]);
		m_frameData[i].descriptorSet = m_gfxCon.getDevice().allocateDescriptorSets(allocInfo).front();

		// We have a set, and now we need to point it to the UBOs

		vk::DescriptorBufferInfo binfo(m_frameData[i].ubo.resource, 0, sizeof(UBO));
		vk::WriteDescriptorSet setWrite(m_frameData[i].descriptorSet, 0 /* we will write to binding 0 in this Set */, 0, vk::DescriptorType::eUniformBuffer, {}, binfo);

		m_gfxCon.getDevice().updateDescriptorSets(setWrite, {});
	}

	// Material set
	vk::DescriptorSetAllocateInfo texAllocInfo(m_descriptorPool.get(), m_materialSetLayout.get());
	m_materialDescSet = m_gfxCon.getDevice().allocateDescriptorSets(texAllocInfo).front();

	// Write to it (bind image and sampler)
	vk::DescriptorImageInfo imageInfo(m_sampler.get(), m_image.view.get(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet imageSetWrite(m_materialDescSet, 0, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo, {}, {});
	m_gfxCon.getDevice().updateDescriptorSets(imageSetWrite, {});

}

void QuadApp::loadImage()
{
	// Load image data
	int texWidth, texHeight, texChannels;

	stbi_uc* pixels = stbi_load("Resources/Textures/images.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
		throw std::runtime_error("Can't find the image resource!");

	size_t imageSize = texWidth * texHeight * sizeof(uint32_t);

	auto allocator = m_gfxCon.getResourceAllocator();


	// Create staging buffer 
	vk::BufferCreateInfo stagingCI({}, imageSize, vk::BufferUsageFlagBits::eTransferSrc);
	VmaAllocationCreateInfo stagingBufAlloc{};
	stagingBufAlloc.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	Buffer stagingBuffer;
	auto res = vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&stagingCI, &stagingBufAlloc, (VkBuffer*)&stagingBuffer.resource, &stagingBuffer.alloc, nullptr);
	if (res != VK_SUCCESS) throw std::runtime_error("Couldnt create vertex buffer");

	// Copy CPU data to staging buffer
	void* data = nullptr;
	vmaMapMemory(allocator, stagingBuffer.alloc, &data);
	memcpy(data, pixels, imageSize);
	vmaUnmapMemory(allocator, stagingBuffer.alloc);


	// Create texture
	auto texExtent = vk::Extent3D(texWidth, texHeight, 1);
	vk::Format imageFormat = vk::Format::eR8G8B8A8Srgb;

	vk::ImageCreateInfo imCI({},
		vk::ImageType::e2D, imageFormat,
		texExtent,
		1, 1,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
	);

	VmaAllocationCreateInfo texAlloc{};
	texAlloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	if (vmaCreateImage(allocator, (VkImageCreateInfo*)&imCI, &texAlloc, (VkImage*)&m_image.resource, &m_image.alloc, nullptr) != VK_SUCCESS)
		throw std::runtime_error("Could not create image");

	// Initial Layout of image is Undefined, we need to transition its layout!
	auto& uploadContext = m_gfxCon.getUploadContext();

	uploadContext.submitWork(
		[&](const vk::CommandBuffer& cmd)
		{
			vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

			vk::ImageMemoryBarrier barrierForTransfer(
				{},
				vk::AccessFlagBits::eTransferWrite,		// any transfer ops should wait until the image layout transition has occurred!
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eTransferDstOptimal,	// -> puts into linear layout, best for copying data from buffer to texture
				{},
				{},
				m_image.resource,
				range
			);
			
			// Transition layout through barrier (after availability ops and before visibility ops)
			cmd.pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eTransfer,
				{},
				{},
				{},
				barrierForTransfer
			);

			// Now we can do our transfer cmd
			vk::BufferImageCopy copyRegion({}, {}, {},
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
				{},
				texExtent
			);

			cmd.copyBufferToImage(stagingBuffer.resource, m_image.resource, vk::ImageLayout::eTransferDstOptimal, copyRegion);

				
			// Now we can transfer the image layout to optimal for shader usage
			vk::ImageMemoryBarrier barrierForReading(
				vk::AccessFlagBits::eTransferWrite,
				vk::AccessFlagBits::eShaderRead,
				vk::ImageLayout::eTransferDstOptimal,
				vk::ImageLayout::eShaderReadOnlyOptimal,
				{},
				{},
				m_image.resource,
				range
			);

			// Transition layout through barrier (after availability ops and before visibility ops)
			cmd.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eFragmentShader,		// guarantee for any future commands that happens after this
				{},
				{},
				{},
				barrierForReading
			);

		});


	// Create image view
	vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
	vk::ImageSubresourceRange subresRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

	vk::ImageViewCreateInfo viewCreateInfo({},
		m_image.resource,
		vk::ImageViewType::e2D,
		imageFormat,
		componentMapping,
		subresRange
	);

	m_image.view = m_gfxCon.getDevice().createImageViewUnique(viewCreateInfo);

	// Create sampler, allocate material set and bind combined image sampler 
	vk::SamplerCreateInfo sCI({});	// nearest and repeat
	m_sampler = m_gfxCon.getDevice().createSamplerUnique(sCI);

	// No need for staging buffer anymore
	vmaDestroyBuffer(allocator, stagingBuffer.resource, stagingBuffer.alloc);
}
	
	

}
