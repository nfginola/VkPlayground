#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "per_frame_res"

layout(location = 0) in vec3 wsPos;

//layout(set = 0, binding = 2) uniform samplerCube skyboxTexture;

layout(location = 0) out vec4 outColor; 

void main()
{
	outColor = texture(skyboxTexture, normalize(wsPos));
}