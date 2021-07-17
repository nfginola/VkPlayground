#version 450
#extension GL_ARB_separate_shader_objects : enable
        
// Match previous stages out with matching identifiers
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(set = 2, binding = 0) uniform sampler2D albedoTexture;
layout(set = 2, binding = 1) uniform sampler2D opacityTexture;


layout(location = 0) out vec4 outColor;   // Pixel color to fill

void main() 
{
    //outColor = vec4(fragColor, 1.f);
    //outColor = vec4(fragUV, 0.0, 1.f);

    vec3 color =  texture(albedoTexture, fragUV).xyz;
    outColor = vec4(color, texture(opacityTexture, fragUV).r);      // Alpha mask
}