#version 450
#extension GL_ARB_separate_shader_objects : enable

// Location can be seen as the identifier used for in/out from this stage to other stages
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;

layout(push_constant) uniform Constants
{
	mat4 modelMat;
} pushConstants;

layout(set = 0, binding = 0) uniform EngineUBO
{
	mat4 viewMat;
	mat4 projMat;
	mat4 viewProjMat;
} engineUBO;


//layout(set = 3, binding = 0) uniform ObjectUBO
//{
//	mat4 modelMat;
//} objectUBO;

void main() 
{
	vec4 worldPos = pushConstants.modelMat * vec4(inPos, 1.f);
	gl_Position = engineUBO.viewProjMat * worldPos;
	fragPos = worldPos.xyz;
	fragUV = inUV;
	fragNormal = normalize((pushConstants.modelMat * vec4(inNormal, 0.f)).xyz);

	//vec3 tangent = normalize((pushConstants.modelMat * vec4(inTangent, 0.f)).xyz);
	fragTangent = normalize((pushConstants.modelMat * vec4(normalize(inTangent), 0.f)).xyz);
	//vec3 bitangent = normalize((pushConstants.modelMat * vec4(inBitangent, 0.f)).xyz);
	fragBitangent = normalize((pushConstants.modelMat * vec4(normalize(inBitangent), 0.f)).xyz);

	//fragTBN = mat3(tangent, bitangent, fragNormal);

}