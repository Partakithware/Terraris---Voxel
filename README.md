# Terraris---Voxel
An example project written in C++

Project Title & Description: "Terraris Engine: A C++ OpenGL Voxel Renderer."

Features: (e.g., AABB Collision, DDA Raycasting, Optimized Mesh Generation).

Dependencies: List all external libraries (GLFW, GLAD, GLM, STB_Image, nlohmann/json).

Build Instructions: ```meson compile -C builddir```

Controls: W,A,S,D,Space,N,M,Left-Shift,Left-CTRL

CTRL: Crouch
Shift: Sprint
N & M: Switch Block ID to Place
Space: Jump

Rendering	Modern OpenGL Pipeline	Utilizes OpenGL 4.6 Core Profile for efficient, modern rendering.
	Single-Pass Texture Array	Employs a GL_TEXTURE_2D_ARRAY for all block textures, eliminating costly texture-binding calls and ensuring high-performance asset switching.
	Custom Shader System	Uses a modular Shader class to manage vertex, fragment, and geometry shaders for rendering blocks, lights, and UI elements.
Voxel Geometry	Optimized Face Culling	Implements intelligent mesh generation that discards block faces hidden by adjacent opaque blocks, drastically reducing draw calls and vertex count.
	Dynamic Meshing (VBO/VAO)	Efficiently regenerates and uploads the entire world mesh to the GPU using GL_DYNAMIC_DRAW when blocks are added or destroyed.
Physics & Interaction	DDA Raycasting Algorithm	Uses the Digital Differential Analyzer (DDA) algorithm for precise, high-speed determination of block targets for destruction and placement.
	AABB Collision Resolution	Implements Axis-Aligned Bounding Box (AABB) collision detection with world voxels, resolving penetrations by isolating the axis of least resistance for smooth, solid movement.
	Simulated Gravity	Includes basic Newtonian physics with a gravity constant, velocity tracking, and grounding checks for a believable player experience.
Engine Management	JSON Block Definitions	Loads all block properties (ID, name, texture, opacity) from an external JSON file, allowing for easy expansion and definition of new content.
	First-Person Camera	Features a Camera class for free-look movement and mouse input handling, including pitch and yaw control.


1. basic.vs (Vertex Shader)

This shader correctly handles the necessary transformations and data passing:

    Input: Takes position, normal, UV coordinates, Block ID, and Texture Index from the vertex buffer.

    Output (to basic.fs):

        FragPos: The vertex position in world space.

        Normal: The normal vector in world space.

        TexCoord: The UV coordinates for texture sampling.

        TextureIndex: The layer index for the texture array.

    Action: Transforms the vertex position from model space (the block's local coordinates) to clip space using the projection * view * model matrices.

2. basic.fs (Fragment Shader)

This shader is currently focused on sampling the texture array. To implement dynamic lighting, you need to use the Normal, FragPos, and the lighting uniforms you pass from main.cpp.

And etc...

