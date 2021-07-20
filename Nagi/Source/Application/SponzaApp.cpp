#include "pch.h"
#include "Application/SponzaApp.h"

#include "AssimpLoader.h"
#include "Camera.h"

namespace Nagi
{

SponzaApp::SponzaApp(Window& window, VulkanContext& gfxCon) :
	Application(window, gfxCon)
{
	// Get input handlers
	auto keyHandler = window.getKeyHandler();
	auto mouseHandler = window.getMouseHandler();
	assert(keyHandler != nullptr && mouseHandler != nullptr);

	auto scExtent = m_gfxCon.getSwapchainExtent();
	Camera fpsCam((float)scExtent.width / scExtent.height, 77.f);

	try
	{
		m_defRenderPass = ezTmp::createDefaultRenderPass(m_gfxCon);
		m_defFramebuffers = ezTmp::createDefaultFramebuffers(m_gfxCon, m_defRenderPass.get());

		// Allocate pool for descriptors
		createDescriptorPool();

		// Create UBOs that will be used in this App
		createUBOs();

		// Set up Texture
		loadTextures();

		vk::SamplerCreateInfo sCI({},
			vk::Filter::eLinear, vk::Filter::eLinear,	// min/mag filter
			vk::SamplerMipmapMode::eLinear,				// mipmapMode
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			0.f,							// mipLodBias
			true, m_gfxCon.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy,		// anisotropy enabled / max anisotropy (max clamp value) (we are just maxing out here (16))
			false, vk::CompareOp::eNever,	// compare enabled/op
			0.f, VK_LOD_CLAMP_NONE
		);
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
		// Create custom render model
		createRenderModels();

		// Load with assimp
		loadExternalModel("Resources/Objs/nanosuit/nanosuit.obj");
		loadExternalModel("Resources/Objs/sponza/sponza.obj");
		loadExternalModel("Resources/Objs/survival_backpack/backpack.obj");



		float dt = 0.f;

		// We can hook to cursor function instead to subscribe to the callback which updates more frequently (but fixed timestep?) for smoother mouse!
		// I assume that the "fixed timestep" effect comes from the fact that Window Events do not occur more frequently or less frequently if we have more/less FPS!
		// This is why the below function has the "same sensitivity" regardless of application FPS!
		mouseHandler->hookFunctionToCursor([&fpsCam](float deltaX, float deltaY)
			{
				// 0.07 --> Arbitrary dt
				fpsCam.rotateCamera(deltaX, deltaY, 0.07f);
			});

		float timeElapsed = 0.f;
		while (m_window.isRunning())
		{
			timeElapsed += dt;
			auto timeStart = std::chrono::system_clock::now();			
			m_window.processEvents();

			// Update camera
			//fpsCam.rotateCamera(mouseHandler->getDeltaX(), mouseHandler->getDeltaY(), dt);

			if (keyHandler->isKeyDown(KeyName::A))		fpsCam.move(MoveDirection::Left);
			if (keyHandler->isKeyDown(KeyName::D))		fpsCam.move(MoveDirection::Right);
			if (keyHandler->isKeyDown(KeyName::W))		fpsCam.move(MoveDirection::Forward);
			if (keyHandler->isKeyDown(KeyName::S))		fpsCam.move(MoveDirection::Backward);
			if (keyHandler->isKeyDown(KeyName::Space))	fpsCam.move(MoveDirection::Up);
			if (keyHandler->isKeyDown(KeyName::LShift))	fpsCam.move(MoveDirection::Down);

			fpsCam.update(dt);

			if (keyHandler->isKeyPressed(KeyName::E))
			{
				std::cout << "Rotate count: " << fpsCam.m_rotateCount << "\n";
				std::cout << "Update count: " << fpsCam.m_updateCount<< "\n\n";
			}


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

			SceneData sceneData{};
			sceneData.lightColor = glm::vec4(1.f);
			//sceneData.lightDirection = glm::vec4(cosf(timeElapsed) * 0.5f - 0.5f, -1.f, -1.f, 0.f);
			//sceneData.lightDirection = glm::normalize(glm::vec4(cosf(timeElapsed), -1.f, -1.f, 0.f));
			//sceneData.lightDirection = glm::normalize(glm::vec4(0.f, 0.f, -1.f, 0.f));
			sceneData.lightDirection = glm::normalize(glm::vec4(-0.35f, -1.f, -1.f, 0.f));

			// GPU BELOW
			auto frameRes = gfxCon.beginFrame();
			auto& cmd = frameRes.gfxCmdBuffer;


			// ================================================ Update UBO
			m_engineFrameData[frameRes.frameIdx].cameraBuffer->putData(&cameraData, sizeof(GPUCameraData));
			m_engineFrameData[frameRes.frameIdx].sceneBuffer->putData(&sceneData, sizeof(SceneData));


			// Setup render pass info
			std::array<vk::ClearValue, 2> clearValues = {
				vk::ClearColorValue(std::array<float, 4>({0.529f, 0.808f, 0.922f, 1.f})),
				vk::ClearDepthStencilValue( /*depth*/ 1.f, /*stencil*/ 0)
			};

			vk::RenderPassBeginInfo rpInfo(m_defRenderPass.get(), m_defFramebuffers[frameRes.imageIdx].get(), vk::Rect2D({ 0, 0 }, scExtent), clearValues);



			// Record command buffer
			cmd.begin(vk::CommandBufferBeginInfo());


			// Bind engine wide resources (Camera, Scene, Set 0)
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_mainGfxPipelineLayout.get(), 0, m_engineFrameData[frameRes.frameIdx].descriptorSet, {});

			cmd.beginRenderPass(rpInfo, {});

			// Render Objects
			int tmpId = 0;
			Material lastMaterial;
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
				if (tmpId == 2)
				{
					auto newMatModel = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(0.07f));
					perObjectData.modelMat = newMatModel;
				}
				// Nanosuit
				else if (tmpId == 1)
				{
					auto newMatModel = glm::translate(glm::mat4(1.f), glm::vec3(9.f, 0.f, -9.f)) * glm::scale(glm::mat4(1.f), glm::vec3(1.f));
					perObjectData.modelMat = newMatModel;
				}
				else if (tmpId == 3)
				{
					auto newMatModel = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 7.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(1.f));
					perObjectData.modelMat = newMatModel;
				}

				for (const auto& renderUnit : renderUnits)
				{
					const auto& mesh = renderUnit.getMesh();
					const auto& mat = renderUnit.getMaterial();

					if (mat != lastMaterial)
					{
						cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mat.getPipeline());
						lastMaterial = mat;
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

			auto timeEnd = std::chrono::system_clock::now();
			std::chrono::duration<float> diff = timeEnd - timeStart;
			dt = diff.count();
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
		dat.sceneBuffer = std::make_unique<Buffer>(m_gfxCon.getAllocator(), vk::BufferCreateInfo({}, sizeof(SceneData), vk::BufferUsageFlagBits::eUniformBuffer), engineUBOAllocCI);

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
	m_mappedTextures.insert({ "rimuru", Texture::fromFile(m_gfxCon, "Resources/Textures/rimuru.jpg", true) });
	m_mappedTextures.insert({ "rimuru2", Texture::fromFile(m_gfxCon, "Resources/Textures/rimuru2.jpg", true) });
	m_mappedTextures.insert({ "defaultopacity", Texture::fromFile(m_gfxCon, "Resources/Textures/defaultopacity.jpg") });
	m_mappedTextures.insert({ "defaultspecular", Texture::fromFile(m_gfxCon, "Resources/Textures/defaultspecular.jpg") });
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
		{ { -0.5f, 0.5f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f, 1.f } },
		{ { 0.5f, 0.5f, 0.f }, { 1.f, 0.f }, { 0.f, 0.f, 1.f } },
		{ { 0.5f, -0.5f, 0.f }, { 1.f, 1.f }, { 0.f, 0.f, 1.f } },
		{ { -0.5f, -0.5f, 0.f }, { 0.f, 1.f }, { 0.f, 0.f, 1.f } }
	};

	// CCW
	std::vector<uint32_t> indices{
		0, 2, 1,
		0, 3, 2
	};

	// Create buffer resources
	auto vb = Buffer::loadImmutable(m_gfxCon, vertices, vk::BufferUsageFlagBits::eVertexBuffer);
	auto ib = Buffer::loadImmutable(m_gfxCon, indices, vk::BufferUsageFlagBits::eIndexBuffer);

	// Create descriptor set with new material
	vk::DescriptorSetAllocateInfo texAllocInfo(m_descriptorPool.get(), m_materialDescriptorSetLayout.get());
	auto newMatDescSet = m_gfxCon.getDevice().allocateDescriptorSets(texAllocInfo).front();

	// Write to it (bind image and sampler)
	vk::DescriptorImageInfo imageInfo(m_commonSampler.get(), m_mappedTextures["rimuru2"]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet imageSetWrite(newMatDescSet, 0, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo, {}, {});
	vk::DescriptorImageInfo opacityImageInfo(m_commonSampler.get(), m_mappedTextures["defaultopacity"]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet opacityImageSetWrite(newMatDescSet, 1, 0, vk::DescriptorType::eCombinedImageSampler, opacityImageInfo, {}, {});
	vk::DescriptorImageInfo specularImageInfo(m_commonSampler.get(), m_mappedTextures["defaultspecular"]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet specularImageSetWrite(newMatDescSet, 2, 0, vk::DescriptorType::eCombinedImageSampler, specularImageInfo, {}, {});
	dev.updateDescriptorSets({ imageSetWrite, opacityImageSetWrite, specularImageSetWrite}, {});


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
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment),			// Camera
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)		// Scene Lighting
	};
	vk::DescriptorSetLayoutCreateInfo engineSetLayoutCI({}, engineSetBindings);
	
	// Per pass layout (we are not using currently)
	std::vector<vk::DescriptorSetLayoutBinding> passBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex)
	};
	vk::DescriptorSetLayoutCreateInfo passSetLayoutCI({}, passBindings);

	// Per material layout
	std::vector<vk::DescriptorSetLayoutBinding> materialBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),	// diffuse 
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),	// Opacity
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment)		// Specular
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

		vk::DescriptorBufferInfo binfoScene(m_engineFrameData[i].sceneBuffer->getBuffer(), 0, sizeof(SceneData));
		vk::WriteDescriptorSet writeInfoScene(m_engineFrameData[i].descriptorSet, 0, 1, vk::DescriptorType::eUniformBuffer, {}, binfoScene);

		dev.updateDescriptorSets({ writeInfo }, {});
		dev.updateDescriptorSets({ writeInfoScene }, {});
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


void SponzaApp::loadExternalModel(const std::filesystem::path& filePath)
{
	auto dev = m_gfxCon.getDevice();
	std::string directory = filePath.parent_path().string() + "/";

	auto loader = AssimpLoader(filePath);
	auto& vertices = loader.getVertices();
	auto& indices = loader.getIndices();
	auto& subsets = loader.getSubsets();


	// ============== Assimp loading done
	// Now load data to Vulkan =============

	// ======== Handle VB/IB
	// Pack data
	std::vector<Vertex> finalVerts;
	finalVerts.reserve(vertices.size());
	for (const auto& vert : vertices)
	{
		Vertex vertex;
		vertex.pos.x = vert.position.x;
		vertex.pos.y = vert.position.y;
		vertex.pos.z = vert.position.z;
		vertex.uv.x = vert.uv.x;
		vertex.uv.y = vert.uv.y;
		vertex.normal.x = vert.normal.x;
		vertex.normal.y = vert.normal.y;
		vertex.normal.z = vert.normal.z;
		finalVerts.push_back(vertex);
	}

	// Push into VB/IB pair
	auto vb = Buffer::loadImmutable(m_gfxCon, finalVerts, vk::BufferUsageFlagBits::eVertexBuffer);
	auto ib = Buffer::loadImmutable(m_gfxCon, indices, vk::BufferUsageFlagBits::eIndexBuffer);


	// ======== Handle Subsets
	std::vector<RenderUnit> renderUnits;
	renderUnits.reserve(subsets.size());
	for (const auto& subset : subsets)
	{
		auto mesh = Mesh(subset.indexStart, subset.indexCount, subset.vertexStart);

		// Get final diffuse path
		std::string diffusePath(directory);
		if (subset.diffuseFilePath.has_value())
			diffusePath += subset.diffuseFilePath.value();
		else
			diffusePath = "Resources/Textures/defaulttexture.jpg";

		// Get final opacity path
		std::string opacityPath(directory);
		if (subset.opacityFilePath.has_value())
			opacityPath += subset.opacityFilePath.value();
		else
			opacityPath = "Resources/Textures/defaultopacity.jpg";

		// Get final specular path
		std::string specularPath(directory);
		if (subset.specularFilePath.has_value())
			specularPath += subset.specularFilePath.value();
		else
			specularPath = "Resources/Textures/defaultspecular.jpg";


		// Insert texture data and create material
		auto lb = m_mappedTextures.lower_bound(diffusePath);
		if (lb != m_mappedTextures.end() && !(m_mappedTextures.key_comp()(diffusePath, lb->first)))
		{
			// Combine to mesh and material into a render unit
			renderUnits.push_back(RenderUnit(mesh, *m_mappedMaterials[diffusePath].get()));
		}
		else
		{
			// Handle diffuse
			{

				m_mappedTextures.insert(lb, { diffusePath, std::move(Texture::fromFile(m_gfxCon, diffusePath, true)) });


				// Create descriptor set with new material
				vk::DescriptorSetAllocateInfo texAllocInfo(m_descriptorPool.get(), m_materialDescriptorSetLayout.get());
				auto newMatDescSet = m_gfxCon.getDevice().allocateDescriptorSets(texAllocInfo).front();

				// Write to it (bind image and sampler)
				vk::DescriptorImageInfo imageInfo(m_commonSampler.get(), m_mappedTextures[diffusePath]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
				vk::WriteDescriptorSet imageSetWrite(newMatDescSet, 0, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo, {}, {});
				dev.updateDescriptorSets(imageSetWrite, {});


				// Create material
				//m_loadedMaterials.push_back(std::make_unique<Material>(m_mainGfxPipeline.get(), m_mainGfxPipelineLayout.get(), newMatDescSet));
				m_mappedMaterials.insert({ diffusePath, std::make_unique<Material>(m_mainGfxPipeline.get(), m_mainGfxPipelineLayout.get(), newMatDescSet) });
			}

			// Handle opacity (implicit that if diffuse was not found, the opacity must be new)
			{
				// WARNING::::: Std move calls destructor if there is an existing element in the map!!! (why??)
				if (m_mappedTextures.find(opacityPath) == m_mappedTextures.cend())
					m_mappedTextures.insert({ opacityPath, std::move(Texture::fromFile(m_gfxCon, opacityPath)) });

				// Write to existing descriptor set (existing material that was made from diffuse) (bind image and sampler)
				vk::DescriptorImageInfo imageInfo(m_commonSampler.get(), m_mappedTextures[opacityPath]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
				vk::WriteDescriptorSet imageSetWrite(m_mappedMaterials[diffusePath]->getDescriptorSet(), 1, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo, {}, {});	// Binding 1
				dev.updateDescriptorSets(imageSetWrite, {});
			}

			// Handle specular
			{
				if (m_mappedTextures.find(specularPath) == m_mappedTextures.cend())
					m_mappedTextures.insert({ specularPath, std::move(Texture::fromFile(m_gfxCon, specularPath)) });

				// Write to existing descriptor set (existing material that was made from diffuse) (bind image and sampler)
				vk::DescriptorImageInfo imageInfo(m_commonSampler.get(), m_mappedTextures[specularPath]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
				vk::WriteDescriptorSet imageSetWrite(m_mappedMaterials[diffusePath]->getDescriptorSet(), 2, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo, {}, {});	// Binding 2
				dev.updateDescriptorSets(imageSetWrite, {});

			}

			// Combine to mesh and material into a render unit
			renderUnits.push_back(RenderUnit(mesh, *m_mappedMaterials[diffusePath].get()));
		}
	}


	m_loadedModels.push_back(std::make_unique<RenderModel>(std::move(vb), std::move(ib), renderUnits));
}

}
