// src/main.cpp

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // Needed for perspective and lookAt functions
#include <glm/gtc/type_ptr.hpp>         // Needed for glm::value_ptr
#include <vector>

// --- STB IMAGE SETUP ---
// Define this implementation flag in ONE source file (main.cpp)
#define STB_IMAGE_IMPLEMENTATION 
#include "../include/stb_image.h" // Assuming you placed it in include/
// -----------------------

// --- JSON SETUP ---
#include "../include/json.hpp" // Assuming you placed it in include/
using json = nlohmann::json;
// ------------------

// Include GLAD before GLFW
#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include "../include/Camera.h" 
#include "../include/Shader.h" 

// --- NEW: Block Data Structures ---
struct BlockDefinition {
    unsigned int id = 0;
    std::string name = "Air";
    std::string texturePath = "";
    bool isOpaque = false;
    // New: Texture index for the shader (index into the Texture Array)
    unsigned int textureIndex = 0; 
};

// Global map to store block definitions (Key: Block ID)
std::map<unsigned int, BlockDefinition> blockDefs;

// Global ID for the new OpenGL Texture Array
unsigned int blockTextureArrayID = 0;

// --- Function Declarations ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods); // <-- NEW DECLARATION

// --- Window Constants ---
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Camera setup
Camera camera(glm::vec3(1.0f, 8.0f, 0.0f)); // Move camera back and to the left
float deltaTime = 0.0f;	
float lastFrame = 0.0f; 

// --- NEW: Player Entity Physics ---
glm::vec3 playerPos = glm::vec3(8.0f, 15.0f, 8.0f); // Start position (above the world)
glm::vec3 playerVelocity = glm::vec3(0.0f);
glm::vec3 PLAYER_SIZE = glm::vec3(0.6f, 1.8f, 0.6f); // Player's AABB Hitbox (Width, Height, Depth)
const float GRAVITY = -25.0f; // Gravity force (units/sec^2)
const float JUMP_VELOCITY = 10.0f;
float PLAYER_SPEED = 3.5f; // Horizontal movement speed
bool isGrounded = false;
// ------------------------------------------

// --- NEW: Raycast Offset Constant ---
// Start the ray 0.65 units forward to clear the player's horizontal volume (0.6)
//const float RAY_START_OFFSET = 0.65f; 
// ------------------------------------

// --- Global variables for Mouse Tracking ---
float lastX = SCR_WIDTH / 2.0f; 
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true; 
// ------------------------------------------

// --- VOXEL WORLD CONSTANTS ---
const int WORLD_SIZE_X = 64;
const int WORLD_SIZE_Y = 16; // A thin slice of ground
const int WORLD_SIZE_Z = 64;
// -----------------------------

// A simple 3D array to represent the world. 
// 1 = Block exists, 0 = Air.
int world[WORLD_SIZE_X][WORLD_SIZE_Y][WORLD_SIZE_Z];

// --- MESH GENERATION DATA ---
// --- NEW GLOBAL VARIABLES FOR MESHING ---
unsigned int VBO_Global; // VBO ID
int totalVertices_Global = 0; // Total vertices to draw
// --- NEW GLOBAL FOR DRAW ORDER ---
int opaqueVertexCount = 0; // Number of vertices in the opaque part of the VBO
// ----------------------------------------

// --- NEW GLOBAL: Currently selected block ID for placement ---
unsigned int currentPlacementBlockID = 4; // Default to Block ID 4 (Dirt, based on your main() init)
// -----------------------------------------------------------


// 8 floats per vertex: (X, Y, Z), (Nx, Ny, Nz), (U, V)
// Each face is 6 vertices (2 triangles)
// The standard unit cube is positioned at (0, 0, 0)
// This structure is designed to be easily translated later.
static const float FACE_VERTICES[6 * 48] = {
    // --- TOP FACE (+Y) ---
    // Normals: 0.0f, 1.0f, 0.0f
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // Top-left
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // Top-right
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // Bottom-right

     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // Bottom-right
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // Bottom-left
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // Top-left

    // --- BOTTOM FACE (-Y) ---
    // Normals: 0.0f, -1.0f, 0.0f
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,

    // --- FRONT FACE (+Z) ---
    // Normals: 0.0f, 0.0f, 1.0f
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,

     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

    // --- BACK FACE (-Z) ---
    // Normals: 0.0f, 0.0f, -1.0f
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,

    // --- LEFT FACE (-X) ---
    // Normals: -1.0f, 0.0f, 0.0f
    // Vertices run: Top-Front, Top-Back, Bottom-Back (Triangle 1)
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // (U=1.0, V=1.0) Top-Front (+Z)
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // (U=0.0, V=1.0) Top-Back (-Z)
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // (U=0.0, V=0.0) Bottom-Back (-Z)

    // Vertices run: Bottom-Back, Bottom-Front, Top-Front (Triangle 2)
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // (U=0.0, V=0.0) Bottom-Back (-Z)
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // (U=1.0, V=0.0) Bottom-Front (+Z)
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // (U=1.0, V=1.0) Top-Front (+Z)

    // --- RIGHT FACE (+X) ---
    // Normals: 1.0f, 0.0f, 0.0f
    // Vertices run: Top-Front, Bottom-Back, Top-Back (Triangle 1)
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // (U=0.0, V=1.0) Top-Front (+Z)
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // (U=1.0, V=0.0) Bottom-Back (-Z)
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // (U=1.0, V=1.0) Top-Back (-Z)

    // Vertices run: Top-Front, Bottom-Front, Bottom-Back (Triangle 2)
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // (U=0.0, V=1.0) Top-Front (+Z)
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // (U=0.0, V=0.0) Bottom-Front (+Z)
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f  // (U=1.0, V=0.0) Bottom-Back (-Z)
};

// Offsets for checking neighbors (Top, Bottom, Front, Back, Left, Right)
static const int FACE_OFFSETS[6][3] = {
    {0, 1, 0},   // +Y (Top)
    {0, -1, 0},  // -Y (Bottom)
    {0, 0, 1},   // +Z (Front)
    {0, 0, -1},  // -Z (Back)
    {-1, 0, 0},  // -X (Left)
    {1, 0, 0}    // +X (Right)
};

// --- Light Source Position ---
// Adjust light position to be visible over the line of blocks
//glm::vec3 lightPos(5.0f, 3.0f, 4.0f); // Position the light source over the middle of the line
// -----------------------------

// --- NEW: UI Crosshair Globals ---
unsigned int crosshairVAO = 0;
unsigned int crosshairVBO = 0;
const float DOT_SIZE = 0.002f; // Size of the dot (as fraction of screen width/height)
const float CROSSHAIR_VERTICES[] = {
    // Position
    0.0f, 0.0f // Center of the screen (normalized device coordinates)
};
// ----------------------------------

// --- NEW: Dynamic Light Parameters (Sun/Moon Cycle) ---
// Radius of the light's orbit around the center of the world
const float LIGHT_ORBIT_RADIUS = 30.0f; 
// Time in seconds for one full 360-degree cycle (5 minutes = 300 seconds)
const float LIGHT_CYCLE_DURATION = 300.0f; 
// Center of the world (adjust based on your world size)
// The center of the 16x16x16 world is at (8, 8, 8)
const glm::vec3 WORLD_CENTER(WORLD_SIZE_X / 2.0f, WORLD_SIZE_Y / 2.0f, WORLD_SIZE_Z / 2.0f); 
// ------------------------------------------------------

bool LoadBlockDefinitions(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "ERROR::JSON::FILE_NOT_FOUND: Could not open " << path << std::endl;
        return false;
    }
    
    json data;
    try {
        data = json::parse(f);
    } catch (json::parse_error& e) {
        std::cerr << "ERROR::JSON::PARSE_ERROR: " << e.what() << " at byte " << e.byte << std::endl;
        return false;
    }

    unsigned int currentTextureIndex = 0;
    
    // The "Air" block (ID 0) is implicitly defined and must be handled
    blockDefs[0] = {0, "Air", "", false, 0}; 

    for (const auto& block : data["blocks"]) {
        BlockDefinition def;
        def.id = block.at("id").get<unsigned int>();
        def.name = block.at("name").get<std::string>();
        def.texturePath = block.at("texture").get<std::string>();
        def.isOpaque = block.at("is_opaque").get<bool>();
        
        // Assign the next available texture index
        // This will be used to sample the correct layer in the Texture Array
        def.textureIndex = currentTextureIndex++; 
        
        blockDefs[def.id] = def;
        std::cout << "Loaded Block: ID " << def.id << ", Name: " << def.name << std::endl;
    }
    
    return true;
}

// Function to load all textures into a single GL_TEXTURE_2D_ARRAY
bool LoadBlockTextures() {
    // 1. Filter out the Air block (ID 0) and gather all texture paths
    std::vector<BlockDefinition*> texturedBlocks;
    for (auto& pair : blockDefs) {
        if (pair.first != 0) {
            texturedBlocks.push_back(&pair.second);
        }
    }
    
    if (texturedBlocks.empty()) {
        std::cerr << "No textured blocks found in definitions." << std::endl;
        return true; 
    }

    // 2. Load the first image to get dimensions and format
    int width, height, nrChannels;
    // We force loading with 4 channels (RGBA) to handle transparent textures (like water) consistently
    unsigned char *data = stbi_load(texturedBlocks[0]->texturePath.c_str(), &width, &height, &nrChannels, 4); 
    
    if (!data) {
        std::cerr << "ERROR::TEXTURE::LOAD_FAILURE: Could not load texture: " << texturedBlocks[0]->texturePath << std::endl;
        return false;
    }
    stbi_image_free(data); // Free initial data after getting dimensions

    GLenum internalFormat = GL_RGBA8; // Use a specific internal format for consistency
    int numLayers = texturedBlocks.size();

    // 3. Create and Bind Texture Array
    glGenTextures(1, &blockTextureArrayID);
    glActiveTexture(GL_TEXTURE0); 
    glBindTexture(GL_TEXTURE_2D_ARRAY, blockTextureArrayID);

    // Allocate storage for the entire array (width, height, and number of layers)
    // We allocate 4 mipmap levels (MIPMAP_LEVELS) here
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 
                   1, // Mipmap levels (4 is usually enough, adjust if needed)
                   internalFormat, 
                   width, 
                   height, 
                   numLayers);

    // 4. Load remaining textures into the array layers
    for (int i = 0; i < numLayers; ++i) {
        const BlockDefinition& def = *texturedBlocks[i];
        
        // Re-load data for this layer, forcing 4 channels
        unsigned char *layerData = stbi_load(def.texturePath.c_str(), &width, &height, &nrChannels, 4); 
        
        if (!layerData) {
            std::cerr << "ERROR::TEXTURE::LOAD_FAILURE: Layer " << i << " (" << def.name << ") failed." << std::endl;
            stbi_image_free(layerData); 
            return false;
        }

        // Upload data to a specific layer in the array
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 
                        0, // M Mipmap level 0 (base level)
                        0, 0, i, // x-offset, y-offset, z-offset (layer index)
                        width, height, 1, // width, height, depth (1 layer)
                        GL_RGBA, GL_UNSIGNED_BYTE, layerData); // Use GL_RGBA since we forced 4 channels
        
        stbi_image_free(layerData);
    }

    // 5. Configure Texture Parameters
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

// CRITICAL FIX: Change MIN filter from NEAREST_MIPMAP_NEAREST to NEAREST.
// This prevents the GPU from averaging pixels from lower-resolution mipmaps,
// which is the true cause of the visible seam/bleed on block edges.
glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // CRITICAL: Generate Mipmaps for all layers
   // glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    return true;
}

// The number of floats per vertex (Position, Normal, TexCoords, BlockID)
const int VERTEX_ATTRIBUTES = 10; // NEW: Pos(3), Normal(3), TexCoord(2), BlockID(1), TexIndex(1)

// Function to check if a block exists at world coordinates (x, y, z)
bool isBlock(int x, int y, int z) {
    if (x < 0 || x >= WORLD_SIZE_X || 
        y < 0 || y >= WORLD_SIZE_Y || 
        z < 0 || z >= WORLD_SIZE_Z) 
    {
        return 0; // Out of bounds is treated as air
    }
    return world[x][y][z] != 0;
}

bool isOpaque(int x, int y, int z) {
    // Boundary Check: Out of bounds is always non-opaque (Air)
    if (x < 0 || x >= WORLD_SIZE_X || 
        y < 0 || y >= WORLD_SIZE_Y || 
        z < 0 || z >= WORLD_SIZE_Z) 
    {
        return false; 
    }

    unsigned int id = world[x][y][z];

    // Lookup the definition for the block ID
    auto it = blockDefs.find(id);
    if (it != blockDefs.end()) {
        return it->second.isOpaque;
    }

    return false; // Default to non-opaque if block ID is unknown
}

// Function Generates the final mesh for the entire world based on visible faces.
std::vector<float> GenerateMesh() {
    std::vector<float> finalMesh; 
    
    // IMPORTANT: Redefine this global constant or ensure it's 10 here.
    // const int VERTEX_ATTRIBUTES = 10; 
    
    // Iterate over every block
    for (int y = 0; y < WORLD_SIZE_Y; ++y) {
        for (int x = 0; x < WORLD_SIZE_X; ++x) {
            for (int z = 0; z < WORLD_SIZE_Z; ++z) {
                
                if (isBlock(x, y, z)) {
                    
                    float blockId = (float)world[x][y][z];
                    
                    // 1. DETERMINE TEXTURE INDEX for this block
                    unsigned int texIndex = 0;
                    if (blockDefs.count((unsigned int)blockId)) {
                        texIndex = blockDefs.at((unsigned int)blockId).textureIndex;
                    }

                    // Check its 6 faces
                    for (int face = 0; face < 6; ++face) {
                        int neighbor_x = x + FACE_OFFSETS[face][0];
                        int neighbor_y = y + FACE_OFFSETS[face][1];
                        int neighbor_z = z + FACE_OFFSETS[face][2];
                        
                        // CULLING CHECK: DRAW face ONLY IF neighbor is NOT an OPAQUE block
                        if (!isOpaque(neighbor_x, neighbor_y, neighbor_z)) {
                            
                            // FACE_VERTICES currently contains 8 floats per vertex (Pos(3), Normal(3), TexCoord(2))
                            // 6 vertices * 8 floats/vertex = 48 floats per face.
                            const int FACE_FLOATS_OLD = 6 * (VERTEX_ATTRIBUTES - 2); // 48
                            const int face_start_offset = face * FACE_FLOATS_OLD;
                            
                            // Loop for the 6 vertices in this face
                            // The stride for the data in FACE_VERTICES is 8 (VERTEX_ATTRIBUTES - 2)
                            for (int i = 0; i < FACE_FLOATS_OLD; i += (VERTEX_ATTRIBUTES - 2)) {
                                
                                // 2. PUSH POSITION, NORMAL, AND TEXCOORDS (8 FLOATS)
                                // Indices 0 through 7 from FACE_VERTICES are pushed
                                
                                // Position (x, y, z): Indices 0, 1, 2
                                finalMesh.push_back(FACE_VERTICES[face_start_offset + i + 0] + (float)x); 
                                finalMesh.push_back(FACE_VERTICES[face_start_offset + i + 1] + (float)y); 
                                finalMesh.push_back(FACE_VERTICES[face_start_offset + i + 2] + (float)z); 
                                
                                // Normal and Texture Coords: Indices 3 through 7
                                for (int j = 3; j < VERTEX_ATTRIBUTES - 2; ++j) {
                                    finalMesh.push_back(FACE_VERTICES[face_start_offset + i + j]);
                                }
                                
                                // 3. PUSH BLOCK ID (9th float)
                                finalMesh.push_back(blockId); 
                                
                                // 4. PUSH TEXTURE INDEX (10th float)
                                finalMesh.push_back((float)texIndex); 
                            }
                        }
                    }
                }
            }
        }
    }
    
    opaqueVertexCount = finalMesh.size() / VERTEX_ATTRIBUTES;
    totalVertices_Global = opaqueVertexCount; 

    return finalMesh;
}

// Function to regenerate the mesh and upload it to the GPU
void UpdateMesh() {
    // 1. Generate the new mesh based on the current world array
    std::vector<float> meshVertices = GenerateMesh();
    
    // 2. Update the global vertex count
    totalVertices_Global = meshVertices.size() / VERTEX_ATTRIBUTES;

    // 3. Upload the new data to the VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Global);
    
    // glBufferSubData is more efficient for updating, but glBufferData is safer 
    // for changing the size. We use glBufferData here as the mesh size changes frequently.
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), meshVertices.data(), GL_DYNAMIC_DRAW);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
}

// Forward declaration of the global mesh update function
extern void UpdateMesh(); 

// Global constants for movement direction (used in raycasting)
const float RAY_DISTANCE = 8.0f; // Increased distance for better interaction range
const float RAY_START_OFFSET = -0.25f; // TIGHTENED: Define a very small offset to ensure the ray starts precisely at the eye position (0.001 instead of 0.1)
extern unsigned int currentPlacementBlockID; 

// --- NEW: Raycast Result Structure ---
struct RaycastHit {
    bool hit = false;
    glm::ivec3 target_block_coord = glm::ivec3(-1); // The solid block hit (for destruction/info)
    glm::ivec3 placement_block_coord = glm::ivec3(-1); // The adjacent air block (for placement)
};

// --- NEW: Single Raycast Function (DDA Implementation) ---
RaycastHit CastSingleRay(glm::vec3 start_pos, glm::vec3 ray_dir) {
    RaycastHit result;
    
    // Integer coordinates of the current block, start in the block player is currently in
    glm::ivec3 map_pos = glm::ivec3(glm::floor(start_pos)); 
    
    // The final result (will be stored in the struct)
    glm::ivec3 target_block_coord = glm::ivec3(-1); 
    glm::ivec3 placement_block_coord = glm::ivec3(-1); 
    
    // --- DDA Voxel Traversal Setup ---
    glm::ivec3 step;
    
    // Distance (t) along the ray to travel 1 unit in X, Y, and Z
    glm::vec3 t_delta = glm::vec3(
        (ray_dir.x == 0.0f) ? 1e10f : std::abs(2.0f / ray_dir.x),
        (ray_dir.y == 0.0f) ? 1e10f : std::abs(2.0f / ray_dir.y),
        (ray_dir.z == 0.0f) ? 1e10f : std::abs(2.0f / ray_dir.z)
    );
    
    // Current distance (t_max) along the ray until the next boundary is crossed
    glm::vec3 t_max;

    // Set initial step direction and t_max
    for (int i = 0; i < 3; i++) {
        if (ray_dir[i] < 0) {
            step[i] = -1;
            t_max[i] = (start_pos[i] - (float)map_pos[i]) * t_delta[i];
        } else {
            step[i] = 1;
            t_max[i] = ((float)map_pos[i] + 1.0f - start_pos[i]) * t_delta[i];
        }
    }
    
    // --- DDA Voxel Traversal Loop ---
    float current_dist = 0.0f;
    // Track the coordinates of the *previous* block before stepping (the air block for placement)
    glm::ivec3 last_air_coord = map_pos; 
    
    while (current_dist < RAY_DISTANCE) {
        
        // --- 1. Advance to the Next Block Boundary ---
        int axis_hit = 0; // 0=X, 1=Y, 2=Z
        float epsilon = 0.04f; 

        if (t_max.x < t_max.y - epsilon) {
            if (t_max.x < t_max.z - epsilon) {
                axis_hit = 0; // X is shortest
            } else {
                axis_hit = 2; // Z is shortest
            }
        } else {
            if (t_max.y < t_max.z - epsilon) {
                axis_hit = 1; // Y is shortest
            } else {
                axis_hit = 2; // Z is shortest
            }
        }

        current_dist = t_max[axis_hit];

        // --- 2. Store current (AIR) position for placement before stepping ---
        last_air_coord = map_pos; 
        
        // --- 3. Step into the new (potential TARGET) block ---
        map_pos[axis_hit] += step[axis_hit];
        t_max[axis_hit] += t_delta[axis_hit];
        
        // --- 4. Check for Block Hit ---
        
        // Bounds Check
        if (map_pos.x < 0 || map_pos.x >= WORLD_SIZE_X || 
            map_pos.y < 0 || map_pos.y >= WORLD_SIZE_Y || 
            map_pos.z < 0 || map_pos.z >= WORLD_SIZE_Z ||
            current_dist >= RAY_DISTANCE) 
        {
             break; // Left world bounds or max distance reached
        }
        
        // Block Solid Check (ID != 0)
        if (world[map_pos.x][map_pos.y][map_pos.z] != 0) {
            // map_pos is the solid block to destroy
            target_block_coord = map_pos;
            
            // last_air_coord is the air block for placement
            placement_block_coord = last_air_coord; 
            break; // Found the target block
        }
    }

    if (target_block_coord.x != -1) {
        result.hit = true;
        result.target_block_coord = target_block_coord;
        result.placement_block_coord = placement_block_coord;
    }
    
    return result;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    
    // Only process actions on press, not release
    if (action != GLFW_PRESS) return;

    // --- 0. SINGLE RAYCAST EXECUTION ---
    
    // Set the starting position (eye height, slightly in front of camera)
    glm::vec3 base_ray_pos = camera.Position + camera.Front * RAY_START_OFFSET; 
    base_ray_pos.y += 0.25f; // Vertical position adjustment (centered on eye height)
    base_ray_pos.x -= -0.35f;
    base_ray_pos.z -= -0.35f;
    
    // The ray direction is simply the camera's forward vector
    glm::vec3 ray_dir = camera.Front;
    
    // Cast the single ray
    RaycastHit best_hit = CastSingleRay(base_ray_pos, ray_dir);
    
    // --- Raycast result extraction ---
    glm::ivec3 target_block_coord = best_hit.target_block_coord; 
    glm::ivec3 placement_block_coord = best_hit.placement_block_coord;

    // --- DEBUGGING OUTPUT ---
    if (best_hit.hit) {
        std::cout << "DEBUG HIT: Solid Block targeted at (" << target_block_coord.x << ", " << target_block_coord.y << ", " << target_block_coord.z << ") - This is the block to **DESTROY**." << std::endl;
        std::cout << "DEBUG PLACE: Target Air Spot calculated at (" << placement_block_coord.x << ", " << placement_block_coord.y << ", " << placement_block_coord.z << ") - This is the block to **PLACE**." << std::endl;
    } else {
        std::cout << "DEBUG: No block targeted within range (" << RAY_DISTANCE << " units) by the single ray." << std::endl;
    }


    // --- 1. BLOCK DESTRUCTION (Left Click) ---
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (best_hit.hit) { 
            world[target_block_coord.x][target_block_coord.y][target_block_coord.z] = 0; 
            std::cout << "ACTION: Block destroyed at: (" << target_block_coord.x << ", " << target_block_coord.y << ", " << target_block_coord.z << ")" << std::endl;
            UpdateMesh(); 
        } else {
            std::cout << "ACTION FAILED: No solid block targeted for destruction." << std::endl;
        }
    }
    
    // --- 2. BLOCK PLACEMENT (Right Click) ---
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (best_hit.hit) {
            
            // CRITICAL: Check if the placement spot is Air (ID 0). 
            if (world[placement_block_coord.x][placement_block_coord.y][placement_block_coord.z] == 0) {
                
                // --- ROBUST PLAYER OVERLAP CHECK (Prevents placement inside player) ---
                glm::ivec3 player_feet_block = glm::ivec3(glm::floor(camera.Position));
                glm::ivec3 player_head_block = player_feet_block;
                player_head_block.y += 1; 

                if (placement_block_coord != player_feet_block && 
                    placement_block_coord != player_head_block) {
                    
                    // Placement is safe
                    world[placement_block_coord.x][placement_block_coord.y][placement_block_coord.z] = currentPlacementBlockID; 
                    std::cout << "ACTION: Block placed at: (" << placement_block_coord.x << ", " << placement_block_coord.y << ", " << placement_block_coord.z << ") - ID: " << currentPlacementBlockID << std::endl;
                    UpdateMesh();
                } else {
                    // Placement failed due to player conflict
                    std::cout << "ACTION FAILED: Cannot place block inside player's occupied space (Feet: " << player_feet_block.x << ", " << player_feet_block.y << ", " << player_feet_block.z << " | Head: " << player_head_block.x << ", " << player_head_block.y << ", " << player_head_block.z << ")." << std::endl;
                }
            } else {
                std::cout << "ACTION FAILED: Placement spot (" << placement_block_coord.x << ", " << placement_block_coord.y << ", " << placement_block_coord.z << ") is already occupied by ID " << world[placement_block_coord.x][placement_block_coord.y][placement_block_coord.z] << std::endl;
            }
        } else {
            std::cout << "ACTION FAILED: No target block was hit by crosshair ray (Required for placement)." << std::endl;
        }
    }
    
    // --- 3. BLOCK PICK (Middle Click) ---
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (best_hit.hit) { 
            unsigned int pickedID = world[target_block_coord.x][target_block_coord.y][target_block_coord.z];
            
            if (pickedID != 0) {
                currentPlacementBlockID = pickedID; 
                std::cout << "ACTION: Block selected: ID " << currentPlacementBlockID << std::endl;
            }
        }
    }
}


// TESTING PLAYER & COLLISION --------------------------------------------------------------
// Collision Check and Resolution: Checks for solid block collisions and resolves player position.
// This uses a sweeping AABB check.
void CheckCollisionAndResolve(glm::vec3& pos, glm::vec3& vel, glm::vec3 size, float dt) {
    glm::vec3 newPos = pos + vel * dt;
    glm::vec3 halfSize = size * 0.5f;

    // Define the range of block coordinates to check around the player's new position
    glm::ivec3 minBlock = glm::floor(newPos - halfSize);
    glm::ivec3 maxBlock = glm::floor(newPos + halfSize);

    for (int x = minBlock.x; x <= maxBlock.x; ++x) {
        for (int y = minBlock.y; y <= maxBlock.y; ++y) {
            for (int z = minBlock.z; z <= maxBlock.z; ++z) {

                // Check collision only against OPAQUE (solid) blocks
                if (isOpaque(x, y, z)) {
                    // Block AABB center
                    glm::vec3 blockCenter = glm::vec3((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f);
                    glm::vec3 blockHalfSize = glm::vec3(0.5f);

                    // --- Collision Resolution (Axis of Least Penetration) ---
                    glm::vec3 distance = newPos - blockCenter;
                    glm::vec3 penetration = glm::abs(distance) - (halfSize + blockHalfSize);

                    if (penetration.x < 0.0f && penetration.y < 0.0f && penetration.z < 0.0f) {
                        
                        // Find the shallowest penetration axis
                        if (penetration.x > penetration.y && penetration.x > penetration.z) {
                            // X-axis collision is deepest (resolve X)
                            float push_x = penetration.x;
                            newPos.x += (distance.x > 0) ? -push_x : push_x;
                            vel.x = 0.0f; // Stop movement on this axis
                        } 
                        else if (penetration.y > penetration.z) {
                            // Y-axis collision is deepest (resolve Y)
                            float push_y = penetration.y;
                            newPos.y += (distance.y > 0) ? -push_y : push_y;
                            
                            // Check for ground collision: if the player is hitting the top of a block
                            if (distance.y < 1.9) {
                                isGrounded = true; 
                            }
                            vel.y = 0.0f; // Stop vertical movement
                        } 
                        else {
                            // Z-axis collision is deepest (resolve Z)
                            float push_z = penetration.z;
                            newPos.z += (distance.z > 0) ? -push_z : push_z;
                            vel.z = 0.0f; // Stop movement on this axis
                        }
                    }
                }
            }
        }
    }
    
    // Update player position and global velocity
    pos = newPos;
    playerVelocity = vel;
}

// Applies gravity and calls the collision resolver
void UpdatePlayerPhysics(float dt) {
    // Reset grounded status
    isGrounded = false;
    
    // 1. Apply Gravity to Y-velocity
    playerVelocity.y += GRAVITY * dt;
    
    // 2. Perform Collision and Movement (AABB vs. World)
    CheckCollisionAndResolve(playerPos, playerVelocity, PLAYER_SIZE, dt);
    
    // 3. Apply friction/damping to horizontal movement when grounded
    if (isGrounded) {
        playerVelocity.x *= 0.8f;
        playerVelocity.z *= 0.8f;
        
        // Stop movement if below a threshold
        if (glm::length(glm::vec2(playerVelocity.x, playerVelocity.z)) < 0.05f) {
            playerVelocity.x = 0.0f;
            playerVelocity.z = 0.0f;
        }
    }
}
//--------------------------------------------------------------------------------------------------


int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Set OpenGL version to 4.6 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6); 
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 2. Create the Window Object
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Terraris Engine", NULL, NULL); 
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback); // <-- REGISTER CALLBACK
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

    // 3. Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // --- SETUP: VBO, VAO, and Shaders ---
    
    // Enable Depth Testing (CRITICAL for 3D objects to render correctly)
    glEnable(GL_DEPTH_TEST); 

    // --- 1. World Initialization ---
    // Fill the world array to create a simple 16x1x16 plane at y=0
    for (int x = 0; x < WORLD_SIZE_X; ++x) {
        for (int z = 0; z < WORLD_SIZE_Z; ++z) {
            for (int y = 0; y <= 4; ++y) {  // Fill from y = 0 to y = 4
                world[x][y][z] = 1135; // Block ID 4 (Dirt)
            }
        }
    }
    
    // Add a layer of water on top of the ground
    for (int x = 0; x < WORLD_SIZE_X; ++x) {
        for (int z = 0; z < WORLD_SIZE_Z; ++z) {
            world[x][5][z] = 1139; // Block ID 3 (Water/Transparent) at y=1
        }
    }

// --- 2. Generate the Single Mesh ---
    std::vector<float> meshVertices = GenerateMesh();
    totalVertices_Global = meshVertices.size() / VERTEX_ATTRIBUTES; // Use the global variable
    
    // VBO/VAO setup
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO_Global); // Initialize the global VBO ID

    // Bind the VAO first
    glBindVertexArray(VAO);
    
    // Bind the VBO and transfer the dynamic mesh data
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Global);
    // CHANGE FROM GL_STATIC_DRAW TO GL_DYNAMIC_DRAW
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(float), meshVertices.data(), GL_DYNAMIC_DRAW);

    // Tell OpenGL how to interpret the vertex data (position attribute at layout location 0)
    // Total size of one vertex is 10 floats (Position + Normal + TexCoords + BlockID + TexIndex)
    GLsizei stride = VERTEX_ATTRIBUTES * sizeof(float); // 10 * sizeof(float)

    // 1. Position attribute (location 0): 3 floats
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // 2. Normal attribute (location 1): 3 floats
    // Offset is 3 floats
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 3. Texture Coordinate attribute (location 2): 2 floats
    // Offset is 6 floats
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // 4. Block ID attribute (location 3): 1 float
    // Offset is 8 floats
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    
    // 5. NEW: Texture Index attribute (location 4): 1 float
    // Offset is 9 floats (3 Pos + 3 Normal + 2 TexCoord + 1 BlockID)
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(4);

    // Unbind VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);

    // 2. Create the Shader Program (for the shaded cube)
    Shader lightingShader("../shaders/basic.vs", "../shaders/basic.fs"); // RENAMED for clarity

    // 3. Create the Lamp Shader Program (for the light source)
    Shader lightCubeShader("../shaders/light.vs", "../shaders/light.fs"); // <-- NEW

    // Load crosshair shader
    Shader crosshairShader("../shaders/crosshair.vs", "../shaders/crosshair.fs");

    // --- Crosshair VAO Setup ---
    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);
    glBindVertexArray(crosshairVAO);

    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CROSSHAIR_VERTICES), CROSSHAIR_VERTICES, GL_STATIC_DRAW);

    // Position attribute (location 0 from crosshair.vs)
    // Args: index=0, size=2 components, type=GL_FLOAT, normalized=FALSE, stride=2*sizeof(float), pointer=0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // ---------------------------

    // --- TEXTURE SETUP ---
    
    // 1. Load block data and create the single GL_TEXTURE_2D_ARRAY
    // Note: You must have a blocks.json file in your ../data/ directory.
    if (!LoadBlockDefinitions("../data/blocks.json") || !LoadBlockTextures()) {
        std::cerr << "Fatal Error: Failed to load block definitions or textures. Shutting down." << std::endl;
        glfwTerminate();
        return -1;
    }

    // 2. Activate Texture Unit 0 and bind the Texture Array
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, blockTextureArrayID);

    // 3. Tell the shader where to find the single Texture Array (Unit 0)
    lightingShader.use();
    lightingShader.setInt("u_blockTextureArray", 0);

    // --------------------------------------------------------------------------
    
    // 4. The Render Loop
    while (!glfwWindowShouldClose(window)) {
        
        // --- Calculate Delta Time ---
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input processing
        // --- NEW: Physics and Player Update ---
        processInput(window);           // Calculate intended velocity
        UpdatePlayerPhysics(deltaTime); // Apply gravity, check collision, update playerPos
        
        // 1. Update Camera Position to follow the Player's Head
        // Set the camera's position based on the player entity's position, placing it near the top of the hitbox
        camera.Position = playerPos + glm::vec3(-0.45f, PLAYER_SIZE.y * 0.2f, -0.45f);
        // ------------------------------------

        // Rendering commands
        // Clear the screen AND the depth buffer
        glClearColor(0.5f, 0.8f, 1.0f, 1.0f); // <-- NEW SKY BLUE COLOR
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

        // 1. Define Transformation Matrices (Same for both objects)
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        

        lightingShader.use(); // Use the shader that performs lighting calculations
        
        // --- NEW: Calculate Dynamic Light Position ---
        float time = (float)glfwGetTime();
        
        // Calculate the current angle (in radians) based on time and cycle duration
        // This angle cycles from 0 to 2*PI over 300 seconds
        float angle = (time / LIGHT_CYCLE_DURATION) * 2.0f * glm::pi<float>();
        
        // Calculate Light Position in a 3D arc (centered at WORLD_CENTER)
        glm::vec3 lightPos;
        // X: Cosine for horizontal movement (East/West)
        lightPos.x = WORLD_CENTER.x + LIGHT_ORBIT_RADIUS * glm::cos(angle);
        // Y: Sine for vertical movement (Sun height/arc). Offset ensures it starts at 0 height.
        lightPos.y = WORLD_CENTER.y + LIGHT_ORBIT_RADIUS * glm::sin(angle);
        // Z: Fixed or set to a small radius offset for the arc plane
        lightPos.z = WORLD_CENTER.z + 0.0f; 
        
        // Ensure the light is always above the horizon (y > 0). If using a 360-arc, this means the light
        // will pass underneath the world during the "night" phase.
        
        // ---------------------------------------------

        lightingShader.use(); // Use the shader that performs lighting calculations
        
        // Pass common matrices
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // --- Pass Lighting Data ---
        // Pass the calculated dynamic lightPos to the shader 
        lightingShader.setVec3("lightPos", lightPos); // <-- USE DYNAMIC lightPos
        
        // For a day/night cycle, we can also vary the intensity (lightColor) based on the sun's height (lightPos.y)
        // For now, let's keep it fixed or adjust it slightly:
        lightingShader.setVec3("lightColor", glm::vec3(1.0f, 0.78f, 0.0f)); // Use a bright color for the "sun"

        
        // Pass the camera's world position for specular calculations 
        lightingShader.setVec3("viewPos", camera.Position); 
        // ----------------------------------------------------

        // Model matrix for the main block (at origin, 1x1 scale)
        glm::mat4 model = glm::mat4(1.0f); 
        lightingShader.setMat4("model", model);

        // Draw the cube
        // Bind the geometry
        // 3. Draw the single generated mesh
        glBindVertexArray(VAO);
        // Draw using the global count
        glDrawArrays(GL_TRIANGLES, 0, totalVertices_Global); // <-- USE GLOBAL COUNT


        // --- PASS 2: DRAW THE LIGHT CUBE (LAMP) ---
        lightCubeShader.use(); // Use the shader that just outputs white
        
        // Pass common matrices
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        
        lightCubeShader.setVec3("cubeColor", glm::vec3(1.0f, 1.0f, 0.0f)); // Pure Yellow
        // Model matrix for the light cube: translate to lightPos and scale down
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos); // <-- Use DYNAMIC lightPos for the cube
        model = glm::scale(model, glm::vec3(5.5f)); // Make it a slightly larger sun
        lightCubeShader.setMat4("model", model);
        
        // Draw the cube (re-using the same VAO as the object)
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Swap the buffers
        //glfwSwapBuffers(window);
        
        
    // --- PASS 3: DRAW THE UI CROSSHAIR (2D SCREEN SPACE) ---
    
    // CRITICAL FIX: Save original viewport and set to full window size
    int viewPort[4];
    glGetIntegerv(GL_VIEWPORT, viewPort); // Get the 3D viewport dimensions
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // Set to full window size
    
    // 0. CRITICAL: Disable depth test so the 2D crosshair is drawn over the 3D scene
    glDisable(GL_DEPTH_TEST); 
    
    // 1. Enable point size drawing
    glEnable(GL_PROGRAM_POINT_SIZE);

    // 2. Use the crosshair shader
    crosshairShader.use();

    // 3. Set MVP matrices to Identity
    glm::mat4 identity = glm::mat4(1.0f);
    crosshairShader.setMat4("model", identity);
    crosshairShader.setMat4("view", identity);
    crosshairShader.setMat4("projection", identity);

    // CRITICAL: Set the point size via the uniform
    crosshairShader.setFloat("pointSize", 7.0f); // Use a slightly larger size (7 pixels) to be sure

    // REMOVE: glPointSize(5.0f); // <- This line must be gone

    // Set the dot color
    crosshairShader.setVec3("crosshairColor", glm::vec3(1.0f, 1.0f, 1.0f)); 

    // 4. Draw the point
    glBindVertexArray(crosshairVAO);
    glDrawArrays(GL_POINTS, 0, 1);
    
    // 6. Cleanup
    glBindVertexArray(0);
    glDisable(GL_PROGRAM_POINT_SIZE);
    
    // 7. CRITICAL: Re-enable depth test for the next 3D frame
    glEnable(GL_DEPTH_TEST); 
    // --------------------------------------------------------

    glfwSwapBuffers(window);
    glfwPollEvents();
    // --------------------------------------------------------

    }

    // 5. Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO_Global);
    
    glfwTerminate();
    return 0;
}

// --- Function Definitions (mouse_callback, processInput, framebuffer_size_callback) are unchanged and defined below ---

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}
int cID = 1;
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Reset horizontal velocity
    playerVelocity.x = 0.0f;
    playerVelocity.z = 0.0f;
    bool isSneaking = false;
    // Calculate forward/right vectors based on camera's view (horizontal-only)
    glm::vec3 forward = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
    glm::vec3 right = glm::normalize(glm::cross(forward, camera.Up));
    
    // Horizontal Movement (Set desired velocity)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        playerVelocity += forward * PLAYER_SPEED;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        playerVelocity -= forward * PLAYER_SPEED;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        playerVelocity -= right * PLAYER_SPEED;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        playerVelocity += right * PLAYER_SPEED;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS){
        if (isSneaking == false){isSneaking = true;}
    }
    if (isSneaking == true){PLAYER_SIZE = glm::vec3(0.6f, 1.5f, 0.6f); PLAYER_SPEED = 2.5f;}
    else{PLAYER_SIZE = glm::vec3(0.6f, 1.8f, 0.6f); PLAYER_SPEED = 3.5f;}
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
    if (isSneaking == false){PLAYER_SPEED = 5.5f;}
    }
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        {
            cID++;
                currentPlacementBlockID = cID;
        }
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        {
            cID--;
                currentPlacementBlockID = cID;
        }
        
        
    
        
    
    // Jumping (Spacebar)
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
        playerVelocity.y = JUMP_VELOCITY;
        isGrounded = false; // Prevents spamming jump
    }
    
    // We no longer move UP/DOWN with controls, as that's handled by gravity and collision.
    // REMOVE: if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camera.ProcessKeyboard(UP, deltaTime);
    // REMOVE: if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera.ProcessKeyboard(DOWN, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}