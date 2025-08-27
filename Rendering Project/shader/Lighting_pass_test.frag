#version 420 core

// Input data from Vertex 
in vec2 TexCoord;

// G-buffer textures
uniform sampler2D gPosition;
uniform sampler2D gNormalShininess;
uniform sampler2D gColorSpec;
uniform sampler2D gDepth;

// Shadow map samplers
uniform samplerCubeArrayShadow shadowCubeArray;   // For Point Lights
uniform sampler2D shadowDir;                // For Directional Lights
uniform sampler2DArrayShadow  shadowSpotArray;     // For Spot Lights


// Camera position for specular calculations
uniform vec3 viewPos;

// Output 
out vec4 FragColor;

// Light structures
struct Light {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct DirLight {
    vec3 position; // to keep track of the center of the light for the shadow map
    vec3 direction;
    Light light;
};

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
    
    Light light;
    float far_plane;
    int shadowID;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
    mat4 SpaceMatrices;
    Light light;    
    int shadowID;
};

// Light uniforms

#define MAX_POINT_LIGHTS 32
#define MAX_SPOT_LIGHTS 16

// Light Input data  
uniform int numPointLights;
uniform int numSpotLights;

uniform DirLight dirLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

// Light space matrices for shadows
uniform mat4 dirLightSpaceMatrice;



// Function prototypes
vec3 CalcDirLight(DirLight light);
vec3 CalcPointLight(PointLight light);
vec3 CalcSpotLight(SpotLight light);

float CalcPointLightShadow(vec3 fragPos, PointLight light);
float CalcDirLightShadow(vec3 fragPos, DirLight light);
float CalcSpotLightShadow(vec3 fragPos, SpotLight light);

vec3 ReinhardToneMapping(vec3 color);

vec3 DebugShadowVisualizationPOINT();
vec3 DebugShadowVisualizationDIR();

float LinearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}


// Global variable
vec3 FragPos;
vec3 diffuseColor;
vec3 Normal;
float shininess;
vec3 viewDir;
float specularIntensity;
float depth;

void main()
{
    // Sample G-buffer data
    FragPos = texture(gPosition, TexCoord).rgb;
    vec4 normalShininess = texture(gNormalShininess, TexCoord);
    Normal = normalShininess.rgb;
    shininess = normalShininess.a;
    
    vec4 colorSpec = texture(gColorSpec, TexCoord);
    diffuseColor = colorSpec.rgb;
    specularIntensity = colorSpec.a;

     depth = texture(gDepth, TexCoord).r;
    
    // If no geometry was rendered to this pixel, discard
    if (length(Normal) < 0.1) {
        discard;
    }
    
    // Calculate view direction
    viewDir = normalize(viewPos - FragPos);
    
    // Initialize result color
    vec3 result = vec3(0.0);
    
    // Calculate directional light (sunlight)
    {
       float visibility = CalcDirLightShadow(FragPos, dirLight);
       result += CalcDirLight(dirLight) * visibility; 
    }
    
    // Calculate point lights
    for(int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; ++i) 
    {
        // dont render far light 
        if (length(pointLights[i].position - FragPos) > pointLights[i].far_plane) 
        {
            continue; // Go to the next light
        }
        float visibility = CalcPointLightShadow(FragPos, pointLights[i]);
        result += CalcPointLight(pointLights[i]) * visibility;  
    }
    
    // Calculate spot lights
    for(int i = 0; i < numSpotLights && i < MAX_SPOT_LIGHTS; ++i) 
    {
       float visibility = CalcSpotLightShadow(FragPos, spotLights[i]);
       result += CalcSpotLight(spotLights[i]) * visibility;
    }
    
    
    FragColor = vec4( result ,1.0);
   // FragColor =  vec4(DebugShadowVisualizationDIR(),1.0);
    


}
// Calculates the color when using a directional light
vec3 CalcDirLight(DirLight light)
{
    vec3 lightDir = normalize(-light.direction);
    
    // Diffuse shading
    float diff = max(dot(Normal, lightDir), 0.0);
    
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, Normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    // Combine results
    vec3 ambient = light.light.ambient * diffuseColor;
    vec3 diffuse = light.light.diffuse * diff * diffuseColor;
    vec3 specular = light.light.specular * spec * specularIntensity;
    
    return (ambient + diffuse + specular);
}

// Calculates the color when using a point light
vec3 CalcPointLight(PointLight light)
{
    vec3 lightDir = normalize(light.position - FragPos);
    
    // Diffuse shading
    float diff = max(dot(Normal, lightDir), 0.0);
    
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, Normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    // Attenuation
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Combine results
    vec3 ambient = light.light.ambient * diffuseColor;
    vec3 diffuse = light.light.diffuse * diff * diffuseColor;
    vec3 specular = light.light.specular * spec * specularIntensity;
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return (ambient + diffuse + specular);
}

// Calculates the color when using a spot light
vec3 CalcSpotLight(SpotLight light)
{
    vec3 lightDir = normalize(light.position - FragPos);
    
    // Diffuse shading
    float diff = max(dot(Normal, lightDir), 0.0);
    
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, Normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    // Attenuation
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // Combine results
    vec3 ambient = light.light.ambient * diffuseColor;
    vec3 diffuse = light.light.diffuse * diff * diffuseColor;
    vec3 specular = light.light.specular * spec * specularIntensity;
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular);
}

// Using a "trilinear" style offset pattern.
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1), 
   vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
   vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
   vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
);
// Shared PCF sampling kernel for 2D shadow maps
vec2 poissonDisk[16] = vec2[](
    vec2(-0.7071, 0.7071), vec2(-0.0000, -0.8750),
    vec2(0.5303, 0.5303), vec2(-0.6250, -0.0000),
    vec2(0.3536, -0.3536), vec2(-0.1768, 0.8839),
    vec2(0.7071, -0.7071), vec2(-0.8839, -0.1768),
    vec2(0.0000, 0.6250), vec2(0.8839, 0.1768),
    vec2(-0.3536, -0.3536), vec2(0.1768, -0.8839),
    vec2(0.6250, 0.0000), vec2(-0.5303, 0.5303),
    vec2(-0.0000, 0.8750), vec2(0.8750, -0.0000)
);



float CalcPointLightShadow(vec3 fragPos, PointLight light) 
{
    vec3 lightToFrag = fragPos - light.position;
    
    // 1. Current depth from light's perspective, normalized to [0, 1]
    float currentDepth = length(lightToFrag) / light.far_plane;
    
    // 2. Apply bias to prevent shadow acne
    vec3 lightDir = normalize(lightToFrag);
    float bias = max(0.01 * (1.0 - dot(Normal, lightDir)), 0.001);
    
    // 3. Sample with hardware shadow comparison - NOW RETURNS 0.0 (shadow) or 1.0 (lit)
    float shadowResult = texture(shadowCubeArray, vec4(lightToFrag, light.shadowID), currentDepth - bias);
    
    // This determines how wide we spread our samples.
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.2 + viewDistance / light.far_plane) * 0.01;
    

    // --- PCF Implementation ---
    float shadow = 0.0; // This will count how many samples are "lit"
    int samples = 20;
    
    for(int i = 0; i < samples; ++i)
    {
        // Sample with hardware shadow comparison at offset positions
        float pcfResult = texture(shadowCubeArray, vec4(lightToFrag + gridSamplingDisk[i] * diskRadius, light.shadowID), currentDepth - bias);
        shadow += pcfResult; // Add the lit contribution (1.0 = lit, 0.0 = shadow)
    }
    
    // Average the result
    shadow /= float(samples);
    
    // shadow is now the percentage of lighting (0.0 = fully shadowed, 1.0 = fully lit)
    return  shadow;
}

float CalcDirLightShadow(vec3 fragPos, DirLight light)
{
    // Transform fragment position to light space
    vec4 fragPosLightSpace = dirLightSpaceMatrice * vec4(fragPos, 1.0);
    
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range for texture coordinates
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get current depth from light's perspective
    float currentDepth = projCoords.z;
    
    // Calculate bias to prevent shadow acne
    vec3 lightDir = normalize(-light.direction);
    float bias = max(0.001 * (1.0 - dot(Normal, lightDir)), 0.001);
    
    // early exit Don't cast shadows if fragment is outside the light's frustum
    if(projCoords.z > 1.0)
        return  1.0;

    // PCF - Percentage-Closer Filtering
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowDir, 0).xy;
    
    float viewDistance = length(viewPos - fragPos);
    
    float radius = 0.5 + 2.0 * (1.0 - exp(-viewDistance * 0.3 ));

    for(int i = 0; i < 16; i++)
    {
        float angle = 2.399963 * float(i); // Golden angle in radians
        float r = sqrt(float(i) + 0.5) / 4.0; // V  tribution
    
        vec2 offset = vec2(cos(angle), sin(angle)) * r * radius * texelSize;
        float pcfDepth = texture(shadowDir, projCoords.xy + offset).r;
    
        if(currentDepth - bias > pcfDepth)
            shadow += 1.0;
    }
    shadow /= 16.0;
    
    
        
    return 1.0 - shadow;
}


float CalcSpotLightShadow(vec3 fragPos, SpotLight light)
{
    // 1. Transform fragment position to light space
    vec4 fragPosLightSpace = light.SpaceMatrices * vec4(fragPos, 1.0);

    // 2. Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // 3. Map to [0, 1] for texture coordinates
    projCoords = projCoords * 0.5 + 0.5;

    // 4. Get depth to compare
    float currentDepth = projCoords.z;

    // 5. If outside the light's frustum, skip shadow
    if (projCoords.z > 1.0)
        return 1.0;

    // 6. Compute bias (tune this!)
    vec3 lightDir = normalize(light.position - fragPos);
    float bias = max(0.005 * (1.0 - dot(Normal, lightDir)), 0.001);

    // 7. PCF sampling using hardware comparison
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowSpotArray, 0).xy;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 offset = vec2(x, y) * texelSize;
            shadow += texture(shadowSpotArray, vec4(projCoords.xy + offset, light.shadowID, currentDepth - bias));
            }
    }

    shadow /= 9.0; // 3x3 kernel

    return shadow;
}

vec3 ReinhardToneMapping(vec3 color)
{
        // Add an exposure control
    float exposure = 1.0; // Make this a uniform
    color *= exposure;

    // A simplified ACES approximation
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}


// debug function to visualize different parts of the shadow process.
vec3 DebugShadowVisualizationPOINT()
{
    // --- SETUP ---
    // Make sure we have a light to test against
    if (numPointLights == 0) {
        return vec3(1.0, 0.0, 1.0); // Return magenta to indicate no lights
    }
    // Use the first point light for debugging
    PointLight light = pointLights[1];
    
    vec3 lightToFrag = FragPos - light.position;
    
    // The fragment's distance from the light, normalized to [0, 1]
    float currentDepth = length(lightToFrag) / light.far_plane;
    
    // The depth stored in the shadow map (closest occluder), already normalized
    float closestDepth =  texture(shadowCubeArray, vec4(gridSamplingDisk[0] , light.shadowID), currentDepth );


    // --- DEBUG MODES ---

    // 1. Visualize the Shadow Map itself.
    // This is the MOST IMPORTANT test. You should see a grayscale representation of your
    // scene from the light's point of view. If this is all white or all black,
    // your depth map generation pass is broken.
    return vec3(closestDepth);

    // 2. Visualize the fragment's calculated depth from the light.
    // This should look like a smooth gradient centered on the light.
    // Comparing this to the view above helps diagnose issues.
    // return vec3(currentDepth);

    // 3. Visualize the raw shadow comparison (no PCF).
    // This will show hard, aliased shadows. White = in shadow, Black = lit.
    // If you see black/white areas, the basic comparison is working.
    // float bias = 0.005;
    // if (currentDepth - bias > closestDepth) {
    //     return vec3(1.0); // In shadow (white)
    // }
    // return vec3(0.0); // Lit (black)

    // 4. Visualize the shadow factor from your full function.
    // This will test your complete `calculatePointLightShadow` function.
    // Should show soft shadows if PCF is working. (Black = shadow, White = lit).
    // float shadowFactor = calculatePointLightShadow(FragPos, light);
    // return vec3(shadowFactor);
}

vec3 DebugShadowVisualizationDIR()
{
    // --- SETUP ---
    // Make sure we have a light to test against
    if (numPointLights == 0) {
        return vec3(1.0, 0.0, 1.0); // Return magenta to indicate no lights
    }
    // Use the first point light for debugging
    DirLight light = dirLight;
    
    vec3 lightToFrag = FragPos - light.position;
    
    // The fragment's distance from the light, normalized to [0, 1]
    vec4 fragPosLightSpace = dirLightSpaceMatrice * vec4(FragPos, 1.0);
    
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // The depth stored in the shadow map (closest occluder), already normalized
    float closestDepth = texture(shadowDir,projCoords.xy).r;


    // --- DEBUG MODES ---

    // 1. Visualize the Shadow Map itself.
    // This is the MOST IMPORTANT test. You should see a grayscale representation of your
    // scene from the light's point of view. If this is all white or all black,
    // your depth map generation pass is broken.
    return vec3(closestDepth);

    // 2. Visualize the fragment's calculated depth from the light.
    // This should look like a smooth gradient centered on the light.
    // Comparing this to the view above helps diagnose issues.
    // return vec3(currentDepth);

    // 3. Visualize the raw shadow comparison (no PCF).
    // This will show hard, aliased shadows. White = in shadow, Black = lit.
    // If you see black/white areas, the basic comparison is working.
    // float bias = 0.005;
    // if (currentDepth - bias > closestDepth) {
    //     return vec3(1.0); // In shadow (white)
    // }
    // return vec3(0.0); // Lit (black)

    // 4. Visualize the shadow factor from your full function.
    // This will test your complete `calculatePointLightShadow` function.
    // Should show soft shadows if PCF is working. (Black = shadow, White = lit).
    // float shadowFactor = calculatePointLightShadow(FragPos, light);
    // return vec3(shadowFactor);
}


