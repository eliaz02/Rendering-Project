// Geometry Pass Fragment Shader
#version 330 core

// Input from vertex shader
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in mat3 TBN;

// G-buffer outputs
layout (location = 0) out vec3 gPosition;       // World space position 
layout (location = 1) out vec4 gNormalShiness;  // World space normal + shininess
layout (location = 2) out vec4 gColorSpec;      // Diffuse color (RGB) + specular intensity (A)


// Material properties
struct Material {
    sampler2D diffuse;      // Diffuse texture
    sampler2D specular;     // Specular texture
    sampler2D normal;       // Normal texture
    sampler2D alpha;        // Alpha texture
    vec3 diffuseColor;      // Diffuse color (used if no texture)
    vec3 ambientColor;      // Ambient color
    vec3 specularColor;     // Specular color
    float shininess;        // Specular shininess factor
};

// Uniforms
uniform Material material;
uniform bool hasDiffuseTexture;
uniform bool hasSpecularTexture;
uniform bool hasNormalTexture;
uniform bool hasAlphaTexture;

void main()
{
    // Calculate final normal (with normal mapping if available)
    vec3 finalNormal;
    if (hasNormalTexture) {
        // Sample normal map and transform from [0,1] to [-1,1]
        vec3 normalMap = texture(material.normal, TexCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0;
        // Transform normal from tangent space to world space
        finalNormal = normalize(TBN * normalMap);
    } else {
        finalNormal = normalize(Normal);
    }
    
    // Calculate diffuse color
    vec3 diffuseColor;
    if (hasDiffuseTexture) {
        diffuseColor = texture(material.diffuse, TexCoord).rgb;
    } else {
        diffuseColor = material.diffuseColor;
    }
    
    // Calculate specular intensity
    float specularIntensity;
    if (hasSpecularTexture) {
        specularIntensity = texture(material.specular, TexCoord).r;
    } else {
        // Use the average of specular color components as intensity
        specularIntensity = (material.specularColor.r + material.specularColor.g + material.specularColor.b) / 3.0;
    }
    
    // Check alpha for transparency (optional - for alpha testing)
    float alphaValue = 1.0;
    if (hasAlphaTexture) {
        alphaValue = texture(material.alpha, TexCoord).r;
        // Discard transparent fragments if needed
        if (alphaValue < 0.1) {
            discard;
        }
    }
    
    // Fill G-buffer
    // Position buffer: world space position + linear depth (for later use)
    gPosition = FragPos;
    
    // Normal buffer: world space normal + shininess
    gNormalShiness = vec4(finalNormal, material.shininess);
    
    // Color + Specular buffer: diffuse color (RGB) + specular intensity (A)
    gColorSpec = vec4(diffuseColor, specularIntensity);
        
   
}