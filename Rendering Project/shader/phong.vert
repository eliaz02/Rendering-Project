// Vertex Shader
#version 330 core

// Input vertex attributes - these match the mesh class attributes
layout (location = 0) in vec3 aPos;       // Position attribute from mesh
layout (location = 1) in vec2 aTexCoord;  // Texture coordinate from mesh
layout (location = 2) in vec3 aNormal;    // Normal attribute from mesh
layout (location = 3) in vec3 aTangent;    // tanget attribute from mesh
layout (location = 4) in vec3 aBitangent;    // tanget attribute from mesh

// Output data to fragment shader
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN;

out vec4 LightSpacePos; 

// Uniforms for transformations
uniform mat4 model;         // Model matrix
uniform mat4 view;          // View matrix
uniform mat4 projection;    // Projection matrix

uniform mat4 lightSpaceMatrix; // Projection matrix * View Matrix of the light 



void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Calculate final position in clip space
    gl_Position = projection * view * vec4(FragPos, 1.0);   
    LightSpacePos = lightSpaceMatrix * vec4(FragPos, 1.0);
    
    // Pass texture coordinates to fragment shader
    TexCoord = aTexCoord;
    // Using transpose(inverse(model)) to handle non-uniform scaling correctly
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix* aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);

    TBN = mat3(T, B, N);       
    
    // Transform normal to world space (excluding translation)
    Normal = normalMatrix * aNormal;
}
