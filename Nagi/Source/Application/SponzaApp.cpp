#include "pch.h"
#include "Application/SponzaApp.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include "Camera.h"

namespace Nagi
{

SponzaApp::SponzaApp(Window& window, VulkanContext& gfxCon) :
	Application(window, gfxCon)
{
	glm::vec3 pos{ 0.f, 10.f, 1.f };

	std::array<Keystate, 8> keystates;
	auto& aKey = keystates[0];
	auto& dKey = keystates[1];
	auto& wKey = keystates[2];
	auto& sKey = keystates[3];
	auto& eKey = keystates[4];
	auto& qKey = keystates[5];
	auto& spaceKey = keystates[6];
	auto& shiftKey = keystates[7];

	window.setKeyCallback(
		[&keystates](GLFWwindow* window, int key, int scancode, int action, int mods)
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
			if (key == GLFW_KEY_D)
				handleFunc(1);
			if (key == GLFW_KEY_W)
				handleFunc(2);
			if (key == GLFW_KEY_S)
				handleFunc(3);
			if (key == GLFW_KEY_E)
				handleFunc(4);
			if (key == GLFW_KEY_Q)
				handleFunc(5);
			if (key == GLFW_KEY_SPACE)
				handleFunc(6);
			if (key == GLFW_KEY_LEFT_SHIFT)
				handleFunc(7);
		});

	auto scExtent = m_gfxCon.getSwapchainExtent();
	Camera fpsCam((float)scExtent.width / scExtent.height, 80);

	window.setMouseButtonCallback(
		[&](GLFWwindow* window, int button, int action, int mods)
		{
			if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);


		});

	bool firstTime = true;
	static int prevX = 0;
	static int prevY = 0;
	window.setMouseCursorCallback(
		[&fpsCam, &firstTime](GLFWwindow* window, int xPos, int yPos)
		{
			if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
			{
				if (firstTime)
				{
					prevX = xPos;
					prevY = yPos;
					firstTime = false;
				}
				
				int dx = xPos - prevX;
				int dy = -(yPos - prevY);	// Down is positive in Screenspace, we flip it

				fpsCam.rotateCamera(dx, dy, 0.16);

				prevX = xPos;
				prevY = yPos;
			}
			else
			{
				prevX = xPos;
				prevY = yPos;
			}


		});


	// :)
	try
	{
		m_defRenderPass = ezTmp::createDefaultRenderPass(m_gfxCon);
		m_defFramebuffers = ezTmp::createDefaultFramebuffers(m_gfxCon, m_defRenderPass.get());

		// Allocate pool for descriptors
		createDescriptorPool();

		// =========== Load Resources

		// Create UBOs that will be used in this App
		createUBOs();

		// Set up Texture
		loadTextures();

		vk::SamplerCreateInfo sCI({}, vk::Filter::eLinear, vk::Filter::eLinear);
		m_commonSampler = m_gfxCon.getDevice().createSamplerUnique(sCI);


		// ============ Setup the layout for our 4 sets and possible push constants (for PipelineLayout)
		setupDescriptorSetLayouts();
		configurePushConstantRange();

		// Setup descriptor sets
		// Engine Global (Set 0) (e.g Camera)
		// Per Pass (Set 1)
		// Per Material (Set 2)
		// Per Object (Set 3)
		allocateDescriptorSets();

		// ============
		createGraphicsPipeline();

		// ======== Load scene data
		createRenderModels();

		loadExternalModel("Resources/Objs/sponza/sponza.obj");
		loadExternalModel("Resources/Objs/nanosuit/nanosuit.obj");

		while (m_window.isRunning())
		{
			m_window.processEvents();

			// Update camera (no frame-time fix for now)
			if (aKey.isDown())
			{
				fpsCam.moveDirLeft();
			}
			else if (dKey.isDown())
			{
				fpsCam.moveDirRight();
			}
			if (wKey.isDown())
			{
				fpsCam.moveDirForward();
			}
			else if (sKey.isDown())
			{
				fpsCam.moveDirBackward();
			}

			if (spaceKey.isDown())
				fpsCam.moveDirUp();
			else if (shiftKey.isDown())
				fpsCam.moveDirDown();

			fpsCam.update(0.016);


			// Update data for shader
			GPUCameraData cameraData{};
			cameraData.viewMat = fpsCam.getViewMatrix();
			cameraData.projectionMat = fpsCam.getProjectionMatrix();
			cameraData.viewProjectionMat = fpsCam.getProjectionMatrix() * fpsCam.getViewMatrix();

			PushConstantData perObjectData{};
			auto matModel = 
				glm::translate(glm::mat4(1.f), glm::vec3(0.f, 2.5f, 0.f)) * 
				glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f)) * 
				glm::scale(glm::mat4(1.f), glm::vec3(8.f, 5.f, 8.f));

			perObjectData.modelMat = matModel;


			// GPU BELOW
			auto frameRes = gfxCon.beginFrame();
			auto& cmd = frameRes.gfxCmdBuffer;


			// ================================================ Update UBO
			m_engineFrameData[frameRes.imageIdx].cameraBuffer->putData(&cameraData, sizeof(GPUCameraData));


			// Setup render pass info
			std::array<vk::ClearValue, 2> clearValues = {
				vk::ClearColorValue(std::array<float, 4>({0.f, 0.f, 0.f, 1.f})),
				vk::ClearDepthStencilValue( /*depth*/ 1.f, /*stencil*/ 0)
			};

			vk::RenderPassBeginInfo rpInfo(m_defRenderPass.get(), m_defFramebuffers[frameRes.imageIdx].get(), vk::Rect2D({ 0, 0 }, scExtent), clearValues);



			// Record command buffer
			cmd.begin(vk::CommandBufferBeginInfo());


			// Bind engine wide resources (Camera, Set 0)
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_mainGfxPipelineLayout.get(), 0, m_engineFrameData[frameRes.frameIdx].descriptorSet, {});

			cmd.beginRenderPass(rpInfo, {});

			int tmpId = 0;
			const Material* lastMaterial = nullptr;
			for (const auto& model : m_loadedModels)
			{
				const auto& renderUnits = model->getRenderUnits();
				const auto& vb = model->getVertexBuffer();
				const auto& ib = model->getIndexBuffer();

				// Bind VB/IB - Lets not mind rebinding here even if next model might have same VB/IB
				std::array<vk::Buffer, 1> vbs{ vb };
				std::array<vk::DeviceSize, 1> offsets{ 0 };
				cmd.bindVertexBuffers(0, vbs, offsets);
				cmd.bindIndexBuffer(ib, 0, vk::IndexType::eUint32);
				
				// Sponza
				if (tmpId == 1)
				{
					auto newMatModel = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(0.07));
					perObjectData.modelMat = newMatModel;
				}
				// Nanosuit
				else if (tmpId == 2)
				{
					auto newMatModel = glm::translate(glm::mat4(1.f), glm::vec3(9.f, 0.f, -9.f)) * glm::scale(glm::mat4(1.f), glm::vec3(1.f));
					perObjectData.modelMat = newMatModel;
				}

				for (const auto& renderUnit : renderUnits)
				{
					const auto& mesh = renderUnit.getMesh();
					const auto& mat = renderUnit.getMaterial();

					if (&mat != lastMaterial)
					{
						cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mat.getPipeline());
						lastMaterial = &mat;
					}

					// Bind per material resources (Textures, Set 2)
					cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mat.getPipelineLayout(), 2, mat.getDescriptorSet(), {});

					// Bind model matrix (Buffer style, disabled_
					//perObjectFrameData.modelMatBuffer->putData(&perObjectData, sizeof(ObjectData));
					//cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mat.getPipelineLayout(), 3, perObjectFrameData.descriptorSet, {});

					// Bind model matrix
					// Push constants - We will be using push constants
					cmd.pushConstants<PushConstantData>(mat.getPipelineLayout(), vk::ShaderStageFlagBits::eVertex, 0, { perObjectData });

					cmd.drawIndexed(mesh.getNumIndices(), 1, mesh.getFirstIndex(), mesh.getVertexBufferOffset(), mesh.getVertexBufferOffset());
				}
				
				++tmpId;
			}
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

SponzaApp::~SponzaApp()
{


}

void SponzaApp::createUBOs()
{
	// Create UBO for engine set (Camera data)
	vk::BufferCreateInfo engineUBOCI({}, sizeof(GPUCameraData), vk::BufferUsageFlagBits::eUniformBuffer);
	VmaAllocationCreateInfo engineUBOAllocCI{};
	engineUBOAllocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	for (int i = 0; i < VulkanContext::s_maxFramesInFlight; ++i)
	{
		EngineFrameData dat{};
		dat.cameraBuffer = std::make_unique<Buffer>(m_gfxCon.getAllocator(), engineUBOCI, engineUBOAllocCI);

		m_engineFrameData.push_back(std::move(dat));
	}

	// Create UBO for per pass ... ( ? )



	// Create UBO for per material data ... (e.g combined image samplers)



	// Create UBO for per object data ... (e.g model matrix)
	vk::BufferCreateInfo objectUBOCI({}, sizeof(ObjectData), vk::BufferUsageFlagBits::eUniformBuffer);
	VmaAllocationCreateInfo objectUBOAllocCI{};
	objectUBOAllocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	for (int i = 0; i < VulkanContext::s_maxFramesInFlight; ++i)
	{
		ObjectFrameData dat{};
		dat.modelMatBuffer = std::make_unique<Buffer>(m_gfxCon.getAllocator(), objectUBOCI, objectUBOAllocCI);
		
		m_objectFrameData.push_back(std::move(dat));
	}

}

void SponzaApp::loadTextures()
{
	//m_loadedTextures.push_back(loadVkImage(m_gfxCon, "Resources/Textures/rimuru.jpg"));
	//m_loadedTextures.push_back(loadVkImage(m_gfxCon, "Resources/Textures/rimuru2.jpg"));
	m_mappedTextures.insert({ "rimuru", loadVkImage(m_gfxCon, "Resources/Textures/rimuru.jpg") });
	m_mappedTextures.insert({ "rimuru2", loadVkImage(m_gfxCon, "Resources/Textures/rimuru2.jpg") });
	m_mappedTextures.insert({ "defaultopacity", loadVkImage(m_gfxCon, "Resources/Textures/defaultopacity.jpg") });
}

void SponzaApp::createDescriptorPool()
{
	// Make pool large enough for our needs
	std::vector<vk::DescriptorPoolSize> descriptorPoolSizes{
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 10),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 100)
	};

	vk::DescriptorPoolCreateInfo poolCI({}, 
		150,										
		descriptorPoolSizes
	);
	
	m_descriptorPool = m_gfxCon.getDevice().createDescriptorPoolUnique(poolCI);
}



void SponzaApp::createRenderModels()
{
	auto dev = m_gfxCon.getDevice();

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

	// Create buffer resources
	auto vb = loadVkImmutableBuffer(m_gfxCon, vertices, vk::BufferUsageFlagBits::eVertexBuffer);
	auto ib = loadVkImmutableBuffer(m_gfxCon, indices, vk::BufferUsageFlagBits::eIndexBuffer);

	// Create descriptor set with new material
	vk::DescriptorSetAllocateInfo texAllocInfo(m_descriptorPool.get(), m_materialDescriptorSetLayout.get());
	auto newMatDescSet = m_gfxCon.getDevice().allocateDescriptorSets(texAllocInfo).front();

	// Write to it (bind image and sampler)
	vk::DescriptorImageInfo imageInfo(m_commonSampler.get(), m_mappedTextures["rimuru2"]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet imageSetWrite(newMatDescSet, 0, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo, {}, {});
	vk::DescriptorImageInfo opacityImageInfo(m_commonSampler.get(), m_mappedTextures["defaultopacity"]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet opacityImageSetWrite(newMatDescSet, 1, 0, vk::DescriptorType::eCombinedImageSampler, opacityImageInfo, {}, {});
	dev.updateDescriptorSets({ imageSetWrite, opacityImageSetWrite }, {});


	// ==== Create render unit(s)
	// Create material
	//m_loadedMaterials.push_back(std::make_unique<Material>(m_mainGfxPipeline.get(), m_mainGfxPipelineLayout.get(), newMatDescSet));
	m_mappedMaterials.insert({ "rimuruMaterial", std::make_unique<Material>(m_mainGfxPipeline.get(), m_mainGfxPipelineLayout.get(), newMatDescSet) });

	// Create mesh for each Render Unit(data into VB/IB)
	auto mesh = Mesh(0, static_cast<uint32_t>(indices.size()));

	// Combine to mesh and material into a render unit
	RenderUnit renderUnit(mesh, *m_mappedMaterials["rimuruMaterial"].get());		// Use the newly created material

	// Create render model
	std::vector<RenderUnit> renderUnits{ renderUnit };
	m_loadedModels.push_back(std::make_unique<RenderModel>(std::move(vb), std::move(ib), renderUnits));
}


void SponzaApp::setupDescriptorSetLayouts()
{
	// Engine set layout
	std::vector<vk::DescriptorSetLayoutBinding> engineSetBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex)
	};
	vk::DescriptorSetLayoutCreateInfo engineSetLayoutCI({}, engineSetBindings);
	
	// Per pass layout (we are not using currently)
	std::vector<vk::DescriptorSetLayoutBinding> passBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex)
	};
	vk::DescriptorSetLayoutCreateInfo passSetLayoutCI({}, passBindings);

	// Per material layout
	std::vector<vk::DescriptorSetLayoutBinding> materialBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),	// Albedo 
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment)		// Opacity
	};
	vk::DescriptorSetLayoutCreateInfo materialSetLayoutCI({}, materialBindings);

	// Per object layout
	std::vector<vk::DescriptorSetLayoutBinding> objectBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex)
	};
	vk::DescriptorSetLayoutCreateInfo objectSetLayoutCI({}, objectBindings);

	// Create layouts
	auto dev = m_gfxCon.getDevice();
	m_engineDescriptorSetLayout = dev.createDescriptorSetLayoutUnique(engineSetLayoutCI);
	m_passDescriptorSetLayout = dev.createDescriptorSetLayoutUnique(passSetLayoutCI);
	m_materialDescriptorSetLayout = dev.createDescriptorSetLayoutUnique(materialSetLayoutCI);
	m_objectDescriptorSetLayout = dev.createDescriptorSetLayoutUnique(objectSetLayoutCI);

}

void SponzaApp::configurePushConstantRange()
{
	// This is used to configure the PipelineLayout when creating a Graphics Pipeline
	m_pushConstantRange = vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData));
}

void SponzaApp::allocateDescriptorSets()
{
	auto dev = m_gfxCon.getDevice();

	// Allocate Set 0 and bind resources to it
	vk::DescriptorSetAllocateInfo engineSetAllocInfo(m_descriptorPool.get(), m_engineDescriptorSetLayout.get());
	
	// We need to allocate 'frame in flight' number of sets since they need to be updated while the other set may still be in flight
	for (auto i = 0ul; i < VulkanContext::s_maxFramesInFlight; ++i)
	{
		m_engineFrameData[i].descriptorSet = dev.allocateDescriptorSets(engineSetAllocInfo).front();

		// Write to the set (bind actual resource)
		vk::DescriptorBufferInfo binfo(m_engineFrameData[i].cameraBuffer->getBuffer(), 0, sizeof(GPUCameraData));
		vk::WriteDescriptorSet writeInfo(m_engineFrameData[i].descriptorSet, 0, 0, vk::DescriptorType::eUniformBuffer, {}, binfo);
		dev.updateDescriptorSets(writeInfo, {});
	}

	// NOTE: We wont be using them yet, but soon.
	// Allocate Sets 1 and bind resources to it..



	// Allocate Sets 2 and bind resources to it (this is done on createRenderModels for now)
	


	// Allocate Sets 3
	vk::DescriptorSetAllocateInfo objectSetAllocInfo(m_descriptorPool.get(), m_objectDescriptorSetLayout.get());

	// We need to allocate 'frame in flight' number of sets since they need to be updated while the other set may still be in flight
	for (auto i = 0ul; i < VulkanContext::s_maxFramesInFlight; ++i)
	{
		m_objectFrameData[i].descriptorSet = dev.allocateDescriptorSets(objectSetAllocInfo).front();

		// Write to the set (bind actual resource)
		vk::DescriptorBufferInfo binfo(m_objectFrameData[i].modelMatBuffer->getBuffer(), 0, sizeof(ObjectData));
		vk::WriteDescriptorSet writeInfo(m_objectFrameData[i].descriptorSet, 0, 0, vk::DescriptorType::eUniformBuffer, {}, binfo);
		dev.updateDescriptorSets(writeInfo, {});
	}


}

void SponzaApp::createGraphicsPipeline()
{
	auto dev = m_gfxCon.getDevice();

	// ======== Shader
	auto vertBin = readFile("compiled_shaders/vertSponza.spv");
	auto fragBin = readFile("compiled_shaders/fragSponza.spv");
	auto vertMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, vertBin.size(), reinterpret_cast<uint32_t*>(vertBin.data())));
	auto fragMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, fragBin.size(), reinterpret_cast<uint32_t*>(fragBin.data())));
	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageC = {
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertMod.get(), "main"),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragMod.get(), "main")
	};

	// ======== Vertex Input Binding Description
	std::vector<vk::VertexInputBindingDescription> bindingDescs{ Vertex::getBindingDescription() };
	auto inputAttrDescs{ Vertex::getAttributeDescriptions() };
	vk::PipelineVertexInputStateCreateInfo vertInC({}, bindingDescs, inputAttrDescs);

	// ======== Input Assembler State
	vk::PipelineInputAssemblyStateCreateInfo iaC({}, vk::PrimitiveTopology::eTriangleList);

	// ======== Viewport & Scissor
	// https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
	auto scExtent = m_gfxCon.getSwapchainExtent();
	vk::Viewport vp = vk::Viewport(0.f, 0.f, static_cast<float>(scExtent.width), static_cast<float>(scExtent.height), 0.0, 1.0);
	vk::Rect2D scissor = vk::Rect2D({ 0, 0 }, scExtent);
	vk::PipelineViewportStateCreateInfo vpC({}, 1, &vp, 1, &scissor);	// no stencil

	// ======== Rasterizer State
	vk::PipelineRasterizationStateCreateInfo rsC({},
		false,
		false,
		vk::PolygonMode::eFill,				// eLine topology for wireframe
		vk::CullModeFlagBits::eBack,
		vk::FrontFace::eCounterClockwise,	// default
		{}, {}, {}, {},						// depth bias
		1.0f								// line width
	);

	// ======== Multisampling options
	vk::PipelineMultisampleStateCreateInfo msC({}, vk::SampleCountFlagBits::e1 /*sample shading and sample mask*/);
	msC.setAlphaToCoverageEnable(true);
	msC.setAlphaToOneEnable(false);

	// ======== Depth stencil
	vk::PipelineDepthStencilStateCreateInfo dsC({}, true, true, vk::CompareOp::eLessOrEqual /*(depth bound test args and stencil)*/);

	// ======== Blend state
	vk::PipelineColorBlendAttachmentState colorBlendAttachment(
		false,
		vk::BlendFactor::eSrcAlpha,				// This is what we supply as the Alpha component of the fragment shader output!
		vk::BlendFactor::eOneMinusSrcAlpha,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eZero,
		vk::BlendFactor::eOne,
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

	// ======== Pipeline Layout (Shader Input)
	
	// Order matters here
	std::vector<vk::DescriptorSetLayout> compatibleLayouts{
		m_engineDescriptorSetLayout.get(),		// Set 0
		m_passDescriptorSetLayout.get(),		// Set 1
		m_materialDescriptorSetLayout.get(),	// Set 2
		m_objectDescriptorSetLayout.get()		// Set 3
	};	

	m_mainGfxPipelineLayout = dev.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, compatibleLayouts, m_pushConstantRange));
	//m_mainGfxPipelineLayout = dev.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, compatibleLayouts));

	m_mainGfxPipeline = dev.createGraphicsPipelineUnique({},
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
			m_mainGfxPipelineLayout.get(),
			m_defRenderPass.get(),
			0
		)
	).value;
}

// Assimp
uint32_t s_meshVertexCount = 0;
uint32_t s_meshIndexCount = 0;
struct AssimpMeshSubset
{
	unsigned int m_vertexCount;
	unsigned int m_vertexStart;

	unsigned int m_indexStart;
	unsigned int m_indexCount;

	std::string m_diffuseFilePath;
	std::string m_specularFilePath;
	std::string m_normalFilePath;
	std::string m_opacityFilePath;
};


void processMesh(aiMesh* mesh, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<AssimpMeshSubset>& subsets)
{
	for (int i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex vert{};
		vert.pos.x = mesh->mVertices[i].x;
		vert.pos.y = mesh->mVertices[i].y;
		vert.pos.z = mesh->mVertices[i].z;

		vert.color.x = mesh->mNormals[i].x;
		vert.color.y = mesh->mNormals[i].y;
		vert.color.z = mesh->mNormals[i].z;

		if (mesh->mTextureCoords[0])
		{
			vert.uv.x = mesh->mTextureCoords[0][i].x;
			vert.uv.y = mesh->mTextureCoords[0][i].y;
		}

		vertices.push_back(vert);
	}

	unsigned int indicesThisMesh = 0;
	for (int i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];

		for (int j = 0; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
			++indicesThisMesh;
		}

	}

	// Get material
	auto mtl = scene->mMaterials[mesh->mMaterialIndex];
	aiString diffPath, norPath, opacityPath;
	mtl->GetTexture(aiTextureType_DIFFUSE, 0, &diffPath);
	mtl->GetTexture(aiTextureType_HEIGHT, 0, &norPath);
	mtl->GetTexture(aiTextureType_OPACITY, 0, &opacityPath);

	// Subset data
	AssimpMeshSubset subsetData = { };
	subsetData.m_diffuseFilePath = diffPath.C_Str();
	subsetData.m_normalFilePath = norPath.C_Str();
	subsetData.m_opacityFilePath = opacityPath.C_Str();

	subsetData.m_vertexCount = mesh->mNumVertices;
	subsetData.m_vertexStart = s_meshVertexCount;
	s_meshVertexCount += mesh->mNumVertices;

	subsetData.m_indexCount = indicesThisMesh;
	subsetData.m_indexStart = s_meshIndexCount;
	s_meshIndexCount += indicesThisMesh;

	subsets.push_back(subsetData);
}

void processNode(aiNode* node, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<AssimpMeshSubset>& subsets)
{

	// For each mesh in the node, process it!
	for (int i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		processMesh(mesh, scene, vertices, indices, subsets);
	}

	for (int i = 0; i < node->mNumChildren; ++i)
	{
		processNode(node->mChildren[i], scene, vertices, indices, subsets);
	}
}

void SponzaApp::loadExternalModel(const std::filesystem::path& filePath)
{
	auto dev = m_gfxCon.getDevice();

	std::string directory = filePath.parent_path().string() + "/";

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<AssimpMeshSubset> subsets;

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(
		filePath.relative_path().string().c_str(),
		aiProcess_Triangulate |
		aiProcess_FlipUVs |			// Vulkan screen space is LH but we are using RH 
		aiProcess_GenNormals
	);

	if (scene == nullptr)
	{
		std::cout << "Assimp: File not found! : " << filePath.filename() << "\n";
		assert(false);
	}

	unsigned int totalVertexCount = 0;
	unsigned int totalSubsetCount = 0;
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		totalVertexCount += scene->mMeshes[i]->mNumVertices;
		++totalSubsetCount;
	}
	vertices.reserve(totalVertexCount);
	indices.reserve(totalVertexCount);
	subsets.reserve(totalSubsetCount);

	processNode(scene->mRootNode, scene, vertices, indices, subsets);

	s_meshVertexCount = 0;
	s_meshIndexCount = 0;

	// ============== Assimp loading done
	// Now load data to Vulkan =============

	auto vb = loadVkImmutableBuffer(m_gfxCon, vertices, vk::BufferUsageFlagBits::eVertexBuffer);
	auto ib = loadVkImmutableBuffer(m_gfxCon, indices, vk::BufferUsageFlagBits::eIndexBuffer);

	std::vector<RenderUnit> renderUnits;
	renderUnits.reserve(subsets.size());
	for (const auto& subset : subsets)
	{
		auto mesh = Mesh(subset.m_indexStart, subset.m_indexCount, subset.m_vertexStart);

		// Create material
		std::string albedoPath(directory);
		if (!subset.m_diffuseFilePath.empty())
			albedoPath += subset.m_diffuseFilePath;
		else
			albedoPath = "Resources/Textures/defaulttexture.jpg";

		std::string opacityPath(directory);
		if (!subset.m_opacityFilePath.empty())
			opacityPath += subset.m_opacityFilePath;
		else
			opacityPath = "Resources/Textures/defaultopacity.jpg";

		auto lb = m_mappedTextures.lower_bound(albedoPath);
		if (lb != m_mappedTextures.end() && !(m_mappedTextures.key_comp()(albedoPath, lb->first)))
		{
			// Combine to mesh and material into a render unit
			renderUnits.push_back(RenderUnit(mesh, *m_mappedMaterials[albedoPath].get()));
		}
		else
		{
			// Handle albedo
			{

				m_mappedTextures.insert(lb, { albedoPath, std::move(loadVkImage(m_gfxCon, albedoPath)) });


				// Create descriptor set with new material
				vk::DescriptorSetAllocateInfo texAllocInfo(m_descriptorPool.get(), m_materialDescriptorSetLayout.get());
				auto newMatDescSet = m_gfxCon.getDevice().allocateDescriptorSets(texAllocInfo).front();

				// Write to it (bind image and sampler)
				vk::DescriptorImageInfo imageInfo(m_commonSampler.get(), m_mappedTextures[albedoPath]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
				vk::WriteDescriptorSet imageSetWrite(newMatDescSet, 0, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo, {}, {});
				dev.updateDescriptorSets(imageSetWrite, {});



				// Create material
				//m_loadedMaterials.push_back(std::make_unique<Material>(m_mainGfxPipeline.get(), m_mainGfxPipelineLayout.get(), newMatDescSet));
				m_mappedMaterials.insert({ albedoPath, std::make_unique<Material>(m_mainGfxPipeline.get(), m_mainGfxPipelineLayout.get(), newMatDescSet) });
			}

			// Handle opacity (implicit that if albedo was not found, the opacity must be new)
			{
				// WARNING::::: Std move calls destructor if there is an existing element in the map!!! (why??)
				if (m_mappedTextures.find(opacityPath) == m_mappedTextures.cend())
					m_mappedTextures.insert({ opacityPath, std::move(loadVkImage(m_gfxCon, opacityPath)) });

				// Write to existing descriptor set (existing material that was made from Albedo) (bind image and sampler)
				vk::DescriptorImageInfo imageInfo(m_commonSampler.get(), m_mappedTextures[opacityPath]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
				vk::WriteDescriptorSet imageSetWrite(m_mappedMaterials[albedoPath]->getDescriptorSet(), 1, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo, {}, {});	// Binding 1
				dev.updateDescriptorSets(imageSetWrite, {});

			}



			// Combine to mesh and material into a render unit
			renderUnits.push_back(RenderUnit(mesh, *m_mappedMaterials[albedoPath].get()));
		}
	}

	m_loadedModels.push_back(std::make_unique<RenderModel>(std::move(vb), std::move(ib), renderUnits));
}

}
