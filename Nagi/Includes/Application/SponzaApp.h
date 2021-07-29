#pragma once
#include "Application.h"

namespace Nagi
{

struct Vertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;

	constexpr static int s_bindingSlot = 0;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		// Binding on Vertex Buffer slot 0 hardcoded
		return 	vk::VertexInputBindingDescription{ s_bindingSlot, sizeof(Vertex) };
	}

	static std::array<vk::VertexInputAttributeDescription, 5> getAttributeDescriptions()
	{
		std::array<vk::VertexInputAttributeDescription, 5> dscs;
		dscs[0] = vk::VertexInputAttributeDescription(0, s_bindingSlot, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos));
		dscs[1] = vk::VertexInputAttributeDescription(1, s_bindingSlot, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv));
		dscs[2] = vk::VertexInputAttributeDescription(2, s_bindingSlot, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal));
		dscs[3] = vk::VertexInputAttributeDescription(3, s_bindingSlot, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, tangent));
		dscs[4] = vk::VertexInputAttributeDescription(4, s_bindingSlot, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, bitangent));
		return dscs;
	}
};

// Temporary, modelMat should live in Set 3 (per object data)
struct PushConstantData
{
	glm::mat4 modelMat;
};

struct ObjectData
{
	glm::mat4 modelMat;
};

struct GPUCameraData
{
	glm::mat4 viewMat;
	glm::mat4 projectionMat;
	glm::mat4 viewProjectionMat;
};

struct SceneData
{
	glm::vec4 directionalLightDirection;
	glm::vec4 directionalLightColor;

	glm::vec4 spotlightPositionAndStrength;
	glm::vec4 spotlightDirectionAndCutoff;

	glm::vec4 pointLightPosition[2];
	glm::vec4 pointLightColor[2];
	glm::vec4 pointLightAttenuation[2];
};

struct EngineFrameData
{
	// Resources
	std::unique_ptr<Buffer> cameraBuffer;
	std::unique_ptr<Buffer> sceneBuffer;

	// Set that encapsulates above resources
	vk::DescriptorSet descriptorSet;
	// Let the pool destruction take care of descriptor set deallocation
};


struct ObjectFrameData
{
	std::unique_ptr<Buffer> modelMatBuffer;

	vk::DescriptorSet descriptorSet;
};

class SponzaApp : public Application
{
public:
	SponzaApp(Window& window, VulkanContext& gfxCon);
	~SponzaApp();

	SponzaApp() = delete;
	SponzaApp& operator=(const Application&) = delete;

private:

	void drawObjects(vk::CommandBuffer& cmd, float timeElapsed);

	void createDescriptorPool();

	void loadExternalModel(const std::filesystem::path& filePath);

	void createRenderModels();

	void createUBOs();
	void loadTextures();

	void setupDescriptorSetLayouts();
	void configurePushConstantRange();

	void allocateDescriptorSets();
	
	void createGraphicsPipeline();

	void setupResources();


private:
	vk::UniqueRenderPass m_defRenderPass;
	std::vector<vk::UniqueFramebuffer> m_defFramebuffers;

	std::vector<EngineFrameData> m_engineFrameData;

	// set cleaned up automatically when pool is destroyed
	vk::DescriptorSet m_engineDescSet;		// we will be using a single descriptor set for engine data (resources with offsets!) --> One buffer for all
	std::unique_ptr<Buffer> m_engineFrameBuffer;



	std::unique_ptr<Buffer> m_sceneBufferDynamicSplit;
	std::unique_ptr<Buffer> m_sceneBufferDynamicUB;		

	std::vector<ObjectFrameData> m_objectFrameData;
	vk::UniqueDescriptorPool m_descriptorPool;

	vk::UniqueDescriptorSetLayout m_engineDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout m_engineDescriptorSetLayout2;
	vk::UniqueDescriptorSetLayout m_passDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout m_materialDescriptorSetLayout;
	vk::UniqueDescriptorSetLayout m_objectDescriptorSetLayout;
	vk::PushConstantRange m_pushConstantRange;

	vk::UniquePipelineLayout m_mainGfxPipelineLayout;
	vk::UniquePipeline m_mainGfxPipeline;

	vk::UniqueSampler m_commonSampler;

	std::map<std::string, std::unique_ptr<Texture>> m_mappedTextures;
	std::map<std::string, std::unique_ptr<Material>> m_mappedMaterials;
	
	std::vector<std::unique_ptr<RenderModel>> m_loadedModels;
};

}



