#version 460 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in float BlockID; 
in float TexIndex; // Texture Index Input

// Texture Samplers
uniform sampler2DArray u_blockTextureArray; // Single Texture Array Sampler (Unit 0)

// Lighting Uniforms
uniform vec3 lightPos;      
uniform vec3 viewPos;       
uniform vec3 lightColor;    

void main() {
    // 1. Get the material color by sampling the correct layer in the Texture Array
    // texture() takes vec3 coords: (u, v, layer_index)
    vec4 textureSample = texture(u_blockTextureArray, vec3(TexCoords, TexIndex)); 

    // Transparency Culling: Discard fragments based on the texture's alpha channel
    if (textureSample.a < 0.1) {
        discard; 
    }

    vec3 cubeColor = textureSample.rgb;

    // Material property: how shiny the object is
    float shininess = 32.0f; 

    // --- Lighting Calculations (Point Light) ---

    // 1. Ambient Lighting 
    float ambientStrength = 0.1f;
    vec3 ambient = ambientStrength * lightColor;
    
    // 2. Diffuse Lighting 
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(lightPos - FragPos); 
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;

    // 3. Specular Lighting 
    vec3 viewDir = normalize(viewPos - FragPos); 
    vec3 reflectDir = normalize(reflect(-lightDirection, norm)); 
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = spec * lightColor;

    // 4. Final Color
    vec3 result = (ambient + diffuse + specular) * cubeColor;
    FragColor = vec4(result, 1.0f); 
}