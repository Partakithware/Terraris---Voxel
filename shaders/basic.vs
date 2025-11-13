#version 460 core
layout (location = 0) in vec3 aPos;     
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in float aBlockID;
layout (location = 4) in float aTexIndex; // NEW: Texture Array Index Input (Location 4)

out vec3 Normal; 
out vec3 FragPos; 
out vec2 TexCoords;
out float BlockID;
out float TexIndex; // NEW: Texture Index Output

// Uniforms 
uniform mat4 model;       
uniform mat4 view;        // The view matrix (from Camera) [cite: 22]
uniform mat4 projection;  // The projection matrix (Perspective) [cite: 23]

void main() {
    // Calculate the position of the fragment in world space [cite: 23]
    FragPos = vec3(model * vec4(aPos, 1.0));
    // Calculate the Normal vector for lighting [cite: 24]
    Normal = mat3(transpose(inverse(model))) * aNormal;
    // Standard position transformation [cite: 25]
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    
    // Pass Texture Coords and Block ID to the fragment shader [cite: 26]
    TexCoords = aTexCoords;
    BlockID = aBlockID;
    TexIndex = aTexIndex; // Pass the texture index
}