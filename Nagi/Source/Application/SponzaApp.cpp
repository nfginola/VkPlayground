#include "pch.h"
#include "Application/SponzaApp.h"

#include "AssimpLoader.h"
#include "Camera.h"
#include "VulkanImGuiContext.h"
#include "Timer.h"

#include "ReflectionTest.h"

#include "ShaderGroup.h"

namespace Nagi
{

SponzaApp::SponzaApp(Window& window, VulkanContext& vkCon) :
	Application(window, vkCon)
{
	// Get input handlers
	auto keyboard = window.getKeyboard();
	auto mouse = window.getMouse();
	assert(keyboard != nullptr && mouse != nullptr);

	auto scExtent = m_vkCon.getSwapchainExtent();
	Camera fpsCam((float)scExtent.width / scExtent.height, 90.f);

	mouse->hookFunctionToCursor([&fpsCam](float deltaX, float deltaY) { fpsCam.rotateCamera(deltaX, deltaY, 0.07f); });

	try
	{
		setupResources();

		// Initialize ImGui (After application resources --> Defined render pass to "hook onto")
		// Give a suitable render pass to draw with
		auto imGuiContext = std::make_unique<VulkanImGuiContext>(m_vkCon, m_window, m_defRenderPass.get());

		// Setup entities
		Scene s1;
		auto e1 = s1.createEntity();
		auto e2 = s1.createEntity();
		auto e3 = s1.createEntity();
		auto e4 = s1.createEntity();
		auto e5 = s1.createEntity();

		e1.addComponent<ModelRefComponent>(m_loadedModels["rimuru"].get());
		e2.addComponent<ModelRefComponent>(m_loadedModels["nanosuit"].get());
		e3.addComponent<ModelRefComponent>(m_loadedModels["sponza"].get());
		e4.addComponent<ModelRefComponent>(m_loadedModels["backpack"].get());

		for (int i = -40; i < 40; i += 8)
		{
			auto eT = s1.createEntity();

			eT.addComponent<ModelRefComponent>(m_loadedModels["nanosuit"].get());

			eT.getComponent<TransformComponent>().mat =
				glm::translate(glm::mat4(1.f), glm::vec3(i, 0.f, 6.f)) *
				glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f)) *
				glm::scale(glm::mat4(1.f), glm::vec3(1.f, 1.f, 1.f));
		}

		e1.getComponent<TransformComponent>().mat = 
				glm::translate(glm::mat4(1.f), glm::vec3(0.f, 2.f, 0.f)) *
				glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f)) *
				glm::scale(glm::mat4(1.f), glm::vec3(8.f, 5.f, 8.f));

		e3.getComponent<TransformComponent>().mat = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(0.07f));


		// Lights
		auto p1 = s1.createEntity();
		auto p2 = s1.createEntity();
		auto d1 = s1.createEntity();
		auto f1 = s1.createEntity();

		p1.getComponent<TransformComponent>().mat = glm::translate(glm::mat4(1.f), glm::vec3(-15.f, 2.f, -2.f));
		p1.addComponent<PointLightComponent>(glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec4(1.f, 0.09f, 0.016f, 0.f));

		p2.getComponent<TransformComponent>().mat = glm::translate(glm::mat4(1.f), glm::vec3(15.f, 2.f, -2.f));
		p2.addComponent<PointLightComponent>(glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec4(1.f, 0.09f, 0.016f, 0.f));

		d1.addComponent<DirectionalLightComponent>(glm::vec4(1.f), glm::vec4(-0.35f, -1.f, -1.f, 0.f));
	
		float dt = 0.f;
		float timeElapsed = 0.f;
		float spotlightStrength = 0.2f;
		bool showImGuiDemo = true;

		while (m_window.isRunning())
		{
			// ============================================= FRAME START
			Timer timer;
			m_window.processEvents();

			// ============================================= IMGUI FRAME START
			imGuiContext->beginFrame();

			// ============================================= IMGUI WINDOWS
			ImGui::ShowDemoWindow(&showImGuiDemo);

			// ============================================= HANDLE INPUT RESPONSE
			if (keyboard->isKeyDown(KeyName::A))		fpsCam.move(MoveDirection::Left);
			if (keyboard->isKeyDown(KeyName::D))		fpsCam.move(MoveDirection::Right);
			if (keyboard->isKeyDown(KeyName::W))		fpsCam.move(MoveDirection::Forward);
			if (keyboard->isKeyDown(KeyName::S))		fpsCam.move(MoveDirection::Backward);
			if (keyboard->isKeyDown(KeyName::Space))	fpsCam.move(MoveDirection::Up);
			if (keyboard->isKeyDown(KeyName::LShift))	fpsCam.move(MoveDirection::Down);

			fpsCam.update(dt);

			if (keyboard->isKeyPressed(KeyName::G))
				spotlightStrength = 0.f;
			else if (keyboard->isKeyPressed(KeyName::F))
				spotlightStrength = 0.2f;

			// =============================================== UPDATE OBJECTS
			e4.getComponent<TransformComponent>().mat =
				glm::translate(glm::mat4(1.f), glm::vec3(0.f, 10.f + 2.f * cosf(timeElapsed), 0.f)) *
				glm::rotate(glm::mat4(1.f), glm::radians(timeElapsed * 21.f), glm::vec3(0.f, 1.f, 0.f)) *
				glm::scale(glm::mat4(1.f), glm::vec3(1.f));

			e2.getComponent<TransformComponent>().mat =
				glm::translate(glm::mat4(1.f), glm::vec3(9.f, 0.f, -9.f)) *
				glm::rotate(glm::mat4(1.f), glm::radians(timeElapsed * 45.f), glm::vec3(0.f, 1.f, 0.f)) *
				glm::scale(glm::mat4(1.f), glm::vec3(1.f));

			// =============================================== UPDATE ENGINE WIDE DATA
			GPUCameraData cameraData{};
			cameraData.viewMat = fpsCam.getViewMatrix();
			cameraData.projectionMat = fpsCam.getProjectionMatrix();
			cameraData.viewProjectionMat = fpsCam.getProjectionMatrix() * fpsCam.getViewMatrix();

			// =============================================== UPDATE SCENE DATA
			SceneData sceneData{};
			sceneData.directionalLightColor = d1.getComponent<DirectionalLightComponent>().color;
			sceneData.directionalLightDirection = glm::normalize(d1.getComponent<DirectionalLightComponent>().direction);

			sceneData.spotlightPositionAndStrength = glm::vec4(fpsCam.getPosition(), spotlightStrength);
			sceneData.spotlightDirectionAndCutoff = glm::vec4(fpsCam.getLookDirection(), glm::cos(glm::radians(21.f)));

			sceneData.pointLightPosition[0] = p1.getComponent<TransformComponent>().translation();
			sceneData.pointLightColor[0] = p1.getComponent<PointLightComponent>().color;
			sceneData.pointLightAttenuation[0] = p1.getComponent<PointLightComponent>().attenuation;

			sceneData.pointLightPosition[1] = p2.getComponent<TransformComponent>().translation();
			sceneData.pointLightColor[1] = p2.getComponent<PointLightComponent>().color;
			sceneData.pointLightAttenuation[1] = p2.getComponent<PointLightComponent>().attenuation;


			// ================================================ BEGIN GPU FRAME
			auto frameRes = vkCon.beginFrame();
			auto& cmd = frameRes.gfxCmdBuffer;


			// ================================================ UPDATE FRAME UBOS

			// here we write to the offset! 
			// Note that this is safe to do because we have safeguarded this frames resources (fence is signaled only when all commands submitted that frame has finished)
			// Meaning that modifying this "frameIdx" part of the buffer is safe because the previous occurence of frameIdx has already finished reading from it

			// New engine frame data in one buffer with dynamic offsets
			uint32_t perFrameDataSize = sizeof(GPUCameraData) + sizeof(SceneData);
			uint32_t thisFrameOffset = perFrameDataSize * frameRes.frameIdx;

			uint32_t cameraDataOffset = thisFrameOffset;
			uint32_t sceneDataOffset = thisFrameOffset + sizeof(GPUCameraData);
			// packing : [camera, scene, camera, scene, camera, scene] (max 3 frames in flight)
			m_engineFrameBuffer->putData(&cameraData, sizeof(GPUCameraData), cameraDataOffset);						// Upload camera data
			m_engineFrameBuffer->putData(&sceneData, sizeof(SceneData), sceneDataOffset);							// Upload scene data

			// Offsets into the 1st dynamic UB and 2nd dynamic UB
			std::array<uint32_t, 2> engineBufferOffsets = { cameraDataOffset, sceneDataOffset };



			// ================================================ RECORD COMMANDS
			cmd.begin(vk::CommandBufferBeginInfo());

			// Bind engine wide resources with offsets into Dynamic UB (per frame resources) (Camera, Scene, Set 0)
			//cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_mainGfxPipelineLayout.get(), 0, m_engineDescriptorSet, engineBufferOffsets);
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_mainGfxPipelineLayout.get(), 0, m_engineDescriptorSet, engineBufferOffsets);

			// ================================================ SETUP AND RECORD RENDER PASS (***)
			{
				std::array<vk::ClearValue, 2> clearValues = 
				{
					vk::ClearColorValue(std::array<float, 4>({0.529f, 0.808f, 0.922f, 1.f})),
					vk::ClearDepthStencilValue( /*depth*/ 1.f, /*stencil*/ 0)
				};
				vk::RenderPassBeginInfo rpInfo(m_defRenderPass.get(), m_defFramebuffers[frameRes.imageIdx].get(), vk::Rect2D({ 0, 0 }, scExtent), clearValues);

				cmd.beginRenderPass(rpInfo, {});

				// ================================================ DRAW SKYBOX
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_skyboxGfxPipeline.get());	// SkyboxGfxPipeline is built with mainGfxPipelineLayout which makes it compatible with the desc set
				cmd.draw(36, 1, 0, 0);

				// ================================================ RECORD OBJECTS DRAW CMDS
				drawObjects(&s1, cmd);		// Submitting entities from scene which have render model refs, no instancing.
				// ================================================ RECORD IMGUI DRAW CMDS
				imGuiContext->render(cmd);

				cmd.endRenderPass();
			}

			cmd.end();

			// ================================================ END GPU FRAME
			// Setup submit info
			std::array<vk::PipelineStageFlags, 1> waitStages{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
			// Queue waits at just before this stage executes for the sem signal with a full mem barrier

			vk::SubmitInfo submitInfo(
				frameRes.sync.imageAvailableSemaphore,
				waitStages,
				cmd,
				frameRes.sync.renderFinishedSemaphore
			);

			vkCon.submitQueue(submitInfo);
			vkCon.endFrame();

			dt = timer.time();
			timeElapsed += dt;
		}

		// Idle to wait for GPU resources to stop being used before resource destruction
		vkCon.getDevice().waitIdle();

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
	// Create UBO for engine set (Camera and scene)
	// check for minimum buffer alignment for dynamic buffer implementation (one buffer, multiple offset binds)
	auto& prop = m_vkCon.getPhysicalDeviceProperties();
	auto minBufferAlignment = prop.limits.minUniformBufferOffsetAlignment;
	std::cout << "min buffer alignment: " << minBufferAlignment << std::endl;

	VmaAllocationCreateInfo engineBufAllocCI{};
	engineBufAllocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	// Create engine data buffer
	uint32_t sizeAligned = getAlignedSize(VulkanContext::getMaxFramesInFlight() * (sizeof(SceneData) + sizeof(GPUCameraData)), minBufferAlignment);
	auto engineBufCI = vk::BufferCreateInfo({}, sizeAligned, vk::BufferUsageFlagBits::eUniformBuffer);
	m_engineFrameBuffer = std::make_unique<Buffer>(m_vkCon.getAllocator(), engineBufCI, engineBufAllocCI);



	// Create UBO for per pass ... ( ? )



	// Create UBO for per material data ... (e.g combined image samplers)



	// Create UBO for per object data (e.g SSBO)
	vk::BufferCreateInfo objectUBOCI({}, sizeof(ObjectData), vk::BufferUsageFlagBits::eStorageBuffer);
	VmaAllocationCreateInfo objectUBOAllocCI{};
	objectUBOAllocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	for (auto i = 0ul; i < VulkanContext::getMaxFramesInFlight(); ++i)
	{
		ObjectFrameData dat{};
		dat.modelMatBuffer = std::make_unique<Buffer>(m_vkCon.getAllocator(), objectUBOCI, objectUBOAllocCI);
		
		m_objectFrameData.push_back(std::move(dat));
	}

}

void SponzaApp::loadTextures()
{
	m_mappedTextures.insert({ "rimuru", Texture::fromFile(m_vkCon, "Resources/Textures/rimuru.jpg", true) });
	m_mappedTextures.insert({ "rimuru2", Texture::fromFile(m_vkCon, "Resources/Textures/rimuru2.jpg", true) });
	m_mappedTextures.insert({ "defaultopacity", Texture::fromFile(m_vkCon, "Resources/Textures/defaultopacity.jpg") });
	m_mappedTextures.insert({ "defaultspecular", Texture::fromFile(m_vkCon, "Resources/Textures/defaultspecular.jpg") });
	m_mappedTextures.insert({ "defaultnormal", Texture::fromFile(m_vkCon, "Resources/Textures/defaultnormal.jpg") });
	m_mappedTextures.insert({ "yokohamaSB", Texture::cubeFromFile(m_vkCon, "Resources/Textures/Skybox/") });
}

void SponzaApp::drawObjects(Scene* scene, vk::CommandBuffer& cmd)
{
	auto view = scene->getRegistry().view<TransformComponent, ModelRefComponent>();

	// We can batch the transforms by ModelRefs, put transforms in SSBO and use draw instanced using InstanceID as lookup for world matrix in SSBO

	int materialChangeThisFrame = 0;
	Material lastMaterial;
	for (auto e : view)
	{
		auto model = scene->getRegistry().get<ModelRefComponent>(e).model;
		auto& mat = scene->getRegistry().get<TransformComponent>(e).mat;
		PushConstantData perObjectData{ mat };

		const auto& renderUnits = model->getRenderUnits();
		const auto& vb = model->getVertexBuffer();
		const auto& ib = model->getIndexBuffer();

		std::array<vk::Buffer, 1> vbs{ vb };
		std::array<vk::DeviceSize, 1> offsets{ 0 };
		cmd.bindVertexBuffers(0, vbs, offsets);
		cmd.bindIndexBuffer(ib, 0, vk::IndexType::eUint32);

		for (const auto& renderUnit : renderUnits)
		{
			const auto& mesh = renderUnit.getMesh();
			const auto& mat = renderUnit.getMaterial();

			if (mat != lastMaterial)
			{
				// we technically dont have to check this every material change because we may still be using the same pipeline but simply different set of resources
				// (textures). We could check the pipeline independently to avoid this state change.
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mat.getPipeline());

				// Bind per material resources (Textures, Set 2)
				cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mat.getPipelineLayout(), 2, mat.getDescriptorSet(), {});
				lastMaterial = mat;
				materialChangeThisFrame++;
			}

			// Upload model matrix through push constant
			// This is easiest as each Draw call in our current case is one instance of some model
			cmd.pushConstants<PushConstantData>(mat.getPipelineLayout(), vk::ShaderStageFlagBits::eVertex, 0, { perObjectData });

			cmd.drawIndexed(mesh.getNumIndices(), 1, mesh.getFirstIndex(), mesh.getVertexBufferOffset(), 0);
		}

	}

	// use this to check the std::sort on RenderUnits to see that the material change count is lower!
	// because we are sorting the renderunits by material, we can have less state change
	//std::cout << "material change this frame: " << materialChangeThisFrame << '\n';
}

void SponzaApp::createDescriptorPool()
{
	// Make pool large enough for our needs (arbitrary)
	std::vector<vk::DescriptorPoolSize> descriptorPoolSizes{
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 10),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 500),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 10),		// Testing Dynamic Uniform Buffer (we can bind offset in BindDescriptor!)
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 10)				// Testing Dynamic Uniform Buffer (we can bind offset in BindDescriptor!)
	};

	vk::DescriptorPoolCreateInfo poolCI({}, 
		150,										
		descriptorPoolSizes
	);
	
	m_descriptorPool = m_vkCon.getDevice().createDescriptorPoolUnique(poolCI);
}

void SponzaApp::createRenderModels()
{
	auto dev = m_vkCon.getDevice();

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
	auto vb = Buffer::loadImmutable(m_vkCon, vertices, vk::BufferUsageFlagBits::eVertexBuffer);
	auto ib = Buffer::loadImmutable(m_vkCon, indices, vk::BufferUsageFlagBits::eIndexBuffer);

	// Create descriptor set with new material
	vk::DescriptorSetAllocateInfo texAllocInfo(m_descriptorPool.get(), m_materialDescriptorSetLayout.get());
	auto newMatDescSet = m_vkCon.getDevice().allocateDescriptorSets(texAllocInfo).front();

	// Write to it (bind image and sampler)
	vk::DescriptorImageInfo imageInfo(m_commonSampler.get(), m_mappedTextures["rimuru2"]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet imageSetWrite(newMatDescSet, 0, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo, {}, {});
	vk::DescriptorImageInfo opacityImageInfo(m_commonSampler.get(), m_mappedTextures["defaultopacity"]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet opacityImageSetWrite(newMatDescSet, 1, 0, vk::DescriptorType::eCombinedImageSampler, opacityImageInfo, {}, {});
	vk::DescriptorImageInfo specularImageInfo(m_commonSampler.get(), m_mappedTextures["defaultspecular"]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet specularImageSetWrite(newMatDescSet, 2, 0, vk::DescriptorType::eCombinedImageSampler, specularImageInfo, {}, {});
	vk::DescriptorImageInfo normalImageInfo(m_commonSampler.get(), m_mappedTextures["defaultnormal"]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet normalImageSetWrite(newMatDescSet, 3, 0, vk::DescriptorType::eCombinedImageSampler, normalImageInfo, {}, {});
	dev.updateDescriptorSets({ imageSetWrite, opacityImageSetWrite, specularImageSetWrite, normalImageSetWrite}, {});


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
	m_loadedModels.insert({ "rimuru", std::make_unique<RenderModel>(std::move(vb), std::move(ib), renderUnits) });
}

void SponzaApp::setupDescriptorSetLayouts()
{
	// Engine set layout
	std::vector<vk::DescriptorSetLayoutBinding> engineSetBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment)			// Skybox	
	};
	vk::DescriptorSetLayoutCreateInfo engineSetLayoutCI({}, engineSetBindings);


	
	// Per pass layout (we are not using currently)
	std::vector<vk::DescriptorSetLayoutBinding> passBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex)
	};

	vk::DescriptorSetLayoutCreateInfo passSetLayoutCI({}, {});
	//vk::DescriptorSetLayoutCreateInfo passSetLayoutCI({}, passBindings);

	// Per material layout
	std::vector<vk::DescriptorSetLayoutBinding> materialBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),	// Diffuse 
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),	// Opacity
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),	// Specular
		vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment)		// Normal
	};
	vk::DescriptorSetLayoutCreateInfo materialSetLayoutCI({}, materialBindings);

	// Per object layout (not used currently)
	std::vector<vk::DescriptorSetLayoutBinding> objectBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex)	// SSBO for model matrices
	};

	//vk::DescriptorSetLayoutCreateInfo objectSetLayoutCI({}, {});
	vk::DescriptorSetLayoutCreateInfo objectSetLayoutCI({}, objectBindings);

	// Create layouts
	auto dev = m_vkCon.getDevice();
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
	auto dev = m_vkCon.getDevice();

	// ======================================= Allocate Set 0 and bind resources to it
	vk::DescriptorSetAllocateInfo engineSetAllocInfo(m_descriptorPool.get(), m_engineDescriptorSetLayout.get());
	
	// Engine descriptor set (single)
	vk::DescriptorSetAllocateInfo engineSetAllocInfo2(m_descriptorPool.get(), m_engineDescriptorSetLayout.get());
	m_engineDescriptorSet = std::move(dev.allocateDescriptorSets(engineSetAllocInfo2).front());

	// offsets into the buffer are bound on set bind time
	vk::DescriptorBufferInfo binfo(m_engineFrameBuffer->getBuffer(), 0, sizeof(GPUCameraData));
	vk::DescriptorBufferInfo binfo2(m_engineFrameBuffer->getBuffer(), 0, sizeof(SceneData));

	vk::WriteDescriptorSet writeInfo(m_engineDescriptorSet, 0, 0, vk::DescriptorType::eUniformBufferDynamic, {}, binfo);
	vk::WriteDescriptorSet writeInfo2(m_engineDescriptorSet, 1, 0, vk::DescriptorType::eUniformBufferDynamic, {}, binfo2);
	dev.updateDescriptorSets({ writeInfo, writeInfo2 }, {});



	// ======================================= Allocate Sets 1 and bind resources to it (per pass)



	// ======================================= Allocate Sets 2 and bind resources to it (per material)
	


	// ======================================= Allocate Sets 3 (for per-object)
	vk::DescriptorSetAllocateInfo objectSetAllocInfo(m_descriptorPool.get(), m_objectDescriptorSetLayout.get());

	// We need to allocate 'frame in flight' number of sets since they need to be updated while the other set may still be in flight
	for (auto i = 0ul; i < VulkanContext::getMaxFramesInFlight(); ++i)
	{
		m_objectFrameData[i].descriptorSet = dev.allocateDescriptorSets(objectSetAllocInfo).front();

		// Write to the set (bind actual resource)
		// We use one buffer per frame because SSBOs can be read AND written to! Its not exposed the same way as normal UBs (look at shader)
		// Its not like UBs where the shader is only exposed to the offset and range of a buffer memory.
		vk::DescriptorBufferInfo binfo(m_objectFrameData[i].modelMatBuffer->getBuffer(), 0, sizeof(ObjectData));
		vk::WriteDescriptorSet writeInfo(m_objectFrameData[i].descriptorSet, 0, 0, vk::DescriptorType::eStorageBuffer, {}, binfo);
		dev.updateDescriptorSets(writeInfo, {});
	}
}



void SponzaApp::createGraphicsPipeline()
{
	auto dev = m_vkCon.getDevice();

	// ======== Shader
	auto vertBin = readFile("compiled_shaders/vertSponza.spv");
	auto fragBin = readFile("compiled_shaders/fragSponza.spv");
	auto vertMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, vertBin.size(), reinterpret_cast<uint32_t*>(vertBin.data())));
	auto fragMod = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, fragBin.size(), reinterpret_cast<uint32_t*>(fragBin.data())));
	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageC = {
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertMod.get(), "main"),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragMod.get(), "main")
	};

	// We probably dont want the includes as we do now (per frame)
	// Perhaps we dont need to have all bindings everywhere for per frame?
	ShaderGroup shdGrp;
	shdGrp
		.addStage(vk::ShaderStageFlagBits::eVertex, "compiled_shaders/vertSponza.spv")
		.addStage(vk::ShaderStageFlagBits::eFragment, "compiled_shaders/fragSponza.spv")
		.build(m_vkCon.getDevice());

	ShaderGroup shdGrp2;
	shdGrp2
		.addStage(vk::ShaderStageFlagBits::eVertex, "compiled_shaders/vertSkybox.spv")
		.addStage(vk::ShaderStageFlagBits::eFragment, "compiled_shaders/fragSkybox.spv")
		.build(m_vkCon.getDevice());
	

	// ======== Vertex Input Binding Description (Vertex Shader)
	std::vector<vk::VertexInputBindingDescription> bindingDescs{ Vertex::getBindingDescription() };
	auto inputAttrDescs{ Vertex::getAttributeDescriptions() };
	vk::PipelineVertexInputStateCreateInfo vertInC({}, bindingDescs, inputAttrDescs);

	// ======== Pipeline Layout (Layouts for Shader Inputs + Push Constant)
	// Order matters here
	std::vector<vk::DescriptorSetLayout> compatibleLayouts{
		m_engineDescriptorSetLayout.get(),		// Set 0
		m_passDescriptorSetLayout.get(),		// Set 1
		m_materialDescriptorSetLayout.get(),	// Set 2
		m_objectDescriptorSetLayout.get()		// Set 3
	};

	m_mainGfxPipelineLayout = dev.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, compatibleLayouts, m_pushConstantRange));
	//m_mainGfxPipelineLayout = dev.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, compatibleLayouts));







	// ======== Input Assembler State (Vertex Shader)
	vk::PipelineInputAssemblyStateCreateInfo iaC({}, vk::PrimitiveTopology::eTriangleList);

	// ======== Viewport & Scissor
	// https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
	auto scExtent = m_vkCon.getSwapchainExtent();
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



	m_mainGfxPipeline = dev.createGraphicsPipelineUnique({},
		vk::GraphicsPipelineCreateInfo({},
			shaderStageC,			// Shaders
			&vertInC,				// Input Layout
			&iaC,					// Input Assembler STATE (Topology)
			{},						// TesselationState
			&vpC,					// Viewport and Stencil 
			&rsC,					// Rasterizer State
			&msC,					// Multisample states
			&dsC,					// Depth stencil states
			&cbC,					// Color blend states
			{},						// Optional dynamic state
			m_mainGfxPipelineLayout.get(),
			m_defRenderPass.get(),	// Suitable render pass
			0
		)
	).value;

	// =============================== Skybox below 

	std::vector<vk::DescriptorSetLayout> compatibleLayoutsSB{
		m_engineDescriptorSetLayout.get(),		// Set 0
	};

	// Why do we need the push constant range?
	// Spec @Pipeline Layout Compatibility 14.2.2
	// Two pipeline layouts are defined to be ?compatible for push constants? if they were created with identical push constant ranges. 
	// Two pipeline layouts are defined to be ?compatible for set N? if they were created with 
	// identically defined descriptor set layouts for sets zero through N, || and if they were created with identical push constant ranges. || <-- Last bit 
	m_skyboxGfxPipelineLayout = dev.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, compatibleLayoutsSB, m_pushConstantRange));

	auto vertBinSB = readFile("compiled_shaders/vertSkybox.spv");
	auto fragBinSB = readFile("compiled_shaders/fragSkybox.spv");
	auto vertModSB = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, vertBinSB.size(), reinterpret_cast<uint32_t*>(vertBinSB.data())));
	auto fragModSB = dev.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, fragBinSB.size(), reinterpret_cast<uint32_t*>(fragBinSB.data())));



	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageCSB = {
	vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertModSB.get(), "main"),
	vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragModSB.get(), "main")
	};

	vk::PipelineVertexInputStateCreateInfo vertInCSB;	// empty

	m_skyboxGfxPipeline = dev.createGraphicsPipelineUnique({},
		vk::GraphicsPipelineCreateInfo({},
			shaderStageCSB,	
			&vertInCSB,			
			&iaC,				 
			{},
			&vpC,
			&rsC,
			&msC,
			&dsC,
			&cbC,
			{},
			//m_mainGfxPipelineLayout.get(),
			m_skyboxGfxPipelineLayout.get(),
			m_defRenderPass.get(),
			0
		)
	).value;


}

void SponzaApp::setupResources()
{
	m_defRenderPass = ezTmp::createDefaultRenderPass(m_vkCon);
	m_defFramebuffers = ezTmp::createDefaultFramebuffers(m_vkCon, m_defRenderPass.get());

	// Allocate pool for descriptors
	createDescriptorPool();

	// Create UBOs that will be used in this App
	createUBOs();

	// Set up Texture
	loadTextures();

	// Setup global sampler that will be used for all images
	vk::SamplerCreateInfo sCI({},
		vk::Filter::eLinear, vk::Filter::eLinear,	// min/mag filter
		vk::SamplerMipmapMode::eLinear,				// mipmapMode
		vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
		0.f,							// mipLodBias
		true, m_vkCon.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy,		// anisotropy enabled / max anisotropy (max clamp value) (we are just maxing out here (16))
		false, vk::CompareOp::eNever,	// compare enabled/op
		0.f, VK_LOD_CLAMP_NONE
	);
	m_commonSampler = m_vkCon.getDevice().createSamplerUnique(sCI);

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
	loadExternalModel("Resources/Objs/nanosuit_gunnar/nanosuit.obj");
	loadExternalModel("Resources/Objs/sponza_new/Sponza.obj");
	loadExternalModel("Resources/Objs/survival_backpack/backpack.obj");

	// Write skybox data
	vk::DescriptorImageInfo skyboxImageInfo(m_commonSampler.get(), m_mappedTextures["yokohamaSB"]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet skyboxImageSetWrite(m_engineDescriptorSet, 2, 0, vk::DescriptorType::eCombinedImageSampler, skyboxImageInfo, {}, {});
	m_vkCon.getDevice().updateDescriptorSets({ skyboxImageSetWrite }, {});
}


void SponzaApp::loadMaterial(std::string directory, AssimpMaterialPaths texturePaths)
{
	// Get final diffuse path
	std::string diffusePath(directory);
	if (texturePaths.diffuseFilePath.has_value())
		diffusePath += texturePaths.diffuseFilePath.value();
	else
		diffusePath = "Resources/Textures/defaulttexture.jpg";

	// Get final opacity path
	std::string opacityPath(directory);
	if (texturePaths.opacityFilePath.has_value())
		opacityPath += texturePaths.opacityFilePath.value();
	else
		opacityPath = "Resources/Textures/defaultopacity.jpg";

	// Get final specular path
	std::string specularPath(directory);
	if (texturePaths.specularFilePath.has_value())
		specularPath += texturePaths.specularFilePath.value();
	else
		specularPath = "Resources/Textures/defaultspecular.jpg";

	// Get final normal path
	std::string normalPath(directory);
	if (texturePaths.normalFilePath.has_value())
		normalPath += texturePaths.normalFilePath.value();
	else
		normalPath = "Resources/Textures/defaultnormal.jpg";

	// Parent paths is diffuse (identifier for descriptor set)
	uploadTexture(diffusePath, true, true, diffusePath, 0);
	uploadTexture(opacityPath, true, false, diffusePath, 1);
	uploadTexture(specularPath, true, false, diffusePath, 2);
	uploadTexture(normalPath, true, false, diffusePath, 3);

}

void SponzaApp::uploadTexture(std::string finalPath, bool genMips, bool srgb, std::string materialParentPath, uint32_t bindingSlot)
{
	if (m_mappedTextures.find(finalPath) == m_mappedTextures.cend())
		m_mappedTextures.insert({ finalPath, std::move(Texture::fromFile(m_vkCon, finalPath, genMips, srgb)) });

	// Parent (diffuse) (responsible for descriptor not yet initialized)
	if (m_mappedMaterials.find(materialParentPath) == m_mappedMaterials.cend())
	{
		// Create descriptor set with new material
		vk::DescriptorSetAllocateInfo texAllocInfo(m_descriptorPool.get(), m_materialDescriptorSetLayout.get());
		auto newMatDescSet = m_vkCon.getDevice().allocateDescriptorSets(texAllocInfo).front();
		m_mappedMaterials.insert({ materialParentPath, std::make_unique<Material>(m_mainGfxPipeline.get(), m_mainGfxPipelineLayout.get(), newMatDescSet) });
	}


	// Write to existing descriptor set (existing material that was made from diffuse) (bind image and sampler)
	vk::DescriptorImageInfo imageInfo(m_commonSampler.get(), m_mappedTextures[finalPath]->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet imageSetWrite(m_mappedMaterials[materialParentPath]->getDescriptorSet(), bindingSlot, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo, {}, {});
	m_vkCon.getDevice().updateDescriptorSets(imageSetWrite, {});
}

void SponzaApp::loadExternalModel(const std::filesystem::path& filePath)
{
	auto dev = m_vkCon.getDevice();
	std::string directory = filePath.parent_path().string() + "/";

	auto loader = AssimpLoader(filePath);
	auto& vertices = loader.getVertices();
	auto& indices = loader.getIndices();
	auto& subsets = loader.getSubsets();
	auto& materials = loader.getMaterials();




	// PACK DATA FOR VULKAN
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
		vertex.tangent.x = vert.tangent.x;
		vertex.tangent.y = vert.tangent.y;
		vertex.tangent.z = vert.tangent.z;
		vertex.bitangent.x = vert.bitangent.x;
		vertex.bitangent.y = vert.bitangent.y;
		vertex.bitangent.z = vert.bitangent.z;
		finalVerts.push_back(vertex);
	}

	// Push into VB/IB pair
	auto vb = Buffer::loadImmutable(m_vkCon, finalVerts, vk::BufferUsageFlagBits::eVertexBuffer);
	auto ib = Buffer::loadImmutable(m_vkCon, indices, vk::BufferUsageFlagBits::eIndexBuffer);


	// ======== Handle Subsets

	// Load materials
	// Here we should load the materials and let renderUnits below simply pick from the loaded materials

	for (auto& mat : materials)
		loadMaterial(directory, mat);

	std::vector<RenderUnit> renderUnits;
	renderUnits.reserve(subsets.size());
	for (const auto& subset : subsets)
	{
		auto mesh = Mesh(subset.indexStart, subset.indexCount, subset.vertexStart);

		// Get final diffuse path (material parent path)
		std::string diffusePath(directory);
		if (subset.diffuseFilePath.has_value())
			diffusePath += subset.diffuseFilePath.value();
		else
			diffusePath = "Resources/Textures/defaulttexture.jpg";

		renderUnits.push_back(RenderUnit(mesh, *m_mappedMaterials[diffusePath].get()));
	}

	std::sort(renderUnits.begin(), renderUnits.end());
	
	// lowercase file name as id for the loaded model asset
	auto fname = filePath.stem().string();
	std::for_each(fname.begin(), fname.end(), [](char& c) { c = std::tolower(c); });

	m_loadedModels.insert({ fname, std::make_unique<RenderModel>(std::move(vb), std::move(ib), renderUnits) });


}

}
