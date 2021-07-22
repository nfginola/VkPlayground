#version 450
#extension GL_ARB_separate_shader_objects : enable
        
// Match previous stages out with matching identifiers
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragPos;

const uint POINT_LIGHT_COUNT = 2;
const float SPOTLIGHT_DISTANCE = 77;

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

    vec4 spotlightPositionAndStrength;
	vec4 spotlightDirectionAndCutoff;

    vec4 pointLightPosition[POINT_LIGHT_COUNT];
    vec4 pointLightColor[POINT_LIGHT_COUNT];
    vec4 pointLightAttenuation[POINT_LIGHT_COUNT];
	
} sceneData;

layout(set = 2, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 2, binding = 1) uniform sampler2D opacityTexture;
layout(set = 2, binding = 2) uniform sampler2D specularTexture;


layout(location = 0) out vec4 outColor; 

vec3 calculateSpecularColor(vec3 normal, vec3 lightDirection, vec3 lightColor)
{
    // Disable specular if no diffuse
    float diffuseFactor = dot(-lightDirection, normal);
    if (diffuseFactor < 0.f)
        return vec3(0.f);
    
    // Regular Phong Specular
//    vec3 camPos = -vec3(engineUBO.viewMat[3].xyz);      // Remember, world moves in relation to camera 
//    vec3 dirToCam = normalize(camPos - fragPos);
//    vec3 reflectionDir = reflect(lightDirection, normal);
//
//    vec3 specular = pow(clamp(dot(dirToCam, reflectionDir), 0.f, 1.f), 32) * texture(specularTexture, fragUV).xyz * lightColor;     


    // Blinn Phong specular
    vec3 fragToCamDir = normalize((-engineUBO.viewMat[3].xyz) - fragPos);
    vec3 fragToLightDir = normalize(-lightDirection);
    vec3 halfwayDir = normalize(fragToCamDir + fragToLightDir);

    vec3 specular = pow(max(dot(normal, halfwayDir), 0.f), 32) * texture(specularTexture, fragUV).xyz * lightColor;


    return specular;
}


vec3 calculateDirectionalLight(vec3 direction, vec3 lightColor, vec3 normal)
{
    vec3 diffuse = clamp(dot(-direction, normal), 0.f, 1.f) * lightColor * texture(diffuseTexture, fragUV).xyz;
    vec3 specular = calculateSpecularColor(normal, direction, lightColor);

    return diffuse + specular;
}

vec3 calculatePointLight(vec3 normal, vec3 attenuation, vec3 color, vec3 position)
{
    // No ambient
    
    // Diffuse
    vec3 vecToLight = position - fragPos;
    float distToLight = length(vecToLight);
    vec3 dirToLight = normalize(vecToLight);
    float pointLightContrib = 1.f / (attenuation.x + attenuation.y * distToLight + attenuation.z * distToLight * distToLight);
  
    // This causes lighting bugs. We will turn off for now and let backsides get illuminated.. (If we implement shadow mapping then we can eliminate this)
    // Occlude point light contribution if more than perpendicular to normal (behind)
//    float pointLightDot = dot(dirToLight, normal) * pointLightContrib;
//    if (pointLightDot <= 0.f) 
//        pointLightContrib = 0.f;
    
    vec3 diffuse = pointLightContrib * color * texture(diffuseTexture, fragUV).xyz;

    // Specular
    vec3 specular = pointLightContrib * calculateSpecularColor(normal, -dirToLight, color);

    return diffuse + specular;
}

vec3 calculateSpotlight()
{
    vec3 lightToFrag = fragPos - sceneData.spotlightPositionAndStrength.xyz;
    float lighToFragDist = length(lightToFrag);
    vec3 lightToFragDir = normalize(lightToFrag);
    float factorFromView = dot(lightToFragDir, sceneData.spotlightDirectionAndCutoff.xyz);
    
    // Linear fall off factor (distance)
    float distanceFallOffFactor = ( 1.f - clamp(lighToFragDist, 0.f, SPOTLIGHT_DISTANCE) / SPOTLIGHT_DISTANCE);
    float spotlightStrength = sceneData.spotlightPositionAndStrength.w;

    // Linear fall off factor (edge)
    float outerCutoff = 0.91354546597f; // 24 degrees

    float edgeIntensity = clamp( (sceneData.spotlightDirectionAndCutoff.w - factorFromView) / (outerCutoff - sceneData.spotlightDirectionAndCutoff.w) , 0.f, 1.f);

    if (factorFromView > sceneData.spotlightDirectionAndCutoff.w)
        return vec3(spotlightStrength) * texture(diffuseTexture, fragUV).xyz * distanceFallOffFactor * edgeIntensity;

//    vec3 fragToLight = normalize(sceneData.spotlightPosition.xyz - fragPos);
//    float theta = dot(fragToLight, normalize(-sceneData.spotlightDirectionAndCutoff.xyz));
//    
//    if(theta > sceneData.spotlightDirectionAndCutoff.w) 
//        return vec3(0.1f, 0.1f, 0.1f);

    return vec3(0.f);
}



void main() 
{
    vec3 normal = normalize(fragNormal);
    vec3 diffuseColor = texture(diffuseTexture, fragUV).xyz;
    
    //vec3 co = calculateDirectionalLight(sceneData.directionalLightDirection.xyz, sceneData.directionalLightColor.xyz, normal);
    vec3 co = vec3(0.f);

    vec3 ambient = 0.007f * texture(diffuseTexture, fragUV).xyz;
    co += ambient;
    for (uint i = 0; i < POINT_LIGHT_COUNT; ++i)
    {
		co += calculatePointLight(normal, sceneData.pointLightAttenuation[i].xyz, sceneData.pointLightColor[i].xyz, sceneData.pointLightPosition[i].xyz);
    }

    co += calculateSpotlight();

    outColor = vec4(clamp(co, vec3(0.f), vec3(1.f)), texture(opacityTexture, fragUV).r);

}