#version 450
#extension GL_ARB_separate_shader_objects : enable
        
// Match previous stages out with matching identifiers
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;   // Pixel color to fill

void main() 
{
    outColor = vec4(fragUV, 0.0, 1.f);
}