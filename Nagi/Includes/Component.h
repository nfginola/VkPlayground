#pragma once
#include <glm/glm.hpp>
#include "ResourceTypes.h" 

namespace Nagi
{
	struct TransformComponent
	{
		glm::mat4 mat = glm::mat4(1.f);

		TransformComponent(float x, float y, float z) { mat = glm::translate(glm::mat4(1.f), glm::vec3(x, y, z)); }
		glm::vec4 translation() { return glm::vec4(mat[3]); }
	};

	struct ModelRefComponent
	{
		RenderModel* model = nullptr;

		ModelRefComponent(RenderModel* mod = nullptr) : model(mod) {}
	};

	struct PointLightComponent
	{
		glm::vec4 color;
		glm::vec4 attenuation;

		PointLightComponent(glm::vec4 col, glm::vec4 att) : color(col), attenuation(att) {}
	};

	struct SpotlightComponent
	{
		glm::vec4 color;

	};

	struct DirectionalLightComponent
	{
		glm::vec4 color;
		glm::vec4 direction;

		DirectionalLightComponent(glm::vec4 col, glm::vec4 dir) : color(col), direction(dir) {}
	};


}
