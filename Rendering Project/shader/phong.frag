// Fragment Shader
#version 330 core

// Input from vertex shader
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in mat3 TBN;
in vec4 LightSpacePos;

// Output
out vec4 FragColor;

// Material properties
struct Material {
    sampler2D diffuse;      // Diffuse texture
    sampler2D specular;     // specular  texture
    sampler2D normal;       // normal texture
    sampler2D alpha;        // alpha texture
    vec3 diffuseColor;      // Diffuse color (used if no texture)
    vec3 ambientColor;      // Ambient color
    vec3 specularColor;     // Specular color
    float shininess;        // Specular shininess factor
};

// Light properties
struct Light {         // Light position
    vec3 ambient;          // Ambient light color
    vec3 diffuse;          // Diffuse light color
    vec3 specular;         // Specular light color
};

struct DirLight {
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
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    Light light;      
};

// Uniforms
uniform Material material;
uniform PointLight pointLight;
uniform DirLight dirLight;

uniform vec3 viewPos;      // Camera position for specular calculation
uniform bool hasDiffuseTexture;  // Whether to use texture or color
uniform bool hasSpecularTexture;  // Whether to use texture or color
uniform bool hasNormalTexture;
uniform bool hasAlphaTexture;

uniform sampler2D shadowMap;
uniform samplerCube shadowMapCube;


// tesing stuff
uniform float near_plane;
uniform float far_plane;  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}


// function prototypes
mat3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
mat3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
mat3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float ShadowCalculation(vec4 fragPosLightSpace,DirLight dirLight, vec3 normal);
float ShadowCalculationPoint(vec3 fragpos, PointLight light,vec3 normal);

float pippo (vec4 fragPosLightSpace,float otherLightDistance);

void main()
{
    vec3 norm;
    
    if (hasNormalTexture)
    {
        // obtain normal from normal map in range [0,1]
        norm = texture(material.normal,TexCoord).rgb;
        // transform normal vector to range [-1,1]
        norm = norm * 2.0 - 1.0;   
                norm = normalize(TBN * norm); 
    }
    else
    {
        norm = normalize(Normal);
    }
    float alphaValue = 1.f;
    if (hasAlphaTexture)
    {
    alphaValue = texture(material.alpha, TexCoord).r;
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    // matrix in first column ambinet, second column diffuse , third column specular
    mat3 pointADS = CalcPointLight(pointLight, norm, FragPos, viewDir);
    mat3 dirADS = CalcDirLight(dirLight, norm, viewDir);

    // Apply shadows to respective light contributions
    float shadowPoint = ShadowCalculationPoint(FragPos, pointLight, norm);
    float shadowSun = ShadowCalculation(LightSpacePos, dirLight, norm);

    vec3 result = max(pointADS[0], dirADS[0]); // Ambient from both
    result += (1.0 - shadowPoint) * (pointADS[1] + pointADS[2]); // Point light diffuse/specular
    result += (1.0 - shadowSun) * (dirADS[1] + dirADS[2]); // Directional light diffuse/specular

   FragColor = vec4( result , alphaValue); // Visualize normal components
   // = vec4(vec3(alphaValue),1.f);
}

// calculates the color when using a directional light.
mat3 CalcDirLight(DirLight dirLight, vec3 normal, vec3 viewDir)
{
    // light direction inverse 
    vec3 lightDir = normalize(-dirLight.direction);

    // ADS Ambient, Diffuse, Specular
    mat3 ADS;
    // ambient 
    
    if (hasDiffuseTexture) {
        ADS[0] = dirLight.light.ambient * vec3(texture(material.diffuse, TexCoord));
    } else {
        ADS[0] = dirLight.light.ambient * material.diffuseColor;
    }

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    
    if (hasDiffuseTexture) {
        ADS[1] = dirLight.light.diffuse * diff * vec3(texture(material.diffuse, TexCoord));
    } else {
        ADS[1] = dirLight.light.diffuse * diff * material.diffuseColor;
    }
    // Specular
    
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    if (hasSpecularTexture) {
        ADS[2] = dirLight.light.specular * spec * vec3(vec3(texture(material.specular, TexCoord)).r ) ; 
       
    } else {
        ADS[2]  = dirLight.light.specular * spec * material.specularColor;
    }

    return ADS;
}

// calculates the color when using a point light.
mat3 CalcPointLight(PointLight pointLight, vec3 normal, vec3 fragPos, vec3 viewDir)
{
 
    // Calculate light direction
    vec3 lightDir = normalize(pointLight.position - FragPos);

    // Calculate reflect direction
    vec3 reflectDir = reflect(-lightDir, normal);

    // Ambient
    vec3 ambient;
    if (hasDiffuseTexture) {
        ambient = pointLight.light.ambient * vec3(texture(material.diffuse, TexCoord));
    } else {
        ambient = pointLight.light.ambient * material.diffuseColor;
    }

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse;
    if (hasDiffuseTexture) {
        diffuse = pointLight.light.diffuse * diff * vec3(texture(material.diffuse, TexCoord));
    } else {
        diffuse = pointLight.light.diffuse * diff * material.diffuseColor;
    }
    // Specular
    vec3 specular;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    if (hasSpecularTexture) {
        specular = pointLight.light.specular * spec * vec3(vec3(texture(material.specular, TexCoord)).r ) ; 
    } else {
        specular = pointLight.light.specular * spec * material.specularColor;
    }
    mat3 ADS ;
    ADS[0].xyz = ambient;
    ADS[1].xyz = diffuse;
    ADS[2].xyz = specular;

    return ADS;
}

float ShadowCalculation(vec4 fragPosLightSpace, DirLight dirLight, vec3 normal)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = (projCoords * 0.5) + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    
    if (projCoords.z > 1.0) 
    return 0.0;  // Outside light's frustum
    // check whether current frag pos is in shadow
    float bias = max(0.00025 * (1.0 - dot(normal, dirLight.direction)), 0.001);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    return shadow;
} 

float ShadowCalculationPoint(vec3 fragpos, PointLight light,vec3 normal)
{
    vec3 fragTolight = fragpos - light.position;

    float closestDepth = texture(shadowMapCube,fragTolight).r;
    float invFarPlane = light.far_plane; // avoid re-sampling 
    // The closestDepth value is currently in the range [0,1] so we first transform it back to [0,far_plane] by multiplying it with far_plane
    closestDepth = closestDepth * invFarPlane; 

    float currentDepth = length(fragTolight);
    vec3 lightDir = normalize(light.position - fragpos);

    float shadow  = 0.0;
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.05);
    
    // early exit 
    float centerDepth = texture(shadowMapCube, fragTolight).r * invFarPlane;
    if (currentDepth - bias <= centerDepth)     return 0.0; // fully lit
    

    // PCF (Percentage Closer Filtering)
    float samples = 3.0;
    float offset  = 0.1;

    for(float x = -offset; x < offset; x += offset / (samples * 0.5))
    {
        for(float y = -offset; y < offset; y += offset / (samples * 0.5))
        {
            for(float z = -offset; z < offset; z += offset / (samples * 0.5))
            {
                float closestDepth = texture(shadowMapCube, fragTolight + vec3(x, y, z)).r; 
                closestDepth *= invFarPlane;   // undo mapping [0;1]
                if(currentDepth - bias > closestDepth)
                    shadow += 1.0;
            }
        }
    }
    shadow /= (samples * samples * samples);
    return shadow;
}

float pippo(vec4 fragPosLightSpace,float otherLightDistance)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = (projCoords * 0.5) + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    float bias =  0.00025;
    // check whether current frag pos is in shadow
    float shadow = (currentDepth ) > closestDepth  ? 1.0 : 0.0;
    
    return  FragPos.x > 0 ? 1-otherLightDistance : currentDepth - closestDepth;

}
