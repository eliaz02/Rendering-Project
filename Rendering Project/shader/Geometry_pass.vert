// Geometry Pass Vertex Shader
#version 330 core

// Input vertex attributes
layout (location = 0) in vec3 aPos;       // Position attribute
layout (location = 1) in vec2 aTexCoord;  // Texture coordinate
layout (location = 2) in vec3 aNormal;    // Normal attribute
layout (location = 3) in vec3 aTangent;   // Tangent attribute
layout (location = 4) in vec3 aBitangent; // Bitangent attribute

// Output data to fragment shader
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN;

// Uniforms for transformations
uniform mat4 model;         // Model matrix
uniform mat4 view;          // View matrix
uniform mat4 projection;    // Projection matrix

void main()
{
    // Calculate world space position
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Calculate final position in clip space
    gl_Position = projection * view * vec4(FragPos, 1.0);
    
    // Pass texture coordinates to fragment shader
    TexCoord = aTexCoord;
    
    // Calculate normal matrix for proper normal transformation
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    
    // Transform tangent space vectors to world space
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    
    // Create TBN matrix for normal mapping
    TBN = mat3(T, B, N);
    
    // Transform normal to world space
    Normal = normalMatrix * aNormal;
}