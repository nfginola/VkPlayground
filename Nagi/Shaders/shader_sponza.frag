#version 450
#extension GL_ARB_separate_shader_objects : enable
        
// Match previous stages out with matching identifiers
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragPos;

const uint POINT_LIGHT_COUNT = 2;

layout(set = 0, binding = 0) uniform EngineUBO
{
	mat4 viewMat;
	mat4 projMat;
	mat4 viewProjMat;
} engineUBO;


layout(set = 0, binding = 1) uniform SceneUBO
{
    vec4 directionalLightDirection;
    vec4 directionalLightColor;

    vec4 pointLightPosition[POINT_LIGHT_COUNT];
    vec4 pointLightColor[POINT_LIGHT_COUNT];
    vec4 pointLightAttenuation[POINT_LIGHT_COUNT];
	
} sceneData;

layout(set = 2, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 2, binding = 1) uniform sampler2D opacityTexture;
layout(set = 2, binding = 2) uniform sampler2D specularTexture;


layout(location = 0) out vec4 outColor;   // Pixel color to fill

float calculateSpecularFactor(vec3 normal)
{
    vec3 camPos = -vec3(engineUBO.viewMat[3].xyz);      // Remember, world moves in relation to camera 
    vec3 dirToCam = normalize(camPos - fragPos);
    vec3 reflectionDir = reflect(sceneData.directionalLightDirection.xyz, normal);     // Directional light --> Incoming lightDirection

    float spec = pow(clamp(dot(dirToCam, reflectionDir), 0.f, 1.f), 128);
    return spec; 
}

vec3 calculateDiffuseFactor(vec3 normal)
{
    // Directional
    float directionalDiffuseFactor = clamp(dot(-sceneData.directionalLightDirection.xyz, normal), 0.f, 1.f);

    // Point Light
    vec3 vecToLight = sceneData.pointLightPosition[0].xyz - fragPos;
    float distToLight = length(vecToLight);
    vec3 dirToLight = normalize(vecToLight);
    float pointLightContrib = 1.f / (sceneData.pointLightAttenuation[0].x + sceneData.pointLightAttenuation[0].y * distToLight + sceneData.pointLightAttenuation[0].z * distToLight * distToLight);
  
    float pointLightDot = clamp(dot(dirToLight, normal) * pointLightContrib, 0.f, 1.f);
    if (pointLightDot <= 0.f) 
        pointLightContrib = 0.f;
    
    vec3 diffuseFactor = (directionalDiffuseFactor * sceneData.directionalLightColor.xyz + pointLightContrib * sceneData.pointLightColor[0].xyz);
    return diffuseFactor;
}






vec3 calculateSpecularColor(vec3 normal, vec3 lightDirection, vec3 lightColor)
{
    // Disable specular if no diffuse
    float diffuseFactor = dot(-lightDirection, normal);
    if (diffuseFactor < 0.f)
        return vec3(0.f);

    vec3 camPos = -vec3(engineUBO.viewMat[3].xyz);      // Remember, world moves in relation to camera 
    vec3 dirToCam = normalize(camPos - fragPos);
    vec3 reflectionDir = reflect(lightDirection, normal);

    vec3 specular = pow(clamp(dot(dirToCam, reflectionDir), 0.f, 1.f), 32) * texture(specularTexture, fragUV).xyz * lightColor;     

    return specular;
}


vec3 calculateDirectionalLight(vec3 direction, vec3 lightColor, vec3 normal)
{
    vec3 ambient = 0.01f * texture(diffuseTexture, fragUV).xyz;
    vec3 diffuse = clamp(dot(-direction, normal), 0.f, 1.f) * lightColor * texture(diffuseTexture, fragUV).xyz;
    vec3 specular = calculateSpecularColor(normal, direction, lightColor);

    return ambient + diffuse + specular;
}

vec3 calculatePointLight(vec3 normal, vec3 attenuation, vec3 color, vec3 position)
{
    // No ambient
    
    // Diffuse
    vec3 vecToLight = position - fragPos;
    float distToLight = length(vecToLight);
    vec3 dirToLight = normalize(vecToLight);
    float pointLightContrib = 1.f / (attenuation.x + attenuation.y * distToLight + attenuation.z * distToLight * distToLight);
  
    // Occlude point light contribution if more than perpendicular to normal (behind)
    float pointLightDot = dot(dirToLight, normal) * pointLightContrib;
    if (pointLightDot <= 0.f) 
        pointLightContrib = 0.f;
    
    vec3 diffuse = pointLightContrib * color * texture(diffuseTexture, fragUV).xyz;

    // Specular
    vec3 specular = pointLightContrib * calculateSpecularColor(normal, -dirToLight, color);

    return diffuse + specular;
}



void main() 
{


    vec3 normal = normalize(fragNormal);
    vec3 diffuseColor = texture(diffuseTexture, fragUV).xyz;
    
    vec3 co = calculateDirectionalLight(sceneData.directionalLightDirection.xyz, sceneData.directionalLightColor.xyz, normal);
    //vec3 co = vec3(0.f);
    for (uint i = 0; i < POINT_LIGHT_COUNT; ++i)
    {
		co += calculatePointLight(normal, sceneData.pointLightAttenuation[i].xyz, sceneData.pointLightColor[i].xyz, sceneData.pointLightPosition[i].xyz);
    }

    outColor = vec4(clamp(co, vec3(0.f), vec3(1.f)), texture(opacityTexture, fragUV).r);
    return;



    vec3 finalAmbientColor = 0.01f * diffuseColor;

    vec3 diffuseFactor = calculateDiffuseFactor(normal);
    vec3 finalDiffuseColor = diffuseFactor * diffuseColor;

    vec3 finalSpecularColor = calculateSpecularFactor(normal) * texture(specularTexture, fragUV).xyz * diffuseFactor;      // Using diffuse factor to remove backface specular!

//    outColor = vec4(specular, 1.f);
//    return;


    // Final color
    vec3 color = clamp(finalAmbientColor + finalDiffuseColor + finalSpecularColor, vec3(0.f), vec3(1.f));
    outColor = vec4(color, texture(opacityTexture, fragUV).r);      // Alpha mask
}