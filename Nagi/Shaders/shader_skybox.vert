#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "per_frame_res"

const vec4 CUBE[36] =
{
// Front 1
vec4(0.5f, -0.5f, -0.5f, 0.5f),
vec4(-0.5f, 0.5f, -0.5f, 0.5f),
vec4(-0.5f, -0.5f, -0.5f, 0.5f),

// Front 2
vec4(-0.5f, 0.5f, -0.5f, 0.5f),
vec4(0.5f, -0.5f, -0.5f, 0.5f),
vec4(0.5f, 0.5f, -0.5f, 0.5f),

// Back 1
vec4(-0.5f, -0.5f, 0.5f, 0.5f),
vec4(0.5f, 0.5f, 0.5f, 0.5f),
vec4(0.5f, -0.5f, 0.5f, 0.5f),

// Back 2
vec4(0.5f, 0.5f, 0.5f, 0.5f),
vec4(-0.5f, -0.5f, 0.5f, 0.5f),
vec4(-0.5f, 0.5f, 0.5f, 0.5f),


vec4(-0.5f, -0.5f, -0.5f, 0.5f),
vec4(-0.5f, 0.5f, 0.5f, 0.5f),
vec4(-0.5f, -0.5f, 0.5f, 0.5f),
    
vec4(-0.5f, 0.5f, 0.5f, 0.5f),
vec4(-0.5f, -0.5f, -0.5f, 0.5f),
vec4(-0.5f, 0.5f, -0.5f, 0.5f),



vec4(0.5f, -0.5f, 0.5f, 0.5f),
vec4(0.5f, 0.5f, -0.5f, 0.5f),
vec4(0.5f, -0.5f, -0.5f, 0.5f),

vec4(0.5f, 0.5f, -0.5f, 0.5f),
vec4(0.5f, -0.5f, 0.5f, 0.5f),
vec4(0.5f, 0.5f, 0.5f, 0.5f),



vec4(0.5f, 0.5f, -0.5f, 0.5f),
vec4(-0.5f, 0.5f, 0.5f, 0.5f),
vec4(-0.5f, 0.5f, -0.5f, 0.5f),

vec4(-0.5f, 0.5f, 0.5f, 0.5f),
vec4(0.5f, 0.5f, -0.5f, 0.5f),
vec4(0.5f, 0.5f, 0.5f, 0.5f),



vec4(0.5f, -0.5f, 0.5f, 0.5f),
vec4(-0.5f, -0.5f, -0.5f, 0.5f),
vec4(-0.5f, -0.5f, 0.5f, 0.5f),
    
vec4(-0.5f, -0.5f, -0.5f, 0.5f),
vec4(0.5f, -0.5f, 0.5f, 0.5f),
vec4(0.5f, -0.5f, -0.5f, 0.5f)
};


layout(location = 0) out vec3 wsPos;

//layout(set = 0, binding = 0) uniform EngineUBO
//{
//	mat4 viewMat;
//	mat4 projMat;
//	mat4 viewProjMat;
//} engineUBO;

void main()
{
    wsPos = CUBE[gl_VertexIndex].xyz;

    gl_Position = engineUBO.viewProjMat * vec4(wsPos, 0.f);
    gl_Position.z = gl_Position.w;  // make distance infinite (after pers divide --> dist is 1 in NDC

    wsPos.z *= -1.f;        // Cubemaps are in LH space --> Our object is in RH.
}

