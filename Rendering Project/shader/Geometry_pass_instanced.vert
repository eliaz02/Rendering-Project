#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in mat4 instanceModel; 

// Output data to fragment shader
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    // Use the 'instanceModel' attribute instead of the 'model' uniform
    vec4 worldPos = instanceModel * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    TexCoord = aTexCoords;

    // Calculate TBN for normal mapping
    mat3 normalMatrix = transpose(inverse(mat3(instanceModel)));
    vec3 T = normalize(normalMatrix * vec3(1.0, 0.0, 0.0)); // Tangent
    vec3 N = normalize(normalMatrix * aNormal);             // Normal
    T = normalize(T - dot(T, N) * N);                       // Re-orthogonalize
    vec3 B = cross(N, T);                                   // Bitangent
    TBN = mat3(T, B, N);
    
    Normal = N;
    
    gl_Position = projection * view * worldPos;
}