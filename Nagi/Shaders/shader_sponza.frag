#version 450
#extension GL_ARB_separate_shader_objects : enable
        
// Match previous stages out with matching identifiers
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragPos;


layout(set = 0, binding = 0) uniform EngineUBO
{
	mat4 viewMat;
	mat4 projMat;
	mat4 viewProjMat;
} engineUBO;

layout(set = 0, binding = 1) uniform SceneUBO
{
    vec4 lightDirection;
    vec4 lightColor;
	
} sceneData;

layout(set = 2, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 2, binding = 1) uniform sampler2D opacityTexture;
layout(set = 2, binding = 2) uniform sampler2D specularTexture;


layout(location = 0) out vec4 outColor;   // Pixel color to fill

vec3 calculateSpecular(vec3 normal)
{
    vec3 camPos = -vec3(engineUBO.viewMat[3].xyz);      // Remember, world moves in relation to camera 
    vec3 dirToCam = normalize(camPos - fragPos);
    vec3 reflectionDir = reflect(sceneData.lightDirection.xyz, normal);     // Directional light --> Incoming lightDirection

    float spec = pow(clamp(dot(dirToCam, reflectionDir), 0.f, 1.f), 128);
    return (spec * texture(specularTexture, fragUV)).xyz;  
}


void main() 
{
    vec3 normal = normalize(fragNormal);
    vec3 diffuseColor = texture(diffuseTexture, fragUV).xyz;

    vec3 ambient = 0.01f * diffuseColor;

    float diffuseFactor = clamp(dot(-sceneData.lightDirection.xyz, normal), 0.f, 1.f);
    vec3 diffuse =  diffuseFactor * diffuseColor;

    vec3 specular = diffuseFactor * calculateSpecular(normal);      // Using diffuse factor to remove backface specular!

//    outColor = vec4(specular, 1.f);
//    return;



    // Final color
    vec3 color = clamp(diffuse + ambient + specular, vec3(0.f), vec3(1.f));
    outColor = vec4(color, texture(opacityTexture, fragUV).r);      // Alpha mask
}