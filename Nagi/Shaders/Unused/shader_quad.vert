#version 450
#extension GL_ARB_separate_shader_objects : enable

// Location can be seen as the identifier used for in/out from this stage to other stages
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

// 1. Push constants
layout(push_constant) uniform constants
{
	mat4 mat;
	vec4 data;
} PushConstants;

layout(set = 0, binding = 0) uniform UBO{
	mat4 mat;
	vec4 data;
} UBOThing;

void main() {
	//gl_Position = vec4(inPos, 1.f);		// gl_Position same as SV_Position
	//gl_Position = PushConstants.mat * vec4(inPos, 1.f);
	gl_Position = UBOThing.mat * vec4(inPos, 1.f);


	// Note that these will be lineraly interpolated by the rasterizer :)
	//fragColor = inColor;
	fragColor = PushConstants.data.xyz;
	fragUV = inUV;
}